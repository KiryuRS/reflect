// Copyright (c) 2026 KiryuRS
// SPDX-License-Identifier: MIT

#pragma once

#include "concepts.hpp"
#include "conversions.hpp"

#include <expected>
#include <iomanip>
#include <unordered_map>

namespace krrs::argparse {

namespace detail {

inline auto generate_arguments(std::integral auto argc, const char* argv[])
{
    using namespace std::string_view_literals;

    // okay to be std::string_view because cmd arguments are already stored in memory
    using arguments_type = std::unordered_map<std::string_view, std::optional<std::string_view>>;
    arguments_type map;
    for (decltype(argc) i = 1; i < argc; i += 2)
    {
        auto key_view = std::string_view{argv[i]} | std::views::drop_while([](char c) { return c == '-'; });
        const std::string_view key{key_view};
        // as long as we see "h" or "help" - simply return the "help" map
        // no reason to generate the rest as its not going to be parsed
        if (key == "h"sv || key == "help"sv)
        {
            return arguments_type{{"help"sv, std::nullopt}};
        }

        map[key] = std::nullopt;

        // peek at the next pair and check if we have a matching value
        if (i + 1 < argc)
        {
            const std::string_view maybe_value{argv[i + 1]};
            if (!maybe_value.starts_with("--"sv) && !maybe_value.starts_with("-"sv))
            {
                map[key] = maybe_value;
            }
        }
    }
    return map;
}

template <typename T>
std::vector<std::string> generate_argument_helpers()
{
    const auto print_key_option = []<std::meta::info Member>() -> std::string {
        constexpr auto name = std::meta::identifier_of(Member);

        std::string str;
        if constexpr (concepts::shortform_trait<Member>)
        {
            str += std::format("-{}, ", name.substr(0, 1));
        }
        str += std::format("--{}", name);
        return str;
    };

    static constexpr auto members = reflect::generate_nonstatic_member_metas<T>();
    const T obj{};

    std::vector<std::string> arg_helpers;
    arg_helpers.reserve(std::ranges::size(members) + 1);

    arg_helpers.push_back("--help");
    template for (constexpr auto member : members)
    {
        using type = [:std::meta::type_of(member):];
        std::ostringstream oss;
        oss << std::setw(20) << std::left << print_key_option.template operator()<member>() << std::format("<{}>", internal::convert<type>::type_str());

        if constexpr (std::meta::has_default_member_initializer(member))
        {
            oss << std::format("  [default: '{}']", obj.[:member:]);
        }
        arg_helpers.push_back(oss.str());
    }
    return arg_helpers;
}

} // namespace detail

template <concepts::type_parsable T>
constexpr std::expected<T, std::string> parse_args(std::integral auto argc, const char* argv[])
{
    using namespace std::string_view_literals;
    static constexpr auto members = reflect::generate_nonstatic_member_metas<T>();

    const auto arguments = detail::generate_arguments(argc, argv);
    if (arguments.contains("help"sv))
    {
        const auto arg_helpers = detail::generate_argument_helpers<T>();
        std::ostringstream oss;
        oss << "Usage: " << argv[0] << " [OPTIONS]\nOptions:\n    ";
        const char* delimiter = "";
        for (const auto& arg_helper : arg_helpers)
        {
            oss << std::exchange(delimiter, "\n    ") << arg_helper;
        }
        return std::unexpected{oss.str()};
    }

    std::vector<std::string_view> missing_options;
    missing_options.reserve(std::ranges::size(members));

    T parsed{};
    template for (constexpr auto member : members)
    {
        constexpr auto name = concepts::shortform_trait<member>
            ? std::meta::identifier_of(member).substr(0, 1)
            : std::meta::identifier_of(member);
        using type = [:std::meta::type_of(member):];

        // reject parsing if:
        // 1. argument name not found and has no default initializer
        // 2. argument name found, but no matching "value" (e.g. --number_of_threads --filepath "/opt/gcc/15")
        if (!arguments.contains(name))
        {
            if constexpr (!std::meta::has_default_member_initializer(member))
            {
                missing_options.push_back(std::meta::identifier_of(member));
            }
            continue;
        }

        const auto optional_arg = arguments.at(name);
        if (!optional_arg.has_value())
        {
            missing_options.push_back(std::meta::identifier_of(member));
            continue;
        }

        std::string_view arg_str = optional_arg.value();
        parsed.[:member:] = internal::convert<type>::parse(arg_str);
    }

    if (!missing_options.empty())
    {
        throw std::invalid_argument(std::format("[argparse] missing required arguments: {}", missing_options));
    }
    return parsed;
}

} // namespace krrs::argparse
