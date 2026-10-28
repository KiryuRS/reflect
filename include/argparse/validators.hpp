// Copyright (c) 2026 KiryuRS
// SPDX-License-Identifier: MIT

#pragma once

#include "../reflect/reflect.hpp"

#include <algorithm>

namespace krrs::v {

// min / max is inclusive
template <typename T>
struct range
{
    T min;
    T max;
};

template <typename T>
range(T, T) -> range<T>;

// note: needs to be structural type
template <typename T, std::size_t N>
struct contains
{
    template <std::convertible_to<std::string_view> ... Ts>
    consteval contains(Ts&& ... strs)
        : value{std::define_static_string(strs)...}
    {
    }

    template <typename ... Ts>
        requires (... && (std::integral<Ts> || std::floating_point<Ts>))
    consteval contains(Ts ... built_ins)
        : value{built_ins...}
    {
    }

   std::array<T, N> value;
};

template <typename T, typename ... Ts>
    requires (... && std::same_as<T, Ts>)
contains(T, Ts ...) -> contains<T, sizeof...(Ts) + 1>;

} // namespace krrs::v

namespace krrs::argparse {

template <typename T>
constexpr void validate_args(const T& obj)
{
    static constexpr auto members = reflect::generate_nonstatic_member_metas<T>();
    template for (constexpr auto member : members)
    {
        const auto& value = obj.[:member:];

        // possible to use std::meta::annotations_of_with_type for explicit comparison,
        // but for templated class you need to specify the template arguments
        static constexpr auto all_annotations = std::define_static_array(std::meta::annotations_of(member));
        template for (constexpr auto annotation : all_annotations)
        {
            using annotated_type = [:std::meta::type_of(annotation):];
            if constexpr (reflect::instance_of<annotated_type, ^^v::range>)
            {
                constexpr auto expected_range = std::meta::extract<annotated_type>(annotation);
                if (value < expected_range.min || value > expected_range.max)
                {
                    throw std::invalid_argument(std::format("{} is not within the range of [{}, {}]", value, expected_range.min, expected_range.max));
                }
            }
            else if constexpr (reflect::instance_of<annotated_type, ^^v::contains>)
            {
                constexpr auto expected_contains = std::meta::extract<annotated_type>(annotation);
                if (!std::ranges::contains(expected_contains.value, value))
                {
                    throw std::invalid_argument(std::format("{} is not within expected list: {}", value, expected_contains.value));
                }
            }
        }
    }
}

}; // namespace krrs::argparse
