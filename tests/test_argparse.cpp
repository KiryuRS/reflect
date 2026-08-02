#include "../include/argparse/parser.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <filesystem>

namespace mocks {

struct [[=krrs::reflect::trait]] option_1
{
    int number_of_threads;
    [[=krrs::argparse::sf_trait]] std::filesystem::path filepath;
    std::string_view consul_url = "http://localhost:8500";

    constexpr auto operator<=>(const option_1&) const noexcept = default;
};

struct [[=krrs::reflect::trait]] option_2
{
    int workers = 4;
    short port = 8080;
    double epsilon_value = 1e-5;

    constexpr auto operator<=>(const option_2&) const noexcept = default;
};

struct [[=krrs::reflect::trait]] option_3
{
    [[=krrs::argparse::sf_trait]] std::vector<int> data_points;
    [[=krrs::argparse::sf_trait]] std::unordered_set<std::string> metric_names;
    std::array<double, 10> buckets;

    constexpr auto operator<=>(const option_3&) const noexcept = default;
};

} // namespace mocks

namespace tests {

using namespace ::testing;

TEST(test_argparse, test_parse_simple)
{
    // consul_url should be default value - parse_args should succeed
    {
        const char* argv[] = {"dummy_exe", "--number_of_threads", "8", "-f", "/opt/gcc/15"};

        const auto result = krrs::argparse::parse_args<mocks::option_1>(std::ranges::size(argv), argv);
        ASSERT_TRUE(result.has_value()) << "unexpected parsing error!";

        const mocks::option_1 expected{.number_of_threads = 8, .filepath = "/opt/gcc/15", .consul_url = "http://localhost:8500"};
        EXPECT_EQ(result.value(), expected);
    }

    // all have default value - parse_args should succeed
    {
        const char* argv[] = {"dummy_exe"};

        const auto result = krrs::argparse::parse_args<mocks::option_2>(std::ranges::size(argv), argv);
        ASSERT_TRUE(result.has_value()) << "unexpected parsing error!";

        const mocks::option_2 expected{.workers = 4, .port = 8080, .epsilon_value = 1e-5};
        EXPECT_EQ(result.value(), expected);
    }
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

} // namespace tests
