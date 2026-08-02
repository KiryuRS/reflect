#include "../include/reflect/reflect.hpp"

#include <gtest/gtest.h>

#include <array>
#include <optional>
#include <sstream>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace mocks {

struct no_trait
{
    int x;
    double y;
    char z;
};

struct [[=krrs::reflect::trait]] with_traits
{
    int id;
    const char* name;
    double price;

    constexpr auto operator<=>(const with_traits&) const noexcept = default;
};

struct [[=krrs::reflect::trait]] derived : with_traits
{
    std::string trade_id;
};

enum class enum_no_trait
{
    NONE = 0,
    FOO,
    BAR,
};

enum class [[=krrs::reflect::trait]] enum_with_traits
{
    NONE = 0,
    PRICE_NO_DELAY,
    PRICE_MINS_15_DELAY,
    STRATEGY_PRICE,
};

} // namespace mocks

namespace tests {

using namespace ::testing;

TEST(test_reflection, test_concepts)
{
    static constexpr auto with_traits = {^^mocks::with_traits, ^^mocks::enum_with_traits, ^^mocks::derived};
    static constexpr auto no_traits = {^^mocks::no_trait, ^^mocks::enum_no_trait};
    static constexpr auto classes = {^^mocks::no_trait, ^^mocks::with_traits, ^^mocks::derived};
    static constexpr auto enums = {^^mocks::enum_no_trait, ^^mocks::enum_with_traits};

    // no reflect traits
    template for (constexpr auto meta : no_traits)
    {
        using type = [:std::meta::info(meta):];
        static_assert(!krrs::reflect::concepts::has_reflect_tag<type>);
    }

    // with reflect traits
    template for (constexpr auto meta : with_traits)
    {
        using type = [:std::meta::info(meta):];
        static_assert(krrs::reflect::concepts::has_reflect_tag<type>);
    }

    // classes
    template for (constexpr auto meta : classes)
    {
        using type = [:std::meta::info(meta):];
        static_assert(krrs::reflect::concepts::reflectable<type>);
    }

    // enums
    template for (constexpr auto meta : enums)
    {
        using type = [:std::meta::info(meta):];
        static_assert(krrs::reflect::concepts::enumerable<type>);
    }
}

TEST(test_reflection, test_ostream)
{
    {
        constexpr mocks::with_traits object{.id = 101, .name = "AAPL.OQ", .price = 0.0162346};
        std::ostringstream oss;
        oss << object;
        EXPECT_EQ(oss.str(), "with_traits{id: 101, name: AAPL.OQ, price: 0.0162346}");
    }

    {
        mocks::enum_with_traits e = mocks::enum_with_traits::PRICE_MINS_15_DELAY;
        std::ostringstream oss;
        oss << e;
        EXPECT_EQ(oss.str(), "PRICE_MINS_15_DELAY");
    }

    {
        mocks::derived object{{102, "TSLA.OQ", 0.000145}, "invalid"};
        std::ostringstream oss;
        oss << object;
        EXPECT_EQ(oss.str(), "derived{id: 102, name: TSLA.OQ, price: 0.000145, trade_id: invalid}");
    }
}

TEST(test_reflection, test_format)
{
    {
        constexpr mocks::with_traits object{.id = 101, .name = "AAPL.OQ", .price = 0.0162346};
        EXPECT_EQ(std::format("{}", object), "with_traits{id: 101, name: AAPL.OQ, price: 0.0162346}");
    }

    {
        mocks::enum_with_traits e = mocks::enum_with_traits::PRICE_MINS_15_DELAY;
        EXPECT_EQ(std::format("{}", e), "PRICE_MINS_15_DELAY");
    }
}

TEST(test_type_traits, test_instance_of)
{
    // basic tests
    static_assert(krrs::reflect::instance_of<std::vector<int>, ^^std::vector>);
    static_assert(!krrs::reflect::instance_of<std::vector<char>, ^^std::unordered_set>);
    static_assert(!krrs::reflect::instance_of<std::vector<short>, ^^std::array>);
    static_assert(krrs::reflect::instance_of<std::array<long, 10>, ^^std::array>);

    // aliasing test
    using my_vector = std::vector<double>;
    static_assert(krrs::reflect::instance_of<my_vector, ^^std::vector>);
    static_assert(krrs::reflect::instance_of<std::string_view, ^^std::basic_string_view>);

    // non class template
    static_assert(!krrs::reflect::instance_of<int, ^^std::optional>);
}

} // namespace tests
