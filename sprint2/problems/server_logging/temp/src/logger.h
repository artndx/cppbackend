#pragma once
#include "boost_log.h"
#include <boost/json.hpp>
#include <unordered_map>

// Запуск сервера
#define LOG(code, ...) \
    logger::Log({{"code"s, code} __VA_OPT__ (, {"exception", __VA_ARGS__})}, logger::LOG_MESSAGES::SERVER_STARTED);  

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


static const std::unordered_map<LOG_MESSAGES, std::string> STR_MESSAGES {
    {LOG_MESSAGES::SERVER_STARTED, "server started"},
    {LOG_MESSAGES::SERVER_EXITED, "server exited"},
    {LOG_MESSAGES::REQUEST_RECEIVED, "request received"},
    {LOG_MESSAGES::RESPONSE_SENT, "response sent"},
    {LOG_MESSAGES::ERROR, "error"},
};

std::ostream& operator<<(std::ostream& out, LOG_MESSAGES msg);

void JSONFormatter(logging::record_view const& rec, logging::formatting_ostream& strm);

void FileConfig();

void ConsoleConfig();

void Log(const json::value& custom_data, LOG_MESSAGES message);

}; // namespace logger