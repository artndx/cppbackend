#include "http_server.h"
#include "sdk.h"

#include <boost/signals2.hpp>
#include <boost/program_options.hpp>
#include <boost/asio/io_context.hpp>
#include <iostream>
#include <thread>
#include <fstream>
#include <optional>

#include "DB_manager.h"
#include "json_loader.h"
#include "request_handler.h"
#include "save_manager.h"

using namespace std::literals;
namespace net = boost::asio;
namespace sys = boost::system;
namespace http = boost::beast::http;
using pqxx::operator"" _zv;

//constexpr const char DB_URL_ENV_NAME[]{ "GAME_DB_URL" };

namespace
{
	// Запускает функцию fn на n потоках, включая текущий
	template <typename Fn>
	void RunWorkers(unsigned n, const Fn& fn)
	{
		n = std::max(1u, n);
		std::vector<std::jthread> workers;
		workers.reserve(n - 1);
		// Запускаем n-1 рабочих потоков, выполняющих функцию fn
		while (--n) {
			workers.emplace_back(fn);
		}
		fn();
	}

}  // namespace

//Once again, I don't know where else to place this stuff below. This file seemed suitable though..
void MyFormatter(logging::record_view const& rec, logging::formatting_ostream& strm)
{
	json::object final_obj;
	//Parsing timestamp and inserting it into the final json::object
	final_obj.emplace("timestamp", pt::to_iso_extended_string(rec[timestamp].get()));

	//Inserting additional information into the final json::object
	final_obj.emplace("data", rec[additional_data].get());

	//Inserting message string into the final json::object
	final_obj.emplace("message", rec[logging::expressions::smessage].get());

	strm << json::serialize(final_obj);
}

void InitBoostLogFilter()
{
	logging::add_console_log(
		std::cout,
		keywords::format = &MyFormatter,
		keywords::auto_flush = true
	);
}

class Ticker : public std::enable_shared_from_this<Ticker> 
{
public:
	using Strand = net::strand<net::io_context::executor_type>;
	using Handler = std::function<void(std::chrono::milliseconds delta)>;

	// Функция handler будет вызываться внутри strand с интервалом period
	Ticker(Strand strand, std::chrono::milliseconds period, Handler handler)
		: strand_{ strand }
		, period_{ period }
		, handler_{ std::move(handler) } {
	}

	void Start() {
		net::dispatch(strand_, [self = shared_from_this()] {
			self->last_tick_ = Clock::now();
			self->ScheduleTick();
			});
	}

private:
	void ScheduleTick() {
		assert(strand_.running_in_this_thread());
		timer_.expires_after(period_);
		timer_.async_wait([self = shared_from_this()](sys::error_code ec) {
			self->OnTick(ec);
			});
	}

	void OnTick(sys::error_code ec) {
		using namespace std::chrono;
		assert(strand_.running_in_this_thread());

		if (!ec) {
			auto this_tick = Clock::now();
			auto delta = duration_cast<milliseconds>(this_tick - last_tick_);
			last_tick_ = this_tick;
			try {
				handler_(delta);
			}
			catch (...) {
			}
			ScheduleTick();
		}
	}

	using Clock = std::chrono::steady_clock;

	Strand strand_;
	std::chrono::milliseconds period_;
	net::steady_timer timer_{ strand_ };
	Handler handler_;
	std::chrono::steady_clock::time_point last_tick_;
};

struct Args
{
	std::string config_file;
	std::string save_file;
	std::string static_dir;
	int tick_period;
	int autosave_period = -1;
	bool randomize = false;
};

[[nodiscard]] std::optional<Args> ParseCommandLine(int argc, const char* const argv[]) 
{
	namespace po = boost::program_options;

	po::options_description desc{ "Allowed options"s };

	Args args;

	desc.add_options()
		("help,h", "produce help message")
		("tick-period,t", po::value<int>(&args.tick_period), "set tick period")
		("save-state-period,a", po::value<int>(&args.autosave_period), "set autosave period")
		("config-file,c", po::value(&args.config_file)->value_name("file"s), "set config file path")
		("state-file,s", po::value(&args.save_file)->value_name("file"s), "set save file path")
		("www-root,w", po::value(&args.static_dir)->value_name("dir"s), "set static files root")
		("randomize-spawn-points", "spawn dogs at random positions");	

	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);

	if (vm.contains("help"s))
	{
		std::cout << desc;
		return std::nullopt;
	}

	if (!vm.contains("config-file"s)) 
	{
		throw std::runtime_error("Config file has not been specified"s);
	}

	if (!vm.contains("www-root"s)) 
	{
		throw std::runtime_error("Static dir path isn't specified"s);
	}

	if (!vm.contains("tick-period"s))
	{
		args.tick_period = -1;
	}
	else
	{
		if (args.tick_period <= 0)
		{
			throw std::runtime_error("Invalid tick speed!"s);
		}
	}

	if (!vm.contains("save-state-period"s) || !vm.contains("state-file"))
	{
		args.autosave_period = -1;
	}
	else
	{
		if (args.autosave_period <= 0)
		{
			throw std::runtime_error("Invalid autosave period!"s);
		}
	}

	if (vm.contains("randomize-spawn-points"))
	{
		args.randomize = true;
	}

	return args;
}

