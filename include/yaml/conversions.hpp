// Copyright (c) 2026 KiryuRS
// SPDX-License-Identifier: MIT

#pragma once

#include "../reflect/reflect.hpp"

#include <yaml-cpp/yaml.h>

#include <sstream>

namespace YAML {

template <typename T>
    requires ::krrs::reflect::concepts::reflectable<T> && ::krrs::reflect::concepts::has_reflect_tag<T>
struct convert<T>
{
    static Node encode(const T& obj)
    {
        static constexpr auto members = ::krrs::reflect::generate_nonstatic_member_metas<T>();

        Node node{};
        template for (constexpr auto member : members)
        {
            constexpr std::string_view name = std::meta::identifier_of(member);
            using type = [:std::meta::type_of(member):];

            if constexpr (::krrs::reflect::concepts::enumerable<type>)
            {
                node[name] = ::krrs::reflect::enum_to_string(obj.[:member:]);
            }
            else
            {
                node[name] = obj.[:member:];
            }
        }
        return node;
    }

    static bool decode(const Node& node, T& obj)
    {
        static constexpr auto members = ::krrs::reflect::generate_nonstatic_member_metas<T>();
        std::vector<std::string_view> missing_values;
        missing_values.reserve(std::ranges::size(members));

        template for (constexpr auto member : members)
        {
            constexpr std::string_view name = std::meta::identifier_of(member);
            using type = [:std::meta::type_of(member):];

            // default value represents optional "argument" in yaml
            if constexpr (std::meta::has_default_member_initializer(member))
            {
                if (!node[name].IsDefined())
                {
                    continue;
                }
            }

            if (node[name].IsDefined())
            {
                if constexpr (::krrs::reflect::concepts::enumerable<type>)
                {
                    const std::string str = node[name].template as<std::string>();
                    obj.[:member:] = ::krrs::reflect::string_to_enum<type>(str);
                }
                else
                {
                    obj.[:member:] = node[name].template as<type>();
                }
            }
            else
            {
                missing_values.push_back(name);
            }
        }

        if (!missing_values.empty())
        {
            throw std::runtime_error(std::format("[yaml] missing the required keys: {} for {}", missing_values, std::meta::identifier_of(^^T)));
        }

        return true;
    }
};

} // namespace YAML
