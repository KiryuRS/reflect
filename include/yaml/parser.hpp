// Copyright (c) 2026 KiryuRS
// SPDX-License-Identifier: MIT

#pragma once

#include "conversions.hpp"

namespace krrs::yaml {

template <::krrs::reflect::concepts::krrs_reflectable T, bool BeginWithIdentifierOfT = true>
T deserialize(const auto& str)
{
    constexpr auto load_as_string = [] (const auto& str) {
        using RawStringT = std::remove_cvref_t<decltype(str)>;
        if constexpr (std::same_as<RawStringT, std::string>)
        {
            return YAML::Load(str);
        }
        else if constexpr (std::same_as<RawStringT, std::string_view>)
        {
            return YAML::Load(std::string{str});
        }
        else
        {
            static_assert(requires { std::integral_constant<bool, false>::value; }, "type not supported!");
        }
    };

    static constexpr std::string_view name = std::meta::identifier_of(^^T);
    const YAML::Node node = load_as_string(str);
    if constexpr (BeginWithIdentifierOfT)
    {
        return node[name].as<T>();
    }
    else
    {
        return node.as<T>();
    }
}

template <::krrs::reflect::concepts::krrs_reflectable T, bool BeginWithIdentifierOfT = true>
std::string serialize(const T& obj)
{
    static constexpr std::string_view name = std::meta::identifier_of(^^T);
    YAML::Node node;
    if constexpr (BeginWithIdentifierOfT)
    {
        node[name] = obj;
    }
    else
    {
        node = obj;
    }

    std::ostringstream oss;
    oss << node;
    return oss.str();
}

} // namespace krrs::yaml
