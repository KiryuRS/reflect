// Copyright (c) 2026 KiryuRS
// SPDX-License-Identifier: MIT

#pragma once

#include "../reflect/reflect.hpp"

#include <filesystem>
#include <ranges>
#include <set>
#include <unordered_set>

namespace krrs::argparse::internal {

template <typename T>
struct convert;

template <typename T>
    requires std::integral<T> || std::floating_point<T>
struct convert<T>
{
    static T parse(std::string_view arg)
    {
        T value{};
        std::from_chars(arg.data(), arg.data() + arg.size(), value);
        return value;
    }

    static constexpr std::string_view type_str()
    {
        return std::meta::display_string_of(^^T);
    }
};

// TODO: Convert to flag based?
template <>
struct convert<bool>
{
    static bool parse(std::string_view arg)
    {
        using namespace std::string_view_literals;
        return arg == "true"sv || arg == "True"sv;
    }

    static constexpr std::string_view type_str()
    {
        return "bool";
    }
};

template <typename T>
    requires std::same_as<T, std::string> || std::same_as<T, std::string_view> || std::same_as<T, std::filesystem::path>
struct convert<T>
{
    static T parse(std::string_view arg)
    {
        return T{arg};
    }

    static constexpr std::string_view type_str()
    {
        if constexpr (std::same_as<T, std::string>)
        {
            return "std::string";
        }
        else if constexpr (std::same_as<T, std::string_view>)
        {
            return "std::string_view";
        }
        else if constexpr (std::same_as<T, std::filesystem::path>)
        {
            return "std::filesystem::path";
        }
        else
        {
            static_assert(requires { std::integral_constant<bool, false>::value; }, "type not supported!");
        }
    }
};

template <typename T>
    requires reflect::instance_of<T, ^^std::vector> || reflect::instance_of<T, ^^std::unordered_set> || reflect::instance_of<T, ^^std::set>
struct convert<T>
{
    using value_type = T::value_type;

    static T parse(std::string_view arg)
    {
        using namespace std::string_view_literals;

        return arg
             | std::views::split(","sv)
             | std::views::transform([](auto subrange) {
                 const std::string_view str{subrange};
                 return convert<value_type>::parse(str);
               })
             | std::views::common
             | std::ranges::to<T>();
    }

    static constexpr std::string type_str()
    {
        static constexpr auto tmpl = std::meta::template_of(^^T);
        return std::format("std::{}<{}>", std::meta::identifier_of(tmpl), convert<value_type>::type_str());
    }
};

template <reflect::instance_of<^^std::array> T>
struct convert<T>
{
    static constexpr auto tmpl_args = std::define_static_array(std::meta::template_arguments_of(^^T));
    using value_type = T::value_type;
    static constexpr std::size_t capacity = std::meta::extract<std::size_t>(tmpl_args[1]);

    static T parse(std::string_view arg)
    {
        const auto unbounded = convert<std::vector<value_type>>::parse(arg);
        if (unbounded.size() > capacity)
        {
            throw std::invalid_argument(std::format("[argparse] {} cannot fit into {}", arg, type_str()));
        }

        T obj{};
        std::ranges::copy(unbounded, obj.begin());
        return obj;
    }

    static constexpr std::string type_str()
    {
        static constexpr auto tmpl = std::meta::template_of(^^T);
        return std::format("std::{}<{},{}>", std::meta::identifier_of(tmpl), convert<value_type>::type_str(), capacity);
    }
};

} // namespace krrs::argparse::internal