int main(int argc, const char* argv[])
{

	Args args;

	try
	{
		if (auto tmp_args = ParseCommandLine(argc, argv))
		{
			if (tmp_args.has_value())
			{
				args = tmp_args.value();
			}
			else
			{
				return EXIT_FAILURE;
			}
		}
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	try
	{
		//Initializing from enviroment variable
		const char* db_url = std::getenv("GAME_DB_URL");
		//Checking if env. variable exists. Throwing error if it does not
		if (!db_url)
		{
			throw std::runtime_error("GAME_DB_URL is not specified");
		}

		const unsigned num_threads = std::thread::hardware_concurrency();

		//Creating and initializing the connection pool
		db::ConnectionPool conn_pool
		{
			num_threads, [db_url]
			{
				auto conn = std::make_shared<pqxx::connection>(db_url);
				return conn;
			}
		};

		{
			db::ConnectionPool::ConnectionWrapper wrap = conn_pool.GetConnection();
			pqxx::connection& connection = *wrap;
			pqxx::work work{ connection };
			work.exec(R"(CREATE TABLE IF NOT EXISTS retired_players ( id UUID CONSTRAINT player_id_constraint PRIMARY KEY, name varchar(100) NOT NULL, score integer, play_time_ms integer );)"_zv);
			work.exec(R"(CREATE INDEX IF NOT EXISTS retired_players_idx ON retired_players ( score DESC, play_time_ms, name );)"_zv);
			work.commit();
		}

		// 1. Загружаем карту из файла и построить модель игры
		model::Players player_manager_{args.randomize};
		model::Game game = json_loader::LoadGame(args.config_file, player_manager_, conn_pool);

		savesystem::SaveManager save_manager{ args.save_file, args.autosave_period, game };

		if (!args.save_file.empty())
		{
			save_manager.LoadState();
		}



		// 2. Инициализируем io_context
		net::io_context ioc(num_threads);

		// 3. Добавляем асинхронный обработчик сигналов SIGINT и SIGTERM
		net::signal_set signals(ioc, SIGINT, SIGTERM);
		
        	signals.async_wait([&ioc](const sys::error_code& ec, [[maybe_unused]] int signal_number) 
        	{
            		if (!ec) 
            		{
                		ioc.stop();
            		}
        	});	
        	
		// 4. Создаём обработчик HTTP-запросов и связываем его с моделью игры
		bool rest_api_tick_system = args.tick_period == -1;
		http_handler::RequestHandler handler{ args.static_dir, game, rest_api_tick_system, save_manager};

		const auto address = net::ip::make_address("0.0.0.0");
		constexpr net::ip::port_type port = 8080;

		// 5. Запустить обработчик HTTP-запросов, делегируя их обработчику запросов

		InitBoostLogFilter();

		http_server::ServeHttp(ioc, { address, port }, [&handler](auto&& req, auto&& send)
			{
				handler(std::forward<decltype(req)>(req), std::forward<decltype(send)>(send));
			}); 

		// Эта надпись сообщает тестам о том, что сервер запущен и готов обрабатывать запросы
		json::object logger_data{ {"port", static_cast<unsigned>(port)}, {"address", address.to_string()} };

		BOOST_LOG_TRIVIAL(info) << logging::add_value(timestamp, pt::microsec_clock::local_time()) << logging::add_value(additional_data, logger_data) << "Server has started..."sv;
		
		auto api_strand = net::make_strand(ioc);

		if (!rest_api_tick_system)
		{
			auto ticker = std::make_shared<Ticker>(api_strand, std::chrono::milliseconds(args.tick_period), [&game, &save_manager](std::chrono::milliseconds delta)
				{
					double ms = static_cast<double>(delta.count());

					game.ServerTick(ms);
					save_manager.Listen(ms);
				}
			);

			ticker->Start();
		}

		// 6. Запускаем обработку асинхронных операций
		RunWorkers(std::max(1u, num_threads), [&ioc]
			{
				ioc.run();
			});

		if(!args.save_file.empty())
		{
			save_manager.SaveState();
		}

	}
	catch (const std::exception& ex)
	{
		//Logging server exit with errors
		json::object logger_data{ {"code", EXIT_FAILURE}, {"exception", ex.what()} };

		BOOST_LOG_TRIVIAL(info) << logging::add_value(timestamp, pt::microsec_clock::local_time()) << logging::add_value(additional_data, logger_data) << "server exited"sv;

		return EXIT_FAILURE;
	}


	//Logging server exit without errors
	json::object logger_data{ {"code", 0} };

	BOOST_LOG_TRIVIAL(info) << logging::add_value(timestamp, pt::microsec_clock::local_time()) << logging::add_value(additional_data, logger_data) << "server exited"sv;
}
