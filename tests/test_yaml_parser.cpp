#include "../include/reflect/yaml_parser.hpp"

#include <ranges>
#include <sstream>
#include <vector>

#include <gtest/gtest.h>

namespace mocks {

enum class region
{
    NONE = 0,
    APAC,
    EMEA,
    AMER,
};

// no default values
struct [[=krrs::reflect::trait]] config_1
{
    std::string hostname;
    int port;
    mocks::region region;

    constexpr auto operator<=>(const config_1&) const = default;
};

// have default values
struct [[=krrs::reflect::trait]] config_2
{
    std::vector<double> override_prices = {};
    std::string application_name = "default-hello-world";
    float epsilon_value = 1e-5f;

    constexpr auto operator<=>(const config_2&) const = default;
};

} // namespace mocks

namespace tests {

using namespace ::testing;

TEST(test_yaml_parser, test_decode_no_default_values)
{
    const std::string str = R"(
config_1:
    hostname: 127.0.0.1
    port: 8080
    region: APAC
)";
    const auto obj = krrs::parser::yaml::deserialize<mocks::config_1>(str);
    const mocks::config_1 expected{.hostname = "127.0.0.1", .port = 8080, .region = mocks::region::APAC};
    EXPECT_EQ(obj, expected);
}

TEST(test_yaml_parser, test_decode_default_values)
{
    const std::string str = R"(
config_2:
    override_prices: [3.14, 9.98, 71.994]
)";
    const auto obj = krrs::parser::yaml::deserialize<mocks::config_2>(str);
    const mocks::config_2 expected{.override_prices = {3.14, 9.98, 71.994}};
    EXPECT_EQ(obj, expected);
}

TEST(test_yaml_parser, test_encode)
{
    auto trim_string = [] (const std::string& str) {
        constexpr auto is_space = [] (char c) { return std::isspace(c); };

        auto view = str
                  | std::views::drop_while(is_space)
                  | std::views::reverse
                  | std::views::drop_while(is_space)
                  | std::views::reverse;
        return std::string{view.begin(), view.end()};
    };

    const mocks::config_1 obj_1{.hostname = "sgzls1216d", .port = 9000, .region = mocks::region::AMER};
    const auto str = krrs::parser::yaml::serialize(obj_1);

    const auto expected = R"(
config_1:
  hostname: sgzls1216d
  port: 9000
  region: AMER
)";
    EXPECT_EQ(str, trim_string(expected));
}

} // namespace tests
