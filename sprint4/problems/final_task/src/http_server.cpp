#include "http_server.h"

namespace http_server {

/* --------------------------- SessionBase -------------------------------- */

void SessionBase::Run(){
    // Вызываем метод Read, используя executor объекта stream_.
    // Таким образом вся работа со stream_ будет выполняться, используя его executor
    net::dispatch(stream_.get_executor(),
                beast::bind_front_handler(&SessionBase::Read, GetSharedThis()));
}

void SessionBase::Read() {
    using namespace std::literals;
    // Очищаем запрос от прежнего значения (метод Read может быть вызван несколько раз)
    request_ = {};
    stream_.expires_after(30s);
    // Считываем request_ из stream_, используя buffer_ для хранения считанных данных
    http::async_read(stream_, buffer_, request_,
                        // По окончании операции будет вызван метод OnRead
                        beast::bind_front_handler(&SessionBase::OnRead, GetSharedThis()));
}

void SessionBase::OnRead(beast::error_code ec, [[maybe_unused]] std::size_t bytes_read) {
    using namespace std::literals;
    if (ec == http::error::end_of_stream) {
        // Нормальная ситуация - клиент закрыл соединение
        return Close();
    }
    if (ec) {
        return ReportError(ec, "read"sv);
    }
    std::string ip(stream_.socket().remote_endpoint().address().to_string());
    std::string url(request_.target());
    std::string method(request_.method_string());
    LogRequestReceived(ip, url, method);
    response_timer_.Start();
    HandleRequest(std::move(request_));
}

void SessionBase::Close() {
    beast::error_code ec;
    stream_.socket().shutdown(tcp::socket::shutdown_send, ec);
    ReportError(ec, "close");
}

}  // namespace http_server
