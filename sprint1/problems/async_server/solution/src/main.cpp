#include "sdk.h"
//
#include <boost/asio/signal_set.hpp>
#include <mutex>
#include <thread>
#include <vector>

#include "http_server.h"

namespace {

namespace net = boost::asio;
using namespace std::literals;
namespace sys = boost::system;
namespace http = boost::beast::http;

// Запрос, тело которого представлено в виде строки
using StringRequest = http::request<http::string_body>;
// Ответ, тело которого представлено в виде строки
using StringResponse = http::response<http::string_body>;

// Пространство имен Content  - совокупность констант,
// задающих значения HTTP-заголовков
namespace Content {

constexpr static std::string_view TEXT_HTML = "text/html"sv;

}; // namespace Content

// Создаёт StringResponse с заданными параметрами
StringResponse MakeStringResponse(http::status status, std::string_view body, unsigned http_version,
                                  bool keep_alive,
                                  size_t content_length, 
                                  std::string_view content_type = Content::TEXT_HTML) {
    StringResponse response(status, http_version);
    response.set(http::field::content_type, content_type);
    response.body() = body;
    response.content_length(content_length);
    response.keep_alive(keep_alive);
    return response;
}

StringResponse HandleRequest(StringRequest&& req) {
    const auto text_response = [&req](http::status status, std::string_view text, size_t size) {
        return MakeStringResponse(status, text, req.version(), req.keep_alive(), size);
    };

    /* Анализ запроса req ... */
    auto http_method = req.method_string();
    if(http_method == "GET"){
        std::ostringstream responce_body;
        responce_body << "Hello, " << req.target().substr(1);
        auto responce = text_response(http::status::ok, responce_body.str(), responce_body.str().size());
        return responce;
    } else if(http_method == "HEAD"){
        std::ostringstream responce_body;
        responce_body << "Hello, " << req.target().substr(1);
        auto responce = text_response(http::status::ok, "" ,responce_body.str().size());
        return responce;
    }

    std::string_view responce_body =  "Invalid method"sv;
    
    auto responce = text_response(http::status::method_not_allowed, responce_body, responce_body.size());
    responce.set("Allow", "GET, HEAD");
    return responce;
} 

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

int main() {
    const unsigned num_threads = std::thread::hardware_concurrency();

    net::io_context ioc(num_threads);

    // Подписываемся на сигналы и при их получении завершаем работу сервера
    net::signal_set signals(ioc, SIGINT, SIGTERM);
    signals.async_wait([&ioc](const sys::error_code& ec, [[maybe_unused]] int signal_number) {
        if (!ec) {
            ioc.stop();
        }
    });

    const auto address = net::ip::make_address("0.0.0.0");
    constexpr net::ip::port_type port = 8080;
    http_server::ServeHttp(ioc, {address, port}, [](auto&& req, auto&& sender) {
        sender(HandleRequest(std::forward<decltype(req)>(req)));
    });

    // Эта надпись сообщает тестам о том, что сервер запущен и готов обрабатывать запросы
    std::cout << "Server has started..."sv << std::endl;

    RunWorkers(num_threads, [&ioc] {
        ioc.run();
    });
}
