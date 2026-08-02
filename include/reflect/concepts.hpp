#pragma once

#include <algorithm>
#include <meta>

namespace krrs::reflect {

namespace detail {

struct reflect_tag
{
};

} // namespace detail

// for annotating a struct. e.g.
// struct [[=krrs::reflect::trait]] my_type { ... };
// not a good place for this variable. but it will do for now ...
inline constexpr detail::reflect_tag trait{};

namespace concepts {

template <typename T>
concept has_reflect_tag = [] {
    constexpr auto all_annotations = std::define_static_array(std::meta::annotations_of(^^T));
    return std::ranges::any_of(all_annotations, [](std::meta::info meta) { return std::meta::type_of(meta) == std::meta::type_of(^^trait); });
}();

template <typename T>
concept enumerable = requires {
    std::is_enum_v<T>;
    std::meta::is_enumerator(^^T);
    { T::NONE } -> std::same_as<T>;
};

template <typename T>
concept reflectable = requires {
    std::is_class_v<T>;
    !std::is_reflection_v<T>;
    std::is_aggregate_v<T>;
};

template <typename T>
concept krrs_reflectable = reflectable<T> && has_reflect_tag<T>;

} // namespace concepts

} // namespace krrs::reflect
