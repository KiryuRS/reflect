// Copyright (c) 2026 KiryuRS
// SPDX-License-Identifier: MIT

#include "../include/reflect/functions.hpp"

#include <gtest/gtest.h>

namespace mocks {

int combine(int, double, char);

struct vec2
{
    int x;
    int y;
};

vec2 operator+(const vec2&, const vec2&);

struct incrementer
{
    int operator()(int) const;
};

template <typename T>
T identity(T);

struct template_scaler
{
    template <typename T, typename U>
    T operator()(T, U) const;
};

} // namespace mocks

namespace tests {

using namespace ::testing;

TEST(test_functions, free_function)
{
    {
        constexpr auto traits = krrs::reflect::generate_function_meta<^^mocks::combine>();
        static_assert(traits.type == krrs::reflect::function_type::FUNCTION);

        using return_type = [: traits.return_type :];
        static_assert(std::same_as<return_type, int>);

        static_assert(traits.parameters.size() == 3);
        using param0_type = [: traits.parameters[0] :];
        using param1_type = [: traits.parameters[1] :];
        using param2_type = [: traits.parameters[2] :];
        static_assert(std::same_as<param0_type, int>);
        static_assert(std::same_as<param1_type, double>);
        static_assert(std::same_as<param2_type, char>);

        using signature = [: traits.signature_type :];
        static_assert(std::same_as<signature, int(int, double, char)>);
    }

    {
        constexpr auto traits = krrs::reflect::generate_function_meta<^^mocks::operator+>();
        static_assert(traits.type == krrs::reflect::function_type::OPERATOR_FUNCTION);

        using return_type = [: traits.return_type :];
        static_assert(std::same_as<return_type, mocks::vec2>);

        static_assert(traits.parameters.size() == 2);
        using param0_type = [: traits.parameters[0] :];
        using param1_type = [: traits.parameters[1] :];
        static_assert(std::same_as<param0_type, const mocks::vec2&>);
        static_assert(std::same_as<param1_type, const mocks::vec2&>);

        using signature = [: traits.signature_type :];
        static_assert(std::same_as<signature, mocks::vec2(const mocks::vec2&, const mocks::vec2&)>);
    }
}

TEST(test_functions, functor)
{
    constexpr auto traits = krrs::reflect::generate_function_meta<^^mocks::incrementer>();
    static_assert(traits.type == krrs::reflect::function_type::OPERATOR_FUNCTION);

    using return_type = [: traits.return_type :];
    static_assert(std::same_as<return_type, int>);

    static_assert(traits.parameters.size() == 1);
    using param0_type = [: traits.parameters[0] :];
    static_assert(std::same_as<param0_type, int>);

    using signature = [: traits.signature_type :];
    static_assert(std::same_as<signature, int(int) const>);
}

// a lambda's call operator surfaces as "operator()", same as a functor
TEST(test_functions, lambda)
{
    // regular lambda
    constexpr auto answer = [] { return 42; };
    using closure_type = decltype(answer);

    constexpr auto traits = krrs::reflect::generate_function_meta<^^closure_type>();
    static_assert(traits.type == krrs::reflect::function_type::LAMBDA_FUNCTION);

    using return_type = [: traits.return_type :];
    static_assert(std::same_as<return_type, int>);

    static_assert(traits.parameters.size() == 0);

    using signature = [: traits.signature_type :];
    static_assert(std::same_as<signature, int() const>);


    // templated lambda
    constexpr auto answer_2 = [] <typename T>(T) { return 100.0; };
    using closure_type_2 = decltype(answer_2);

    constexpr auto traits_2 = krrs::reflect::generate_function_meta<^^closure_type_2, long>();
    static_assert(traits_2.type == krrs::reflect::function_type::LAMBDA_FUNCTION_TEMPLATE);

    using return_type_2 = [: traits_2.return_type :];
    static_assert(std::same_as<return_type_2, double>);

    static_assert(traits_2.parameters.size() == 1);
    using param0 = [: traits_2.parameters[0] :];
    static_assert(std::same_as<param0, long>);

    static_assert(traits_2.template_parameters.size() == 1);
    using tmpl_param0 = [: traits_2.template_parameters[0] :];
    static_assert(std::same_as<tmpl_param0, long>);

    using signature_2 = [: traits_2.signature_type :];
    static_assert(std::same_as<signature_2, double(long) const>);
}

TEST(test_functions, template_free_function)
{
    {
        constexpr auto traits = krrs::reflect::generate_function_meta<^^mocks::identity, int>();
        static_assert(traits.type == krrs::reflect::function_type::FUNCTION_TEMPLATE);

        using return_type = [: traits.return_type :];
        static_assert(std::same_as<return_type, int>);

        static_assert(traits.parameters.size() == 1);
        using param0_type = [: traits.parameters[0] :];
        static_assert(std::same_as<param0_type, int>);

        static_assert(traits.template_parameters.size() == 1);
        using template_arg0 = [: traits.template_parameters[0] :];
        static_assert(std::same_as<template_arg0, int>);
    }

    // same template, different instantiation
    {
        constexpr auto traits = krrs::reflect::generate_function_meta<^^mocks::identity, double>();
        static_assert(traits.type == krrs::reflect::function_type::FUNCTION_TEMPLATE);

        using return_type = [: traits.return_type :];
        static_assert(std::same_as<return_type, double>);

        static_assert(traits.template_parameters.size() == 1);
        using template_arg0 = [: traits.template_parameters[0] :];
        static_assert(std::same_as<template_arg0, double>);
    }
}

TEST(test_functions, template_functor)
{
    constexpr auto traits = krrs::reflect::generate_function_meta<^^mocks::template_scaler, int, double>();
    static_assert(traits.type == krrs::reflect::function_type::OPERATOR_FUNCTION_TEMPLATE);

    using return_type = [: traits.return_type :];
    static_assert(std::same_as<return_type, int>);

    static_assert(traits.parameters.size() == 2);
    using param0_type = [: traits.parameters[0] :];
    using param1_type = [: traits.parameters[1] :];
    static_assert(std::same_as<param0_type, int>);
    static_assert(std::same_as<param1_type, double>);

    static_assert(traits.template_parameters.size() == 2);
    using template_arg0 = [: traits.template_parameters[0] :];
    using template_arg1 = [: traits.template_parameters[1] :];
    static_assert(std::same_as<template_arg0, int>);
    static_assert(std::same_as<template_arg1, double>);
}

} // namespace tests
