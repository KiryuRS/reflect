#pragma once

#include "../reflect/reflect.hpp"

#include <concepts>
#include <ranges>
#include <stdexcept>
#include <vector>

namespace krrs::argparse::internal {

template <typename T>
struct convert;

template <typename T>
    requires std::integral<T> || std::floating_point<T>
struct convert<T>
{
    static T parse(std::string_view key, const std::optional<std::string_view>& arg)
    {
        // requires to have a value
        if (!arg.has_value())
        {
            throw std::invalid_argument(std::format("[argparse] {} does not have any value!", key));
        }

        T val;
        std::istringstream iss{arg.value()};
        if (!iss >> val)
        {
            constexpr auto name = std::meta::identifier_of(^^T);
            throw std::invalid_argument(std::format("[argparse] failed to convert {} to type {}", arg.value(), name));
        }
    }
};

template <>
struct convert<bool>
{
    static bool parse(std::string_view /* key */, const std::optional<std::string_view>& arg)
    {
        // no value means its a flag enabler = return true
        if (!arg.has_value())
        {
            return true;
        }

        const std::string lower
            = arg.value() | std::views::transform([](char c) { return static_cast<char>(std::tolower(c)); }) | std::ranges::to<std::string>();
        return lower == "true";
    }
};

template <typename T>
    requires std::same_as<T, std::string> || std::same_as<T, std::string_view>
struct convert<T>
{
    static T parse(std::string_view key, const std::optional<std::string_view>& arg)
    {
        // requires to have a value
        if (!arg.has_value())
        {
            throw std::invalid_argument(std::format("[argparse] {} does not have any value!", key));
        }

        return T{arg.value()};
    }
};

template <reflect::instance_of<^^std::vector> T>
struct convert<T>
{
    static T parse(std::string_view key, const std::optional<std::string_view>& arg)
    {
        // requires to have a value
        if (!arg.has_value())
        {
            throw std::invalid_argument(std::format("[argparse] {} does not have any value!", key));
        }

        using value_type = T::value_type;
        using namespace std::string_view_literals;

        return arg | std::views::split(","sv) | std::views::transform([](auto subrange) {
                   const std::string_view str{subrange};
                   return convert<value_type>::parse(str);
               })
               | std::ranges::to<std::vector<std::string>>();
    }
};

} // namespace krrs::argparse::internal
