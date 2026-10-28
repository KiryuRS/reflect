// Copyright (c) 2026 KiryuRS
// SPDX-License-Identifier: MIT

#pragma once

#include <meta>
#include <type_traits>

namespace krrs::reflect {

namespace detail {

template <std::meta::info Actual, std::meta::info TmplArg>
consteval bool is_instance_of()
{
    if constexpr (!std::meta::has_template_arguments(Actual) || !std::meta::is_class_template(TmplArg))
    {
        return false;
    }
    else
    {
        constexpr auto arguments = std::define_static_array(std::meta::template_arguments_of(Actual));
        try
        {
            // can't use constexpr here. NTTP causes the entire expression to be non-constant evaluated
            auto tmpl_arg = std::meta::substitute(TmplArg, arguments);
            return std::meta::dealias(Actual) == std::meta::dealias(tmpl_arg);
        }
        catch (const std::meta::exception&)
        {
            // exception could be due to substitution failed.
            // indicates that its not matching - no-op and return false
        }
    }
    return false;
}

} // namespace detail

// answers the question to: "Is the type a template class type? e.g. is T an std::array? or std::vector? or std::unordered_map?"
//
// do not require for user to supply the template arguments / parameters.
// instance_of<T, ^^std::vector>, returns true if T is some form of std::vector (e.g. std::vector<double>, std::vector<std::vector<int>>)
//
// see "test_instance_of" in test_reflection.cpp for more examples
template <typename T, std::meta::info TmplArg, typename RawT = std::remove_cvref_t<T>>
concept instance_of = detail::is_instance_of<^^RawT, TmplArg>();

} // namespace krrs::reflect
