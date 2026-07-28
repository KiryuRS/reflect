#pragma once

#include "../reflect/reflect.hpp"

#include <concepts>
#include <iomanip>
#include <optional>
#include <ranges>

namespace krrs::json::internal {

namespace concepts {

template <typename T>
concept trivial_like = std::integral<T> || std::floating_point<T>;

template <typename T>
concept string_like = std::same_as<T, std::string> || std::same_as<T, std::string_view> || std::convertible_to<T, std::string>;

template <typename T, typename RawT = std::remove_cvref_t<T>>
concept read_iterable = requires {
    requires std::ranges::range<RawT>;
    requires std::ranges::input_range<const RawT&>;
};

} // namespace concepts

// base - should not ever register this!
template <typename T>
struct convert;

template <concepts::trivial_like T>
struct convert<T>
{
    static std::string encode(T obj)
    {
        return std::to_string(obj);
    }
};

template <concepts::string_like T>
struct convert<T>
{
    static std::string encode(const T& str)
    {
        return std::string{'"'} + str + '"';
    }
};

template <reflect::instance_of<^^std::optional> T>
struct convert<T>
{
    static std::string encode(const T& obj)
    {
        if (obj.has_value())
        {
            return convert<typename T::value_type>::encode(*obj);
        }
        return "null";
    }
};

template <typename T>
    requires concepts::read_iterable<T> && (!concepts::string_like<T>) && (!reflect::instance_of<T, ^^std::optional>)
struct convert<T>
{
    static std::string encode(const T& container)
    {
        const char* delimiter = "";
        std::ostringstream oss;
        oss << '[';
        for (const auto& elem : container)
        {
            using type = std::remove_cvref_t<decltype(elem)>;
            oss << std::exchange(delimiter, ", ") << convert<type>::encode(elem);
        }
        oss << ']';
        return oss.str();
    }
};

template <typename T>
    requires reflect::concepts::krrs_reflectable<T>
struct convert<T>
{
    static std::string encode(const T& obj)
    {
        static constexpr auto members = reflect::generate_nonstatic_member_metas<T>();

        const char* delimiter = "";
        std::ostringstream oss;

        oss << "{ ";
        template for (constexpr auto member : members)
        {
            using type = [:std::meta::type_of(member):];
            constexpr auto name = std::meta::identifier_of(member);
            const auto& variable = obj.[:member:];

            oss << std::exchange(delimiter, ", ");
            if constexpr (requires { std::format("{}", std::declval<type>()); })
            {
                oss << std::format(R"("{}": {})", name, convert<type>::encode(variable));
            }
            else
            {
                oss << std::quoted(name) << ": " << convert<type>::encode(variable);
            }
        }
        oss << " }";
        return oss.str();
    }
};

} // namespace krrs::json::internal
