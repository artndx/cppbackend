#include <catch2/catch_test_macros.hpp>

#include "../src/htmldecode.h"

using namespace std::literals;

TEST_CASE("Empty string") {
    CHECK(HtmlDecode(""sv) == ""s);
}

TEST_CASE("Text without mnemonics") {
    CHECK(HtmlDecode("hello"sv) == "hello"s);
}

TEST_CASE("Text with mnemonics") {
    CHECK(HtmlDecode("M&ampM&aposs"sv) == "M&M's"s);
}

TEST_CASE("Text with upper case mnemonics") {
    CHECK(HtmlDecode("M&AMPM&APOSs"sv) == "M&M's"s);
}

TEST_CASE("Text with mixed case mnemonics") {
    CHECK(HtmlDecode("M&AMPM&ApOss"sv) == "M&M&ApOss"s);
}

TEST_CASE("Text with mnemonics at the beginning, end and middle") {
    CHECK(HtmlDecode("&ltSome&amptext&gt"sv) == "<Some&text>"s);
}

TEST_CASE("Text with unfinished mnemonics") {
    CHECK(HtmlDecode("&quotHello world!&quo"sv) == "\"Hello world!&quo"s);
}

TEST_CASE("Text with mnemonics ending and not ending with a semicolon.") {
    CHECK(HtmlDecode("Johnson&amp;Johnson&amp"sv) == "Johnson&Johnson&"s);
}

// Напишите недостающие тесты самостоятельно
