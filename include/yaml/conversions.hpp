// Copyright (c) 2026 KiryuRS
// SPDX-License-Identifier: MIT

#pragma once

#include "../reflect/reflect.hpp"

#include <yaml-cpp/yaml.h>

#include <filesystem>
#include <optional>
#include <sstream>
#include <unordered_set>

namespace YAML {

template <>
struct convert<std::filesystem::path>
{
    static Node encode(const std::filesystem::path& obj)
    {
        Node node{NodeType::Scalar};
        node = obj.string();
        return node;
    }

    static bool decode(const Node& node, std::filesystem::path& obj)
    {
        if (!node.IsScalar())
            return false;

        obj = node.template as<std::string>();
        return true;
    }
};

template <typename T>
struct convert<std::unordered_set<T>>
{
    static Node encode(const std::unordered_set<T>& obj)
    {
        Node node{NodeType::Sequence};
        for (const auto& elem : obj)
            node.push_back(elem);
        return node;
    }

    static bool decode(const Node& node, std::unordered_set<T>& obj)
    {
        if (!node.IsSequence())
            return false;

        obj.clear();
        for (const auto& elem : node)
            obj.insert(elem.template as<T>());
        return obj;
    }
};

template <typename T>
struct convert<std::optional<T>>
{
    static Node encode(const std::optional<T>& obj)
    {
        Node node{};
        if (obj.has_value())
        {
            node = obj.value();
        }
        return node;
    }

    static bool decode(const Node& node, std::optional<T>& obj)
    {
        if (!node.IsDefined())
            return true;

        obj = node.template as<T>();
        return true;
    }
};

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
            if constexpr (std::meta::has_default_member_initializer(member) || ::krrs::reflect::instance_of<type, ^^std::optional>)
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
