// Copyright (c) 2026 KiryuRS
// SPDX-License-Identifier: MIT

#include "../include/argparse/parser.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <source_location>

namespace mocks {

struct [[=krrs::reflect_trait]] option_1
{
    int number_of_threads;
    [[=krrs::shortform_trait]] std::filesystem::path filepath;
    std::string_view consul_url = "http://localhost:8500";

    constexpr auto operator<=>(const option_1&) const noexcept = default;
};

struct [[=krrs::reflect_trait]] option_2
{
    int workers = 4;
    short port = 8080;
    double epsilon_value = 1e-5;

    constexpr auto operator<=>(const option_2&) const noexcept = default;
};

struct [[=krrs::reflect_trait]] option_3
{
    [[=krrs::shortform_trait]] std::vector<int> data_points;
    [[=krrs::shortform_trait]] std::unordered_set<std::string> metric_names;
    std::array<double, 10> buckets;
};

struct [[=krrs::reflect_trait]] option_4
{
    std::filesystem::path log_path;
    bool stdout;
    std::string symlink;
    std::array<uint8_t, 2> roll_time;
};

// with strict validations
struct [[=krrs::reflect_trait]] option_5
{
    [[=krrs::v::range{100, 200}]] int calendar_id;
    [[=krrs::v::contains{"by_books", "latest_changes_for_books"}]] std::string_view endpoint;
};

// inherited options
struct [[=krrs::reflect_trait]] option_6 : option_1
{
    std::string allowed_user;

    constexpr auto operator<=>(const option_6&) const noexcept = default;
};

} // namespace mocks

