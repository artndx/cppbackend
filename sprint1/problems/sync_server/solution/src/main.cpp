#ifdef WIN32
#include <sdkddkver.h>
#endif
// boost.beast будет использовать std::string_view вместо boost::string_view
#define BOOST_BEAST_USE_STD_STRING_VIEW

#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <iostream>
#include <thread>
#include <optional>
#include <sstream>

namespace net = boost::asio;
using tcp = net::ip::tcp;
using namespace std::literals;
namespace beast = boost::beast;
namespace http = beast::http;

// Запрос, тело которого представлено в виде строки
using StringRequest = http::request<http::string_body>;
// Ответ, тело которого представлено в виде строки
using StringResponse = http::response<http::string_body>; 


// Пространство имен Content  - совокупность констант,
// задающих значения HTTP-заголовков
namespace Content {

constexpr static std::string_view TEXT_HTML = "text/html"sv;

}; // namespace Content

/* Чтение запроса клиента */
std::optional<StringRequest> ReadRequest(tcp::socket& socket, beast::flat_buffer& buffer) {
    beast::error_code ec;
    StringRequest req;
    // Считываем из socket запрос req, используя buffer для хранения данных.
    // В ec функция запишет код ошибки.
    http::read(socket, buffer, req, ec);

    if (ec == http::error::end_of_stream) {
        return std::nullopt;
    }
    if (ec) {
        throw std::runtime_error("Failed to read request: "s.append(ec.message()));
    }
    return req;
}

/* Вывод в консоль информации о полученном запросе */
void DumpRequest(const StringRequest& req) {
    std::cout << req.method_string() << ' ' << req.target() << std::endl;
    // Выводим заголовки запроса
    for (const auto& header : req) {
        std::cout << "  "sv << header.name_string() << ": "sv << header.value() << std::endl;
    }
}

/* Вывод в консоль информации о отправляемом ответе */
void DumpResponse(const StringResponse& res) {
    std::cout << res.result_int() << ' ' << res.result() << std::endl;
    // Выводим заголовки ответа
    for (const auto& header : res) {
        std::cout << "  "sv << header.name_string() << ": "sv << header.value() << std::endl;
    }
    std::cout << " "sv << res.body() << std::endl;
}

/*  Создаёт StringResponse с заданными параметрами */
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

/*  Возвращает StringResponce по заданному запросу */
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

/*  Обрабатывает запросы клиента по установленному соединению */
template<typename RequestHandler>
void HandleConnection(tcp::socket& socket, RequestHandler&& handle_request) {
    try {
        // Буфер для чтения данных в рамках текущей сессии.
        beast::flat_buffer buffer;

        // Продолжаем обработку запросов, пока клиент их отправляет
        while (auto request = ReadRequest(socket, buffer)) {
            StringResponse response = handle_request(*std::move(request));
            //DumpRequest(*request);
            //DumpResponse(response);
            http::write(socket, response);
            if (response.need_eof()) {
                break;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
    beast::error_code ec;
    // Запрещаем дальнейшую отправку данных через сокет
    socket.shutdown(tcp::socket::shutdown_send, ec);
} 

int main() {
    // Выведите строчку "Server has started...", когда сервер будет готов принимать подключения
    const auto address = net::ip::make_address("0.0.0.0");
    constexpr unsigned short port = 8080;
    net::io_context ioc;
    tcp::acceptor acceptor(ioc, tcp::endpoint(address, port));
    std::cout << "Server has started..." << std::endl;
    while(true){
        tcp::socket socket(ioc);
        acceptor.accept(socket);
        
        std::thread t(
            [](tcp::socket socket) {
                HandleConnection(socket, HandleRequest);
            },
            std::move(socket)
        );
        t.detach();
    }
}
