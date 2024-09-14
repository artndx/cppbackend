#include "sdk.h"
//
#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <thread>

#include "json_loader.h"
#include "request_handler.h"

using namespace std::literals;
namespace net = boost::asio;
namespace sys = boost::system;

namespace {

// Запускает функцию fn на n потоках, включая текущий
template <typename Fn>
void RunWorkers(unsigned n, const Fn& fn) {
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

int main(int argc, const char* argv[]) {
    LOG_TO_CONSOLE();
    std::optional<cmd_parser::Args> args;
    try{
        args = cmd_parser::ParseCommandLine(argc, argv);
        if(!args.has_value()){
            LOG_SERVER_EXIT(0);
            return EXIT_SUCCESS;
        }
    }catch(const std::exception& ex){
        LOG_SERVER_EXIT(EXIT_FAILURE, ex.what());
        return EXIT_FAILURE;
    }

    try {
        // 1. Загружаем карту из файла и построить модель игры
        model::Game game = json_loader::LoadGame((*args).config_file);

        // 2. Инициализируем io_context
        const unsigned num_threads = std::thread::hardware_concurrency();
        net::io_context ioc(num_threads);

        // 3. Добавляем асинхронный обработчик сигналов SIGINT и SIGTERM
        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](const sys::error_code& ec, [[maybe_unused]] int signal_number) {
            if (!ec) {
                ioc.stop();
                std::cout << std::endl;
            }
        });   

        // 4. Создаём обработчик HTTP-запросов и связываем его с моделью игры
        std::shared_ptr<request_handler::RequestHandler> handler = std::make_shared<request_handler::RequestHandler>(game, (*args), net::make_strand(ioc));

        // 5. Запустить обработчик HTTP-запросов, делегируя их обработчику запросов
        const auto address = net::ip::make_address("0.0.0.0");
        constexpr net::ip::port_type port = 8080;
        http_server::ServeHttp(ioc, {address, port}, [&handler](auto&& req, auto&& send) {
            (*handler)(std::forward<decltype(req)>(req), std::forward<decltype(send)>(send));
        });
        

        // Эта надпись сообщает тестам о том, что сервер запущен и готов обрабатывать запросы
        std::cout << "Server has started..."sv << std::endl;
        LOG_SERVER_START(port, address.to_string());

        // 6. Запускаем обработку асинхронных операций
        RunWorkers(std::max(1u, num_threads), [&ioc] {
            ioc.run();
        });
    } catch (const std::exception& ex) {
        LOG_SERVER_EXIT(EXIT_FAILURE, ex.what());
        return EXIT_FAILURE;
    }
    LOG_SERVER_EXIT(0);
}
