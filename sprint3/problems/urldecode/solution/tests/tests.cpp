#define BOOST_TEST_MODULE urlencode tests
#include <boost/test/unit_test.hpp>

#include "../src/urldecode.h"

BOOST_AUTO_TEST_CASE(UrlDecode_tests) {
    using namespace std::literals;

    BOOST_TEST(UrlDecode(""sv) == ""s);
    BOOST_TEST(UrlDecode("Hello"sv) == "Hello"s);
    BOOST_TEST(UrlDecode("Hello%20world!"sv) == "Hello world!"s);
    BOOST_TEST(UrlDecode("Hello+world!"sv) == "Hello world!"s);
    BOOST_TEST(UrlDecode("Hell%6f%20world!"sv) == "Hello world!"s);
    BOOST_TEST(UrlDecode("Hell%6F%20world!"sv) == "Hello world!"s);
    BOOST_REQUIRE_THROW(UrlDecode("Hell%6W"sv), std::invalid_argument);
    BOOST_REQUIRE_THROW(UrlDecode("Hell%00"sv), std::invalid_argument);
    BOOST_REQUIRE_THROW(UrlDecode("Hell%?#"sv), std::invalid_argument);
    BOOST_REQUIRE_THROW(UrlDecode("Hell%6"sv), std::invalid_argument);
    // Напишите остальные тесты для функции UrlDecode самостоятельно
}