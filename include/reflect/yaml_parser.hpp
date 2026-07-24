#pragma once

#include "reflect.hpp"

#include <yaml-cpp/yaml.h>

#include <sstream>

namespace YAML {

template <typename T>
    requires ::krrs::reflect::concepts::reflectable<T> && ::krrs::reflect::concepts::has_reflect_tag<T>
struct convert<T>
{
    static Node encode(const T& obj)
    {
        static constexpr auto members = nsdms();

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
        static constexpr auto members = nsdms();
        template for (constexpr auto member : members)
        {
            constexpr std::string_view name = std::meta::identifier_of(member);
            using type = [:std::meta::type_of(member):];

            // default value represents optional "argument" in yaml
            if constexpr (std::meta::has_default_member_initializer(member))
            {
                if (!node[name])
                    continue;
            }

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
        return true;
    }

    // used internally
    static consteval auto nsdms()
    {
        return std::define_static_array(std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::current()));
    }
};

} // namespace YAML

namespace krrs::parser::yaml {

template <typename T>
    requires ::krrs::reflect::concepts::reflectable<T> && ::krrs::reflect::concepts::has_reflect_tag<T>
T deserialize(const std::string& str)
{
    static constexpr std::string_view name = std::meta::identifier_of(^^T);
    const YAML::Node node = YAML::Load(str);
    return node[name].as<T>();
}

template <typename T>
    requires ::krrs::reflect::concepts::reflectable<T> && ::krrs::reflect::concepts::has_reflect_tag<T>
std::string serialize(const T& obj)
{
    static constexpr std::string_view name = std::meta::identifier_of(^^T);
    YAML::Node node;
    node[name] = obj;

    std::ostringstream oss;
    oss << node;
    return oss.str();
}

} // namespace krrs::parser::yaml
