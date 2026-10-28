// Copyright (c) 2026 KiryuRS
// SPDX-License-Identifier: MIT

#pragma once

#include <meta>
#include <utility>

namespace krrs::inteoc::detail {

consteval bool is_regular_member_function(std::meta::info f)
{
    return std::meta::is_function(f)
        && !std::meta::is_special_member_function(f)
        && !std::meta::is_static_member(f)
        && std::meta::has_identifier(f);
}

template <typename T, typename RawT = std::remove_cvref_t<T>>
consteval auto regular_member_functions()
{
    static constexpr auto all_functions = std::define_static_array(std::meta::members_of(^^RawT, std::meta::access_context::current()));
    std::vector<std::meta::info> results;
    template for (constexpr auto func : all_functions)
    {
        if constexpr (is_regular_member_function(func))
        {
            results.push_back(func);
        }
    }
    return std::define_static_array(results);
}

template <typename T, typename RawT = std::remove_cvref_t<T>>
consteval std::meta::info find_matching_function(std::meta::info f)
{
    static constexpr auto all_members = std::define_static_array(std::meta::members_of(^^RawT, std::meta::access_context::current()));
    template for (constexpr auto member : all_members)
    {
        if constexpr (is_regular_member_function(member))
        {
            if (std::meta::identifier_of(member) == std::meta::identifier_of(f) && std::meta::type_of(member) == std::meta::type_of(f))
            {
                return member;
            }
        }
    }

    return std::meta::info{};
}

} // krrs::inteoc::detail
