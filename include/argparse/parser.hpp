#pragma once

#include "conversions.hpp"

#include <expected>
#include <iomanip>
#include <unordered_map>

namespace krrs::argparse {

namespace detail {

struct short_tag
{
};

// limitation to only allow specific types for parsing
template <typename T>
consteval bool validate_option_types()
{
    static constexpr auto members = reflect::generate_nonstatic_member_metas<T>();
    std::array<bool, std::ranges::size(members)> valid_options;
    int i = 0;

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

template <typename T>
concept type_parsable = requires {
    validate_option_types<T>();
    requires reflect::concepts::krrs_reflectable<T>;
};

template <std::meta::info Meta, std::meta::info Trait>
consteval bool has_annotated_trait()
{
    static constexpr auto annotations = std::define_static_array(std::meta::annotations_of(Meta));
    return std::ranges::any_of(annotations, [](std::meta::info meta) { return std::meta::type_of(meta) == std::meta::type_of(Trait); });
}

inline auto generate_arguments(int argc, const char* argv[])
{
    using namespace std::string_view_literals;

    // okay to be std::string_view because cmd arguments are already stored in memory
    using arguments_type = std::unordered_map<std::string_view, std::optional<std::string_view>>;
    arguments_type map;
    for (int i = 1; i < argc; i += 2)
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
        static constexpr short_tag sf_trait{};
        constexpr auto name = std::meta::identifier_of(Member);

        std::string str;
        if constexpr (has_annotated_trait<Member, ^^sf_trait>())
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

// short-form trait. e.g. [[=krrs::argparse::sf_trait]]
inline constexpr detail::short_tag sf_trait{};

template <detail::type_parsable T>
constexpr std::expected<T, std::string> parse_args(int argc, const char* argv[])
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
        constexpr auto name = detail::has_annotated_trait<member, ^^sf_trait>()
            ? std::meta::identifier_of(member).substr(0, 1)
            : std::meta::identifier_of(member);
        using type = [:std::meta::type_of(member):];

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
