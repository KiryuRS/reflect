// Copyright (c) 2026 KiryuRS
// SPDX-License-Identifier: MIT

#pragma once

#include "conversions.hpp"

namespace krrs::yaml {

template <::krrs::reflect::concepts::krrs_reflectable T>
T deserialize(const std::string& str)
{
    static constexpr std::string_view name = std::meta::identifier_of(^^T);
    const YAML::Node node = YAML::Load(str);
    return node[name].as<T>();
}

template <::krrs::reflect::concepts::krrs_reflectable T>
std::string serialize(const T& obj)
{
    static constexpr std::string_view name = std::meta::identifier_of(^^T);
    YAML::Node node;
    node[name] = obj;

    std::ostringstream oss;
    oss << node;
    return oss.str();
}

} // namespace krrs::yaml
