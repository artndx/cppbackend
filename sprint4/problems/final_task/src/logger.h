#pragma once
#include <boost/log/trivial.hpp>     
#include <boost/log/core.hpp>        
#include <boost/log/expressions.hpp> 
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/date_time.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/json.hpp>
#include <unordered_map>
#include <chrono>
#include <optional>

namespace logger{

namespace logging = boost::log;
namespace keywords = logging::keywords;
namespace json = boost::json;

enum class LOG_MESSAGES{
    SERVER_STARTED,
    SERVER_EXITED,
    REQUEST_RECEIVED,
    RESPONSE_SENT,
    ERROR
};

class Timer{
public:
    void Start(){
        start_ = std::chrono::system_clock::now();
    }

    size_t End(){
        auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now() - start_).count();
        start_.min();
        return dur;
    }
private:
    std::chrono::system_clock::time_point start_ = std::chrono::system_clock::now();
};

static const std::unordered_map<LOG_MESSAGES, std::string> STR_MESSAGES {
    {LOG_MESSAGES::SERVER_STARTED, "Server has started..."},
    {LOG_MESSAGES::SERVER_EXITED, "server exited"},
    {LOG_MESSAGES::REQUEST_RECEIVED, "request received"},
    {LOG_MESSAGES::RESPONSE_SENT, "response sent"},
    {LOG_MESSAGES::ERROR, "error"},
};

std::ostream& operator<<(std::ostream& out, LOG_MESSAGES msg);

void JSONFormatter(logging::record_view const& rec, logging::formatting_ostream& strm);

void FileConfig();

void ConsoleConfig();

void Log(const json::value& data, LOG_MESSAGES message);

/* Запуск сервера */
void LogServerStart(unsigned port, std::string_view address);

/* Остановка сервера */
void LogServerExit(unsigned code, std::optional<std::string_view> exception = std::nullopt);

/* Получение запроса */
void LogRequestReceived(std::string_view ip, std::string_view url, std::string_view method);

/* Формирование ответа */
void LogResponseSent(std::string_view ip, size_t response_time, 
                    unsigned code, std::string_view content_type);

/* Возникновение ошибки */
void LogError(unsigned code, std::string_view text, std::string_view where);

/* Настройка для вывода логов */
void LogConfigure(bool toFile, bool toConsole);

}; // namespace logger