namespace tests {

using namespace ::testing;

TEST(test_argparse, test_simple)
{
    const auto expect_parse_success = [] <std::size_t N> (const char* (&argv)[N], const auto& expected) {
        using type = std::remove_cvref_t<decltype(expected)>;

        const auto result = krrs::argparse::parse_args<type>(std::ranges::size(argv), argv);
        ASSERT_TRUE(result.has_value()) << "unexpected parsing error!";
        EXPECT_EQ(result.value(), expected);
    };

    // consul_url should be default value
    const char* argv_1[] = {"dummy_exe", "--number_of_threads", "8", "-f", "/opt/gcc/15"};
    const mocks::option_1 expected_1{.number_of_threads = 8, .filepath = "/opt/gcc/15", .consul_url = "http://localhost:8500"};
    expect_parse_success(argv_1, expected_1);

    // all have default value
    const char* argv_2[] = {"dummy_exe"};
    const mocks::option_2 expected_2{.workers = 4, .port = 8080, .epsilon_value = 1e-5};
    expect_parse_success(argv_2, expected_2);

    // inherited UDT should account for all variables
    const char* argv_6[] = {"dummy_exe", "--number_of_threads", "8", "-f", "/opt/sp/gcc/16.2", "--allowed_user", "Obama"};
    const mocks::option_6 expected_6{{8, "/opt/sp/gcc/16.2", "http://localhost:8500"}, "Obama"};
    expect_parse_success(argv_6, expected_6);
}

TEST(test_argparse, test_failure_scenarios)
{
    const auto expect_parse_exception = [] <typename T, std::size_t N> (const char* (&argv)[N],
                                                                        std::string_view expected_str,
                                                                        std::source_location where = std::source_location::current()) {
        const std::string line_loc = std::format("{}:{}", where.file_name(), where.line());
        bool has_exception = false;
        try
        {
            auto _ = krrs::argparse::parse_args<T>(std::ranges::size(argv), argv);
        }
        catch (const std::invalid_argument& e)
        {
            has_exception = true;
            const std::string e_str{e.what()};
            EXPECT_EQ(expected_str, e_str) << std::format("Failed at: {}", line_loc);
        }
        EXPECT_TRUE(has_exception) << std::format("Failed at: {}", line_loc);
    };

    // should have all arguments reported as missing
    const char* argv_1[] = {"dummy_exe"};
    expect_parse_exception.template operator()<mocks::option_4>(argv_1, R"([argparse] missing required arguments: ["log_path", "stdout", "symlink", "roll_time"])");

    // roll_time should have an error
    const char* argv_2[] = {"dummy_exe", "--log_path", "/somewhere/over", "--stdout", "True", "--symlink", "road-to-nowhere", "--roll_time", "1,2,3,4"};
    expect_parse_exception.template operator()<mocks::option_4>(argv_2, R"([argparse] 1,2,3,4 cannot fit into std::array<unsigned char,2>)");
}

TEST(test_argparse, test_with_containers)
{
    const char* argv[] = {"dummy_exe", "-d", "1,2,3,4,5", "-m", "hello,world,goodbye,world", "--buckets", "10.10,11.11,12.12,13.13,14.14"};

    const auto result = krrs::argparse::parse_args<mocks::option_3>(std::ranges::size(argv), argv);
    ASSERT_TRUE(result.has_value()) << "unexpected parsing error!";

    const mocks::option_3 expected{.data_points = {1,2,3,4,5}, .metric_names = {"hello", "world", "goodbye"}, .buckets = {10.10, 11.11, 12.12, 13.13, 14.14}};
    const mocks::option_3& actual = result.value();
    EXPECT_THAT(actual.data_points, expected.data_points);
    EXPECT_THAT(actual.metric_names, UnorderedElementsAreArray(expected.metric_names));
    EXPECT_THAT(actual.buckets, expected.buckets);
}

// can't decide on how to test the output of help - for now to print in std::cout and manual verify
TEST(test_argparse, test_help)
{
    const char* argv[] = {"dummy_exe", "--help"};
    static constexpr auto options = {^^mocks::option_1, ^^mocks::option_2, ^^mocks::option_3};

    template for (constexpr auto option_meta : options)
    {
        using type = [:std::meta::info(option_meta):];
        const auto result = krrs::argparse::parse_args<type>(std::ranges::size(argv), argv);
        ASSERT_FALSE(result.has_value());

        std::cout << std::format("{}\n", result.error());
    }
}

TEST(test_argparse, test_validate)
{
    const auto expect_parse_exception = [] <typename T, std::size_t N> (const char* (&argv)[N],
                                                                        std::string_view expected_str,
                                                                        std::source_location where = std::source_location::current()) {
        const std::string line_loc = std::format("{}:{}", where.file_name(), where.line());
        bool has_exception = false;
        try
        {
            auto _ = krrs::argparse::parse_args<T>(std::ranges::size(argv), argv);
        }
        catch (const std::invalid_argument& e)
        {
            has_exception = true;
            const std::string e_str{e.what()};
            EXPECT_EQ(expected_str, e_str) << std::format("Failed at: {}", line_loc);
        }
        EXPECT_TRUE(has_exception) << std::format("Failed at: {}", line_loc);
    };

    // calendar_id within [100, 200] and endpoint in the allowed list -> passes validation
    const char* argv_ok[] = {"dummy_exe", "--calendar_id", "150", "--endpoint", "by_books"};
    const auto result = krrs::argparse::parse_args<mocks::option_5>(std::ranges::size(argv_ok), argv_ok);
    ASSERT_TRUE(result.has_value()) << "unexpected validation error!";
    EXPECT_EQ(result->calendar_id, 150);
    EXPECT_EQ(result->endpoint, "by_books");

    // calendar_id below the inclusive lower bound
    const char* argv_below[] = {"dummy_exe", "--calendar_id", "50", "--endpoint", "by_books"};
    expect_parse_exception.template operator()<mocks::option_5>(argv_below, "50 is not within the range of [100, 200]");

    // calendar_id above the inclusive upper bound
    const char* argv_above[] = {"dummy_exe", "--calendar_id", "500", "--endpoint", "by_books"};
    expect_parse_exception.template operator()<mocks::option_5>(argv_above, "500 is not within the range of [100, 200]");

    // endpoint not present in the allowed list
    const char* argv_contains[] = {"dummy_exe", "--calendar_id", "150", "--endpoint", "by_cds"};
    expect_parse_exception.template operator()<mocks::option_5>(argv_contains, R"(by_cds is not within expected list: ["by_books", "latest_changes_for_books"])");
}

} // namespace tests
