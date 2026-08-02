#pragma once

#include "concepts.hpp"

namespace krrs::reflect {

namespace detail {

template <concepts::enumerable T>
consteval auto generate_enumerator_metas()
{
    return std::define_static_array(std::meta::enumerators_of(^^T));
}

} // namespace detail

template <concepts::enumerable T>
inline constexpr std::string_view enum_to_string(T e)
{
    static constexpr auto enum_metas = detail::generate_enumerator_metas<T>();

    template for (constexpr auto meta : enum_metas)
    {
        if ([:meta:] == e)
        {
            return std::meta::identifier_of(meta);
        }
    }
    return "UNKNOWN";
}

template <concepts::enumerable T>
inline constexpr T string_to_enum(std::string_view str)
{
    static constexpr auto enum_metas = detail::generate_enumerator_metas<T>();
    template for (constexpr auto meta : enum_metas)
    {
        if (std::meta::identifier_of(meta) == str)
        {
            return [:meta:];
        }
    }
    return T::NONE;
}

} // namespace krrs::reflect
