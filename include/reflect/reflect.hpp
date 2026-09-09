// Copyright (c) 2026 KiryuRS
// SPDX-License-Identifier: MIT

#pragma once

#include "enums.hpp"
#include "type_traits.hpp"

#include <format>
#include <sstream>
#include <string_view>
#include <utility>

namespace krrs::reflect {

namespace detail {

// this includes bases of T
template <typename T>
consteval std::size_t total_members_count()
{
    static constexpr auto bases = std::define_static_array(std::meta::bases_of(^^T, std::meta::access_context::current()));
    std::size_t n = std::define_static_array(std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::current())).size();
    template for (constexpr auto base : bases)
    {
        using base_type = [:std::meta::type_of(base):];
        n += total_members_count<base_type>();
    }
    return n;
}

} // namespace detail

template <concepts::reflectable T, bool IncludeBases = true>
consteval auto generate_nonstatic_member_metas()
{
    static constexpr auto self_members = std::define_static_array(std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::current()));
    if constexpr (IncludeBases)
    {
        static constexpr auto bases = std::define_static_array(std::meta::bases_of(^^T, std::meta::access_context::current()));
        std::array<std::meta::info, detail::total_members_count<T>()> all_members;
        auto iter = std::ranges::begin(all_members);

        template for (constexpr auto base : bases)
        {
            using type = [:std::meta::type_of(base):];
            static constexpr auto members = generate_nonstatic_member_metas<type>();
            iter = std::ranges::copy(members, iter).out;
        }

        std::ranges::copy(self_members, iter);
        return all_members;
    }
    else
    {
        return self_members;
    }
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
        static constexpr auto all_nsdms = generate_nonstatic_member_metas<T>();
        std::ostringstream oss;
        const char* delimiter = "";
        oss << std::meta::identifier_of(^^T) << '{';
        template for (constexpr auto meta : all_nsdms)
        {
            oss << std::exchange(delimiter, ", ") << std::meta::identifier_of(meta) << ": ";

            using member_type = [:std::meta::type_of(meta):];
            using raw_member_type = std::remove_cvref_t<member_type>;

            // single byte implicitly converts to "char". should not be the expected behavior
            if constexpr (std::same_as<raw_member_type, uint8_t>)
            {
                oss << static_cast<uint16_t>(object.[:meta:]);
            }
            else if constexpr (std::same_as<raw_member_type, int8_t>)
            {
                oss << static_cast<int16_t>(object.[:meta:]);
            }
            else
            {
                oss << object.[:meta:];
            }
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
    requires ::krrs::reflect::concepts::has_reflect_tag<T> && ::krrs::reflect::concepts::enumerable<T>
std::ostream& operator<<(std::ostream& os, T e)
{
    return os << ::krrs::reflect::enum_to_string(e);
}

// this needs more "constraints" than the previous because function overloads resolution
// depends on the number of constraints
template <typename T>
    requires ::krrs::reflect::concepts::krrs_reflectable<T> && (!::krrs::reflect::concepts::enumerable<T>)
std::ostream& operator<<(std::ostream& os, const T& object)
{
    return os << ::krrs::reflect::to_string(object);
}

namespace std {

template <typename T>
    requires ::krrs::reflect::concepts::has_reflect_tag<T> && ::krrs::reflect::concepts::enumerable<T>
struct formatter<T> : formatter<string_view>
{
    auto format(T e, format_context& ctx) const
    {
        return formatter<string_view>::format(::krrs::reflect::enum_to_string(e), ctx);
    }
};

template <typename T>
    requires ::krrs::reflect::concepts::krrs_reflectable<T> && (!::krrs::reflect::concepts::enumerable<T>)
struct formatter<T> : formatter<string>
{
    auto format(const T& object, format_context& ctx) const
    {
        return formatter<string>::format(::krrs::reflect::to_string(object), ctx);
    }
};

} // namespace std
