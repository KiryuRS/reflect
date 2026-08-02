// Copyright (c) 2026 KiryuRS
// SPDX-License-Identifier: MIT

#include "../include/json/parser.hpp"

#include <gtest/gtest.h>

namespace mocks {

struct[[= krrs::reflect_trait]] simple
{
    int value;
    std::string_view name;
    short port;
    uint64_t timestamp;
    double epsilon_value;
};

struct [[=krrs::reflect_trait]] complex
{
    std::vector<int> data;
    simple payload;
};

struct [[=krrs::reflect_trait]] with_opt
{
    std::optional<int> has_value;
    std::optional<int> no_value;
};

} // namespace mocks

namespace tests {

using namespace ::testing;

struct test_json : ::testing::Test
{
    std::string trim_string(const std::string& str)
    {
        constexpr auto is_space = [](char c) { return std::isspace(c); };

        auto view = str | std::views::drop_while(is_space) | std::views::reverse | std::views::drop_while(is_space) | std::views::reverse;
        return std::string{view.begin(), view.end()};
    };
};

TEST_F(test_json, test_simple_encode)
{
    constexpr mocks::simple s{.value = 100, .name = "Software Developer", .port = 9000, .timestamp = 666615154114, .epsilon_value = 3.14};
    const std::string serialized = ::krrs::json::serialize(s);
    EXPECT_EQ(serialized, R"({ "value": 100, "name": "Software Developer", "port": 9000, "timestamp": 666615154114, "epsilon_value": 3.14 })");
}

TEST_F(test_json, test_complex_encode)
{
    constexpr mocks::simple s{.value = 100, .name = "Software Developer", .port = 9000, .timestamp = 666615154114, .epsilon_value = 3.14};
    const mocks::complex c{.data = {10, 11, 20, 21, 22}, .payload = s};
    const std::string serialized = ::krrs::json::serialize(c);
    EXPECT_EQ(
        serialized,
        R"({ "data": [10, 11, 20, 21, 22], "payload": { "value": 100, "name": "Software Developer", "port": 9000, "timestamp": 666615154114, "epsilon_value": 3.14 } })");
}

TEST_F(test_json, test_opt_encode)
{
    const mocks::with_opt wo{.has_value = 991, .no_value = std::nullopt};
    const std::string serialized = ::krrs::json::serialize(wo);
    EXPECT_EQ(serialized, R"({ "has_value": 991, "no_value": null })");
}

} // namespace tests
