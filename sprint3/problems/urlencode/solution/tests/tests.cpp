#include <gtest/gtest.h>

#include "../src/urlencode.h"

using namespace std::literals;

TEST(UrlEncodeTestSuite, EmptyURL) {
    EXPECT_EQ(UrlEncode(""sv), ""s);
}

TEST(UrlEncodeTestSuite, OrdinaryCharsAreNotEncoded) {
    EXPECT_EQ(UrlEncode("hello"sv), "hello"s);
}


TEST(UrlEncodeTestSuite, SpacePlacedByPlus) {
    EXPECT_EQ(UrlEncode("hello world"sv), "hello+world"s);
}

TEST(UrlEncodeTestSuite, ReservedCharsAreEncoded) {
    EXPECT_EQ(UrlEncode("HelloWorld!"sv), "HelloWorld%21"s);
}

TEST(UrlEncodeTestSuite, OtherCharsAreEncoded) {
    char a = 31;
    char b = 128;
    std::string url {a, b};
    EXPECT_EQ(UrlEncode(url), "%1f%80"s);
}
/* Напишите остальные тесты самостоятельно */
