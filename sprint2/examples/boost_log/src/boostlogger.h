#pragma once
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/date_time.hpp>

using namespace std::literals;
namespace logging = boost::log;
namespace sinks = boost::log::sinks;
namespace keywords = boost::log::keywords;
namespace expr = boost::log::expressions;
namespace attrs = boost::log::attributes;

BOOST_LOG_ATTRIBUTE_KEYWORD(line_id, "LineID", unsigned int)
BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "TimeStamp", boost::posix_time::ptime)

void MyFormatter(logging::record_view const& rec, logging::formatting_ostream& strm) {
    // чтобы поставить логгеры в равные условия, уберём всё лишнее
    auto ts = rec[timestamp];
    strm << to_iso_extended_string(*ts) << ": ";

    // выводим само сообщение
    strm << rec[expr::smessage];
}

void InitBoostLogFilter() {
    logging::add_common_attributes();

    logging::add_file_log(
        keywords::file_name = "sample.log",
        keywords::format = &MyFormatter
    );
}

void BoostLogIndexInThread(int f, int i) {
    BOOST_LOG_TRIVIAL(info) << "Thread "sv << f << " index "sv << i;
}