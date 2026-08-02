// Copyright (c) 2026 KiryuRS
// SPDX-License-Identifier: MIT

#pragma once

#include "../reflect/reflect.hpp"

#include <filesystem>
#include <set>
#include <unordered_set>

namespace krrs {

namespace detail {

struct short_tag
{
};

} // namespace detail

// short-form trait to represent single letter. e.g. --filepath will additionally produce -f
inline constexpr detail::short_tag shortform_trait{};

namespace argparse::concepts {

namespace detail {

// limitation to only allow specific types for parsing
template <typename T>
consteval bool validate_option_types()
{
    static constexpr auto members = reflect::generate_nonstatic_member_metas<T>();
    std::array<bool, std::ranges::size(members)> valid_options;
    std::size_t i = 0;

    template for (constexpr auto member : members)
    {
        constexpr auto type_info = std::meta::type_of(member);
        using type = [:type_info:];
        const bool supported_types =
            std::integral<type> || std::floating_point<type> // built-in types
            || std::same_as<type, std::string> || std::same_as<type, std::string_view> // string types
            || std::same_as<type, std::filesystem::path> // filesystem::path type
            || reflect::instance_of<type, ^^std::vector> || reflect::instance_of<type, ^^std::unordered_set> || reflect::instance_of<type, ^^std::set> // homogeneous dynamic-sized container types
            || reflect::instance_of<type, ^^std::array> // homogeneous fixed-sized container type
        ;
        valid_options[i++] = supported_types;
    }
    return std::ranges::all_of(valid_options, std::identity{});
}

template <std::meta::info Meta, std::meta::info Trait>
consteval bool has_annotated_trait()
{
    static constexpr auto annotations = std::define_static_array(std::meta::annotations_of(Meta));
    return std::ranges::any_of(annotations, [](std::meta::info meta) { return std::meta::type_of(meta) == std::meta::type_of(Trait); });
}

} // namespace detail

template <typename T>
concept type_parsable = requires {
    detail::validate_option_types<T>();
    requires reflect::concepts::krrs_reflectable<T>;
};

template <std::meta::info Meta>
concept shortform_trait = detail::has_annotated_trait<Meta, ^^shortform_trait>();

} // namespace argparse::concepts

} // namespace krrs
