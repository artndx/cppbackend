#include "logger.h"
#include <filesystem>
#include <sstream>

using namespace std::literals;

namespace logger{

std::ostream& operator<<(std::ostream& out, LOG_MESSAGES msg){
    out << STR_MESSAGES.at(msg);
    return out;
}

BOOST_LOG_ATTRIBUTE_KEYWORD(additional_data, "AdditionalData", json::value)
BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "TimeStamp", boost::posix_time::ptime)

void JSONFormatter(logging::record_view const& rec, logging::formatting_ostream& strm) {
    json::object log;

    log.emplace("timestamp", to_iso_extended_string(*rec[timestamp]));
    log.emplace("data", *rec[additional_data]);
    log.emplace("message", *rec[logging::expressions::smessage]);

    strm << json::serialize(log);
}

void FileConfig(){
    logging::add_common_attributes();
    std::filesystem::path file_path = std::filesystem::weakly_canonical( std::filesystem::path("../../logs/") / std::filesystem::path ("sample_%N.json"));
    logging::add_file_log(
        keywords::file_name = file_path.string(),
        keywords::format = &JSONFormatter,
        keywords::open_mode = std::ios_base::app | std::ios_base::out,
        // ротируем по достижению размера 10 мегабайт
        keywords::rotation_size = 10 * 1024 * 1024,
        // ротируем ежедневно в полдень
        keywords::time_based_rotation = logging::sinks::file::rotation_at_time_point(12, 0, 0)
    );
}

void ConsoleConfig(){
    logging::add_common_attributes();
    logging::add_console_log( 
        std::clog,
        keywords::format = &JSONFormatter,
        keywords::auto_flush = true
    );
}

void Log(const json::value& data, LOG_MESSAGES message){
    BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, data)
                            << message;
}

/* Запуск сервера */
void LogServerStart(unsigned port, std::string_view address){
    json::object data;
    data["port"] = port;
    data["address"] = address.data();
    logger::Log(data, logger::LOG_MESSAGES::SERVER_STARTED);  
}

/* Остановка сервера */
void LogServerExit(unsigned code, std::optional<std::string_view> exception){
    json::object data;
    data["code"] = code;
    if(exception.has_value()){
        data["exception"] = exception.value().data();
    }
    
    logger::Log(data, logger::LOG_MESSAGES::SERVER_EXITED); 
}

/* Получение запроса */
void LogRequestReceived(std::string_view ip, std::string_view url, std::string_view method){
    json::object data;
    data["ip"] = ip.data();
    data["URL"] = url.data();
    data["method"] = method.data();

    logger::Log(data, logger::LOG_MESSAGES::REQUEST_RECEIVED);
}

/* Формирование ответа */
void LogResponseSent(std::string_view ip, size_t response_time, unsigned code, std::string_view content_type){
    json::object data;
    data["ip"] = ip.data();
    data["response_time"] = response_time;
    data["code"] = code;
    data["content_type"] = content_type.data();

    logger::Log(data, logger::LOG_MESSAGES::RESPONSE_SENT);
}

/* Возникновение ошибки */
void LogError(unsigned code, std::string_view text, std::string_view where){
    json::object data;
    data["code"] = code;
    data["text"] = text.data();
    data["where"] = where.data();

    logger::Log(data, logger::LOG_MESSAGES::ERROR);
}

/* Настройка для вывода логов */
void LogConfigure(bool toFile, bool toConsole){
    if(toFile){
        /* Настройка для вывода в файл */
        FileConfig();
    }

    if(toConsole){
        /* Настройка для вывода в консоль */
        ConsoleConfig();
    }
}

}; // namespace logger