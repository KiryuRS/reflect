// Copyright (c) 2026 KiryuRS
// SPDX-License-Identifier: MIT

#pragma once

#include "regular_mem_funcs.hpp"

#include <meta>
#include <type_traits>
#include <utility>

namespace krrs::inteoc::detail {

template <std::meta::info Function, typename Object, bool NoExcept, typename R, typename ... Args>
struct descriptor_base
{
    static constexpr bool is_const = std::is_const_v<Object>;
    static constexpr bool is_noexcept = NoExcept;
    static constexpr std::meta::info info = Function;

    Object* object;
    R (*thunk)(Object*, Args...) noexcept(NoExcept);

    template <typename T>
    void bind(T& obj) noexcept
    {
        object = &obj;
        thunk = [](Object* self, Args&& ... args) noexcept(NoExcept) -> R {
            using self_type = std::conditional_t<is_const, const T, T>;
            constexpr auto func = find_matching_function<std::remove_const_t<T>>(Function);
            return static_cast<self_type*>(self)->[:func:](std::forward<Args>(args)...);
        };
    }

    R operator()(Args... args) const noexcept(NoExcept)
    {
        return thunk(object, std::forward<Args>(args)...);
    }
};

template <std::meta::info Function, typename Signature = [:std::meta::type_of(Function):] >
struct descriptor;

template <std::meta::info Function, typename R, typename ... Args, bool NoExcept>
struct descriptor<Function, R(Args...) noexcept(NoExcept)> : descriptor_base<Function, void, NoExcept, R, Args...>
{
};

template <std::meta::info Function, typename R, typename ... Args, bool NoExcept>
struct descriptor<Function, R(Args...) const noexcept(NoExcept)> : descriptor_base<Function, const void, NoExcept, R, Args...>
{
};

template <typename Interface>
struct vtable_of
{
    struct type;
    consteval {
        std::vector<std::meta::info> members;
        static constexpr auto member_functions = regular_member_functions<Interface>();
        template for (constexpr auto f : member_functions)
        {
            members.push_back(std::meta::data_member_spec(std::meta::substitute(^^descriptor, {std::meta::reflect_constant(f)}),
                                                          {.name = std::meta::identifier_of(f)}));
        }
        std::meta::define_aggregate(^^type, members);
    }
};

template <typename Interface>
using vtable = typename vtable_of<Interface>::type;

} // namespace krrs::inteoc::detail
