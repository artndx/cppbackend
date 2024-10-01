#include "http_server.h"

#include <boost/asio/dispatch.hpp>
#include <iostream>

namespace http_server
{
    void ReportError(beast::error_code ec, std::string_view what)
    {
        //Logging net errors
        json::object logger_data{ {"code", ec.value()}, {"text", ec.message()}, {"where", what} };

        BOOST_LOG_TRIVIAL(info) << logging::add_value(timestamp, pt::microsec_clock::local_time()) << logging::add_value(additional_data, logger_data) << "error"sv;

        //Looking back, maybe I should've set severity lvl to "error", who knows..
    }

    void SessionBase::Run()
    {
        // Вызываем метод Read, используя executor объекта stream_.
        // Таким образом вся работа со stream_ будет выполняться, используя его executor
        net::dispatch(stream_.get_executor(), beast::bind_front_handler(&SessionBase::Read, GetSharedThis()));
    }

}  // namespace http_server
