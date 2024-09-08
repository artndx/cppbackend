#include <catch2/catch_test_macros.hpp>
#include <iostream>

#include "../src/tv.h"

namespace Catch {

template <>
struct StringMaker<std::nullopt_t> {
    static std::string convert(std::nullopt_t) {
        using namespace std::literals;
        return "nullopt"s;
    }
};

template <typename T>
struct StringMaker<std::optional<T>> {
    static std::string convert(const std::optional<T>& opt_value) {
        if (opt_value) {
            return StringMaker<T>::convert(*opt_value);
        } else {
            return StringMaker<std::nullopt_t>::convert(std::nullopt);
        }
    }
};

}  // namespace Catch

SCENARIO("TV", "[TV]") {
    GIVEN("A TV") {  // Дано: Телевизор
        TV tv;

        // Изначально он выключен и не показывает никаких каналов
        SECTION("Initially it is off and doesn't show any channel") {
            CHECK(!tv.IsTurnedOn());
            CHECK(!tv.GetChannel().has_value());
        }

        // Когда он выключен,
        WHEN("it is turned off") {
            REQUIRE(!tv.IsTurnedOn());

            // он не может переключать каналы
            THEN("it can't select any channel") {
                CHECK_THROWS_AS(tv.SelectChannel(10), std::logic_error);
                CHECK(tv.GetChannel() == std::nullopt);
                tv.TurnOn();
                CHECK(tv.GetChannel() == 1);
            }

            // а также не может переключиться на предыдущий канал
            AND_THEN("it can't select a previous channel"){
                CHECK_THROWS_AS(tv.SelectLastViewedChannel(), std::logic_error);
            }
        }

        WHEN("it is turned on first time") {  // Когда его включают в первый раз,
            CHECK(!tv.IsTurnedOn()); // Проверка на первичное включение
            tv.TurnOn();

            // то он включается и показывает канал #1
            THEN("it is turned on and shows channel #1") {
                CHECK(tv.IsTurnedOn());
                CHECK(tv.GetChannel() == 1);

                // А когда его выключают,
                AND_WHEN("it is turned off") {
                    tv.TurnOff();

                    // то он выключается и не показывает никаких каналов
                    THEN("it is turned off and doesn't show any channel") {
                        CHECK(!tv.IsTurnedOn());
                        CHECK(tv.GetChannel() == std::nullopt);
                    }
                }
            }
            // И затем может выбирать канал с 1 по 99
            AND_THEN("it can select channel from 1 to 99") {
                CHECK(tv.IsTurnedOn());

                // И когда канал выбран,
                AND_WHEN("the channel is selected"){

                    int channel = 10;
                    tv.SelectChannel(channel);
                    // то он показывает выбранный канал,
                    THEN("it show a selected channel"){
                        CHECK(tv.GetChannel() == channel);
                    }

                    // А затем может показывать предыдущий канал после включения другого канала
                    AND_THEN("it can select a previous channel after selecting a new channel"){

                        // И когда новый канал выбран,
                        AND_WHEN("a new channel is selected"){
                            int previous_channel = channel;
                            int new_channel = 20;
                            tv.SelectChannel(new_channel);
                            CHECK(tv.GetChannel() == new_channel);

                            // то выбрав предыдущий канал, он его и покажет,
                            THEN("after selecting a previous channel it is show it"){
                                tv.SelectLastViewedChannel();
                                CHECK(tv.GetChannel() == previous_channel);
                                
                                // А при повторном переключении предыдущего канала
                                AND_WHEN("after selecting a previous channel again"){
                                    tv.SelectLastViewedChannel();
                                    
                                    // он переключает два последних выбранных канала
                                    THEN("it switches the last two selected channels"){
                                        CHECK(tv.GetChannel() == new_channel);
                                    }
                                }
                            }
                        }
                    }

                    // И затем после выключения/включения
                    AND_THEN("after turning off/on a selected channel is saved"){
                        tv.TurnOff();
                        tv.TurnOn();
                        CHECK(tv.GetChannel() == channel);
                    }
                }
            }
            /* Реализуйте самостоятельно остальные тесты */

            // Но не может выбирать каналы за пределами [1, 99]
            AND_THEN("it can't show channels less 1 or more 99"){
                int incorrect_channel = 100;
                CHECK_THROWS_AS(tv.SelectChannel(incorrect_channel), std::out_of_range);
            }
        }
    }
}
