#pragma once
#include <thread>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <mutex>

using namespace std::literals;

#define LOG(X) Logger::GetInstance().Log([&](std::ostream& o) { o << X; })

class Logger {
    static auto GetTimeStamp() {
        const auto now = std::chrono::system_clock::now();
        const auto t_c = std::chrono::system_clock::to_time_t(now);
        return std::put_time(std::localtime(&t_c), "%F %T");
    }

public:
    void LogMessage(std::string_view message) {
        Log([message](std::ostream& out) {
            out << message;
        });
    }
    
    static Logger& GetInstance() {
        static Logger obj;
        return obj;
    }

    template<class Func>
    void Log(Func&& cb) {
        std::lock_guard g{m};
        log_file_ << GetTimeStamp() << ": "sv ;
        cb(log_file_);
        log_file_ << std::endl;
    }

private:
    // для демонстрации пока оставим файл в текущей директории
    std::ofstream log_file_{"sample2.log"s};
    std::mutex m;
};

inline void LogIndexInThread(int f, int i) {
    LOG("Thread "sv << f << " index "sv << i);
}