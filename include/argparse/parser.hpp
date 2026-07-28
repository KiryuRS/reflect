#pragma once

#include "conversions.hpp"

#include <expected>
#include <unordered_map>

namespace krrs::argparse {

namespace detail {

struct short_tag
{
};
struct optional_tag
{
};
struct default_tag
{
};

// okay to be std::string_view because cmd arguments are already stored in memory
using arguments_map = std::unordered_map<std::string_view, std::optional<std::string_view>>;

template <std::meta::info Meta, auto Trait>
consteval bool has_annotated_trait()
{
    using trait_type = decltype(Trait);
    static constexpr auto annotations = std::define_static_array(std::meta::annotations_of(Meta));

    if constexpr (std::ranges::empty(annotations))
    {
        return false;
    }
    else
    {
        return std::ranges::any_of(annotations, [](std::meta::info meta) { return std::meta::type_of(meta) == std::meta::type_of(^^trait_type); });
    }
}

inline arguments_map generate_arguments(int argc, char* argv[])
{
    using namespace std::string_view_literals;

    arguments_map map;
    for (int i = 1; i < argc; i += 2)
    {
        auto key_view = std::string_view{argv[i]} | std::views::drop_while([](char c) { return c == '-'; });
        const std::string_view key{key_view};
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

} // namespace detail

// short-form trait. e.g. [[=krrs::argparse::sf_trait]]
inline constexpr detail::short_tag sf_trait{};
// optional trait argument. e.g. [[=krrs::argparse::opt_trait]]
inline constexpr detail::optional_tag opt_trait{};
// has a default argument. e.g. [[=krrs::argparse::def_trait]]
inline constexpr detail::default_tag def_trait{};

template <typename T>
    requires reflect::concepts::krrs_reflectable<T>
constexpr std::expected<T, std::string> parse_args(int argc, const char* argv[])
{
    using namespace std::string_view_literals;
    static constexpr auto members = reflect::generate_nonstatic_member_metas<T>();

    if (argc <= 1)
    {
        throw std::invalid_argument("[argparse] no arguments supplied!");
    }

    const auto arguments = detail::generate_arguments(argc, argv);
    if (arguments.contains("help"sv))
    {
        // TODO: Display help
        return std::unexpected{"help?"};
    }

    T parsed{};

    template for (constexpr auto member : members)
    {
        constexpr auto name = std::meta::identifier_of(member);
        using type = [:std::meta::type_of(member):];

        constexpr bool is_optional = detail::has_annotated_trait<member, opt_trait>;
        constexpr bool has_default = detail::has_annotated_trait<member, def_trait>;

        const auto iter = std::ranges::find_if(arguments)

        // const auto iter = std::ranges::find_if(arguments, [name, &annotations] (std::string_view arg) {
        //     const bool found = arg.starts_with("--") && arg.substr(2) == name;

        //     // short-form trait (e.g. -c, -f)
        //     if constexpr (detail::has_annotated_trait<member, sf_trait>)
        //     {
        //         const bool short_found = arg.starts_with("-") && arg.substr(1) == name.substr(0, 1);
        //         return short_found || found;
        //     }
        //     else
        //     {
        //         return found;
        //     }
        // });

        // // ensures that the corresponding value is legitimate
        // // e.g. --config --hostname  <- should be rejected (pairing argument not supplied)
        // //      --config   <- should be rejected (no pairing argument, reached end)
        // if (iter != arguments.end() &&
        //     iter + 1 != arguments.end() &&
        //     (!(iter + 1)->starts_with("--") || !(iter + 1)->starts_with("-")))
        // {
        //     const std::string_view value = *(iter + 1);
        //     if (const auto result = convert<type>::parse_value(value); result.has_value())
        //     {
        //         parsed.[:member:] = result.value();
        //     }
        //     else
        //     {
        //         throw std::invalid_argument(
        //             std::format("[argparse] failed to parse {} with error: {}", name, result.error());
        //         );
        //     }
        // }
        // else if (is_optional || has_default)
        // {
        //     // no-op
        // }
        // else
        // {
        //     throw std::invalid_argument(
        //         std::format("[argparse] {} is required but not supplied!", name);
        //     );
        // }
    }

    return parsed;
}

} // namespace krrs::argparse
