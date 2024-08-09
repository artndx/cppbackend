#pragma once

#include <chrono>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <syncstream>
#include <string>
#include <string_view>
#include <optional>
#include <mutex>
#include <thread>

using namespace std::literals;

#define LOG(...) Logger::GetInstance().Log(__VA_ARGS__)

class Logger {
    auto GetTime() const {
        if (manual_ts_) {
            return *manual_ts_;
        }

        return std::chrono::system_clock::now();
    }

    auto GetTimeStamp() const {
        const auto now = GetTime();
        const auto t_c = std::chrono::system_clock::to_time_t(now);
        return std::put_time(std::localtime(&t_c), "%F %T");
    }

    // Для имени файла возьмите дату с форматом "%Y_%m_%d"
    std::string GetFileTimeStamp() const{
        const auto now = GetTime();
        const auto t_c = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << "/var/log/sample_log_" 
        << std::put_time(std::localtime(&t_c), "%Y_%m_%d") 
        << ".log";
        return ss.str();
    }

    Logger() = default;
    Logger(const Logger&) = delete;

public:
    static Logger& GetInstance() {
        static Logger obj;
        return obj;
    }

    // Выведите в поток все аргументы.
    template<class... Ts>
    void Log(const Ts&... args){
        std::osyncstream sync_stream(log_file_);
        sync_stream << GetTimeStamp() << ": "sv;
        ((sync_stream << args), ...);
        sync_stream << std::endl;
    }

    // Установите manual_ts_. Учтите, что эта операция может выполняться
    // параллельно с выводом в поток, вам нужно предусмотреть 
    // синхронизацию.
    void SetTimestamp(std::chrono::system_clock::time_point ts){
        std::lock_guard{m_};
        manual_ts_ = ts;
        if(const auto new_file = GetFileTimeStamp(); new_file != file_time_stamp_){
            log_file_.close();
            log_file_.open(new_file, std::ios::app);
        }
    }

private:
    std::optional<std::chrono::system_clock::time_point> manual_ts_;
    std::string file_time_stamp_ = GetFileTimeStamp();
    std::ofstream log_file_{file_time_stamp_, std::ios::app};
    std::mutex m_;
};
