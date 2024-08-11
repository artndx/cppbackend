#include "boostlogger.h"
#include "simplelogger.h"

class DurationMeasure {
public:
    DurationMeasure() = default;
    ~DurationMeasure() {
        std::chrono::system_clock::time_point end_ts = std::chrono::system_clock::now();
        std::cout << (end_ts - start_ts_).count() << std::endl;
    }

private:
    std::chrono::system_clock::time_point start_ts_ = std::chrono::system_clock::now();
};

int main() {
    InitBoostLogFilter();

    static const int num_iterations = 100000;

    {
        std::cout << "Boost log: "sv << std::flush;
        DurationMeasure g;
        std::thread thread1([](){
            for(int i = 0; i < num_iterations; ++i) {
                BoostLogIndexInThread(1, i);
            }
        });
        std::thread thread2([](){
            for(int i = 0; i < num_iterations; ++i) {
                BoostLogIndexInThread(2, i);
            }
        });
        thread1.join();
        thread2.join();
    }
    {
        std::cout << "Custom log: "sv << std::flush;
        DurationMeasure g;
        std::thread thread1([](){
            for(int i = 0; i < num_iterations; ++i) {
                LogIndexInThread(1, i);
            }
        });
        std::thread thread2([](){
            for(int i = 0; i < num_iterations; ++i) {
                LogIndexInThread(2, i);
            }
        });
        thread1.join();
        thread2.join();
    }
}