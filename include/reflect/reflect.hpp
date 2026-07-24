// Copyright (c) 2026 KiryuRS
// SPDX-License-Identifier: MIT

#pragma once

#include <algorithm>
#include <format>
#include <meta>
#include <sstream>
#include <string_view>
#include <type_traits>
#include <utility>

namespace krrs::reflect {

namespace detail {

// for annotating a struct. e.g.
// struct [[=krrs::reflect::trait]] my_type { ... };
struct reflect_tag { };

} // namespace detail

inline constexpr detail::reflect_tag trait{};

namespace concepts {

template <typename T>
concept has_reflect_tag = [] {
    constexpr auto all_annotations = std::define_static_array(std::meta::annotations_of(^^T));
    return std::ranges::any_of(all_annotations, [] (std::meta::info meta) {
        return std::meta::type_of(meta) == std::meta::type_of(^^trait);
    });
}();

template <typename T>
concept enumerable = requires {
    std::is_enum_v<T>;
    std::meta::is_enumerator(^^T);
    { T::NONE } -> std::same_as<T>;
};

template <typename T>
concept reflectable = requires {
    std::is_class_v<T>;
    !std::is_reflection_v<T>;
    std::is_aggregate_v<T>;
};

} // namespace concepts

template <concepts::enumerable T>
inline constexpr std::string_view enum_to_string(T e)
{
    static constexpr auto enum_metas = std::define_static_array(std::meta::enumerators_of(^^T));
    template for (constexpr auto meta : enum_metas)
    {
        if ([:meta:] == e)
            return std::meta::identifier_of(meta);
    }
    return "UNKNOWN";
}

template <concepts::enumerable T>
inline constexpr T string_to_enum(std::string_view str)
{
    static constexpr auto enum_metas = std::define_static_array(std::meta::enumerators_of(^^T));
    template for (constexpr auto meta : enum_metas)
    {
        if (std::meta::identifier_of(meta) == str)
            return [:meta:];
    }
    return T::NONE;
}

template <concepts::has_reflect_tag T>
inline constexpr std::string to_string(const T& object)
{
    if constexpr (::krrs::reflect::concepts::enumerable<T>)
    {
        return enum_to_string(object);
    }
    else if constexpr (::krrs::reflect::concepts::reflectable<T>)
    {
        static constexpr auto all_nsdms = std::define_static_array(std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::current()));

        const char* delimiter = "";
        std::ostringstream oss;
        oss << std::meta::identifier_of(^^T);
        oss << '{';
        template for (constexpr auto meta : all_nsdms)
        {
            oss << std::exchange(delimiter, ", ") << std::meta::identifier_of(meta) << ": " << object.[:meta:];
        }
        oss << '}';
        return oss.str();
    }
    else
    {
        static_assert(requires { std::integral_constant<bool, false>::value; }, "type not supported!");
    }
}

} // namespace krrs::reflect

template <typename T>
    requires ::krrs::reflect::concepts::has_reflect_tag<T>
          && ::krrs::reflect::concepts::enumerable<T>
std::ostream& operator<<(std::ostream& os, T e)
{
    return os << ::krrs::reflect::enum_to_string(e);
}

// this needs more "constraints" than the previous because function overloads resolution
// depends on the number of constraints
template <typename T>
    requires ::krrs::reflect::concepts::has_reflect_tag<T> &&
             ::krrs::reflect::concepts::reflectable<T> &&
             (!::krrs::reflect::concepts::enumerable<T>)
std::ostream& operator<<(std::ostream& os, const T& object)
{
    return os << ::krrs::reflect::to_string(object);
}

namespace std {

template <typename T>
    requires ::krrs::reflect::concepts::has_reflect_tag<T>
          && ::krrs::reflect::concepts::enumerable<T>
struct formatter<T> : formatter<string_view>
{
    auto format(T e, format_context& ctx) const
    {
        return formatter<string_view>::format(::krrs::reflect::enum_to_string(e), ctx);
    }
};

template <typename T>
    requires ::krrs::reflect::concepts::has_reflect_tag<T> &&
             ::krrs::reflect::concepts::reflectable<T> &&
             (!::krrs::reflect::concepts::enumerable<T>)
struct formatter<T> : formatter<string>
{
    auto format(const T& object, format_context& ctx) const
    {
        return formatter<string>::format(::krrs::reflect::to_string(object), ctx);
    }
};

} // namespace std
