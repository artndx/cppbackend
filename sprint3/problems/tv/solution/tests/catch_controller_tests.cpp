#include <catch2/catch_test_macros.hpp>
#include <iostream>

#include "../src/controller.h"

SCENARIO("Controller", "[Controller]") {
    using namespace std::literals;
    GIVEN("Controller and TV") {
        TV tv;
        std::istringstream input;
        std::ostringstream output;
        Menu menu{input, output};
        Controller controller{tv, menu};

        auto run_menu_command = [&menu, &input](std::string command) {
            input.str(std::move(command));
            input.clear();
            menu.Run();
        };
        auto expect_output = [&output](std::string_view expected) {
            // В g++ 10.3 не реализован метод ostringstream::view(), поэтому приходится
            // использовать метод str()
            // Также в conan есть баг, из-за которого Catch2 не подхватывает поддержку string_view:
            // https://github.com/conan-io/conan-center-index/issues/13993
            // Поэтому expected преобразуется к строке, чтобы обойти ошибку компиляции
            CHECK(output.str() == std::string{expected});
        };
        auto expect_extra_arguments_error = [&expect_output](std::string_view command) {
            expect_output("Error: the "s.append(command).append(
                " command does not require any arguments\n"sv));
        };
        auto expect_empty_output = [&expect_output] {
            expect_output({});
        };

        WHEN("The TV is turned off") {
            /* Info */
            AND_WHEN("Info command is entered without arguments") {
                run_menu_command("Info"s);

                THEN("Output contains info that TV is turned off") {
                    expect_output("TV is turned off\n"s);
                }
            }

            AND_WHEN("Info command is entered with some arguments") {
                run_menu_command("Info some extra arguments");

                THEN("Error message is printed") {
                    expect_extra_arguments_error("Info"s);
                }
            }

            AND_WHEN("Info command has trailing spaces") {
                run_menu_command("Info  "s);

                THEN("Output contains information that TV is turned off") {
                    expect_output("TV is turned off\n"s);
                }
            }
            /* TurnOn*/
            AND_WHEN("TurnOn command is entered without arguments") {
                run_menu_command("TurnOn"s);

                THEN("TV is turned on") {
                    CHECK(tv.IsTurnedOn());
                    expect_empty_output();
                }
            }

            AND_WHEN("TurnOn command is entered with some arguments") {
                run_menu_command("TurnOn some args"s);

                THEN("Error message is printed and TV is not turned on") {
                    CHECK(!tv.IsTurnedOn());
                    expect_extra_arguments_error("TurnOn"s);
                }
            }
            /* Протестируйте остальные аспекты поведения класса Controller, когда TV выключен */

            /*TurnOff*/
            AND_WHEN("TurnOff command is entered without arguments") {
                run_menu_command("TurnOff"s);

                THEN("TV is remains turned off") {
                    CHECK(!tv.IsTurnedOn());
                    expect_empty_output();
                }
            }

            AND_WHEN("TurnOff command is entered some arguments") {
                run_menu_command("TurnOff some args"s);;

                THEN("Error message is printed and TV is remains turned off") {
                    CHECK(!tv.IsTurnedOn());
                    expect_extra_arguments_error("TurnOff"s);
                }
            }

            /*SelectChannel*/
            AND_WHEN("SelectChannel command is entered without arguments") {
                run_menu_command("SelectChannel"s);

                THEN("Error message is printed") {
                    CHECK(!tv.IsTurnedOn());
                    expect_output("Error: the SelectChannel command require one argument <channel-number>\n"s);
                }
            }

            AND_WHEN("SelectChannel command is entered with some argument") {
                run_menu_command("SelectChannel 10"s);

                THEN("Error message is printed") {
                    CHECK(!tv.IsTurnedOn());
                    expect_output("TV is turned off\n"s);
                }
            }

            /*SelectPreviousChannel*/
            AND_WHEN("SelectPreviousChannel command is entered without arguments") {
                run_menu_command("SelectPreviousChannel"s);

                THEN("Output contains info that TV is turned off") {
                    expect_output("TV is turned off\n"s);
                }
            }

            AND_WHEN("SelectPreviousChannel command is entered some arguments") {
                run_menu_command("SelectPreviousChannel some args"s);;

                THEN("Error message is printed") {
                    expect_extra_arguments_error("SelectPreviousChannel"s);
                }
            }
        }
        

        WHEN("The TV is turned on") {
            tv.TurnOn();

            /*TurnOff*/
            AND_WHEN("TurnOff command is entered without arguments") {
                run_menu_command("TurnOff"s);

                THEN("TV is turned off") {
                    CHECK(!tv.IsTurnedOn());
                    expect_empty_output();
                }
            }

            AND_WHEN("TurnOff command is entered with some arguments") {
                run_menu_command("TurnOff some args");

                THEN("Error message is printed and TV is not turned off") {
                    CHECK(tv.IsTurnedOn());
                    expect_extra_arguments_error("TurnOff"s);
                }
            }

            /*TurnOn*/
            AND_WHEN("TurnOn commans is entered without arguments"){
                run_menu_command("TurnOn"s);

                THEN("TV is remains turned on") {
                    CHECK(tv.IsTurnedOn());
                    expect_empty_output();
                }
            }

            AND_WHEN("TurnOn commans is entered with some arguments"){
                run_menu_command("TurnOn some args"s);

                THEN("Error message is printed and TV is remains turned on") {
                    CHECK(tv.IsTurnedOn());
                    expect_extra_arguments_error("TurnOn"s);
                }
            }
// Включите эту секцию, после того как реализуете метод TV::SelectChannel

            /*Info*/
            AND_WHEN("Info command is entered without arguments") {
                tv.SelectChannel(12);
                run_menu_command("Info"s);

                THEN("Current channel is printed") {
                    expect_output("TV is turned on\nChannel number is 12\n"s);
                }
            }
            
            /*SelectChannel*/
            AND_WHEN("SelectChannel command is entered with invalid argument") {
                SECTION("Argument is double is string"){
                    run_menu_command("SelectChannel xsd"s);

                    THEN("Error is printed") {
                        expect_output("Invalid channel\n"s);
                    }
                }

                SECTION("Argument is double"){
                    run_menu_command("SelectChannel 10.5"s);

                    THEN("Error is printed") {
                        expect_output("Invalid channel\n"s);
                    }
                }
            }

            AND_WHEN("SelectChannel command is entered with incorrect argument") {
                run_menu_command("SelectChannel 999"s);

                THEN("Error is printed") {
                    expect_output("Channel is out of range\n"s);
                }
            }

            AND_WHEN("SelectChannel command is entered with valid argument") {
                run_menu_command("SelectChannel 10"s);

                THEN("Channel is selected") {
                    expect_empty_output();
                }
            }

            /*SelectPreviousChannel*/
            AND_WHEN("SelectPreviousChannel command is entered without arguments") {
                run_menu_command("SelectPreviousChannel"s);

                THEN("TV is selected a previous channel") {
                    expect_empty_output();
                }
            }

            AND_WHEN("SelectPreviousChannel command is entered with some arguments") {
                run_menu_command("SelectPreviousChannel some args");

                THEN("Error message is printed") {
                    expect_extra_arguments_error("SelectPreviousChannel"s);
                }
            }
        }
    }
}
