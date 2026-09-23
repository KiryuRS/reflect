#pragma once

#include <concepts>
#include <meta>
#include <ranges>

namespace krrs::reflect {

namespace detail {

template <std::meta::info Meta>
consteval auto retrieve_all_operator_functions()
{
    static constexpr auto all_members = std::define_static_array(std::meta::members_of(Meta, std::meta::access_context::current()));
    // special member functions:
    // - operator=(const T&)
    // - operator=(T&&)
    auto op_funcs_view = all_members | std::views::filter([] (std::meta::info m) {
        return !std::meta::is_special_member_function(m)
            && (std::meta::is_operator_function(m) || std::meta::is_operator_function_template(m));
    });
    return std::define_static_array(std::vector<std::meta::info>{std::from_range, op_funcs_view});
}

consteval bool is_class_type(std::meta::info m) noexcept
{
    try
    {
        return std::meta::is_class_type(m);
    }
    catch (const std::meta::exception&)
    {

    }
    return false;
}

consteval bool is_closure_like(std::meta::info m)
{
    // closure type (e.g. lambdas) is unnamed, hence std::meta::parent_of() reflects an unnamed class.
    // whereas a named functor (e.g. struct S { void operator()(); }) reflects a named class.
    //
    // this function is triggered whenever something is an operator function, which includes the following:
    //
    // struct V { int v; };
    // bool operator+(V, V);  <-- NOTE: std::meta::has_identifier() would return false!
    //
    // so we need to ensure that the function exists inside a class
    const auto parent = std::meta::parent_of(m);
    return std::meta::is_class_member(m) && !std::meta::has_identifier(parent);
}

template <std::meta::info Meta>
concept same_as_function = std::meta::is_function(Meta) || std::meta::is_operator_function(Meta);

template <std::meta::info Meta>
concept same_as_template_function = std::meta::is_function_template(Meta) || std::meta::is_operator_function_template(Meta);

} // namespace detail

enum class function_type
{
    FUNCTION,
    OPERATOR_FUNCTION,
    LAMBDA_FUNCTION,
    FUNCTION_TEMPLATE,
    OPERATOR_FUNCTION_TEMPLATE,
    LAMBDA_FUNCTION_TEMPLATE,
};

// non-template functions
template <std::meta::info Func>
    requires detail::same_as_function<Func>
consteval auto generate_function_meta()
{
    // well, you could create a regular struct here, but showing the alternative approach
    struct function_traits;
    consteval
    {
        std::meta::info members[]
        {
            std::meta::data_member_spec(^^std::meta::info, {.name = "signature_type"}),
            std::meta::data_member_spec(^^std::meta::info, {.name = "return_type"}),
            std::meta::data_member_spec(^^std::span<const std::meta::info>, {.name = "parameters"}),
            std::meta::data_member_spec(^^function_type, {.name = "type"}),
        };

        std::meta::define_aggregate(^^function_traits, members);
    }

    constexpr auto parameters = std::define_static_array(
        std::meta::parameters_of(Func)
        | std::views::transform([] (std::meta::info meta) {
            return std::meta::type_of(meta);
        })
    );

    // normalize "signature_type" so that slice operator is only required to access the type
    if constexpr (std::meta::is_operator_function(Func))
    {
        constexpr bool is_lambda = detail::is_closure_like(Func);
        return function_traits{.signature_type = std::meta::type_of(Func),
                                .return_type = std::meta::return_type_of(Func),
                                .parameters = parameters,
                                .type = is_lambda ? function_type::LAMBDA_FUNCTION : function_type::OPERATOR_FUNCTION};
    }
    else
    {

        return function_traits{.signature_type = std::meta::type_of(Func),
                                .return_type = std::meta::return_type_of(Func),
                                .parameters = parameters,
                                .type = function_type::FUNCTION};
    }
}

// for templated functions - needs to be instantiated with template arguments
template <std::meta::info Func, typename ... TmplArgs>
    requires (!detail::same_as_function<Func> && detail::same_as_template_function<Func>)
consteval auto generate_function_meta()
{
    static_assert(std::meta::can_substitute(Func, {^^TmplArgs...}), "Wrong number of template arguments supplied!");

    struct function_traits;
    consteval
    {
        std::meta::info members[]
        {
            std::meta::data_member_spec(^^std::meta::info, {.name = "signature_type"}),
            std::meta::data_member_spec(^^std::meta::info, {.name = "return_type"}),
            std::meta::data_member_spec(^^std::span<const std::meta::info>, {.name = "parameters"}),
            std::meta::data_member_spec(^^std::span<const std::meta::info>, {.name = "template_parameters"}),
            std::meta::data_member_spec(^^function_type, {.name = "type"}),
        };

        std::meta::define_aggregate(^^function_traits, members);
    }

    if constexpr (std::meta::is_operator_function_template(Func))
    {
        constexpr bool is_lambda = detail::is_closure_like(Func);

        constexpr auto instantiated = std::meta::substitute(Func, {^^TmplArgs...});
        const auto fn_traits = generate_function_meta<instantiated>();

        return function_traits{.signature_type = fn_traits.signature_type,
                               .return_type = fn_traits.return_type,
                               .parameters = fn_traits.parameters,
                               .template_parameters = std::define_static_array(std::meta::template_arguments_of(instantiated)),
                               .type = is_lambda ? function_type::LAMBDA_FUNCTION_TEMPLATE : function_type::OPERATOR_FUNCTION_TEMPLATE};
    }
    else if constexpr (std::meta::is_function_template(Func))
    {
        constexpr auto instantiated = std::meta::substitute(Func, {^^TmplArgs...});
        const auto fn_traits = generate_function_meta<instantiated>();

        return function_traits{.signature_type = fn_traits.signature_type,
                               .return_type = fn_traits.return_type,
                               .parameters = fn_traits.parameters,
                               .template_parameters = std::define_static_array(std::meta::template_arguments_of(instantiated)),
                               .type = function_type::FUNCTION_TEMPLATE};
    }
    else
    {
        static_assert(requires { requires std::integral_constant<bool, false>::value; }, "not supported!");
    }
}

// struct S1
// {
//     template <typename T>
//     void operator()(T);
// }
// generate_function_meta<^^S1::operator(), int>
//
// the above may invoke an internal compilation error where the mangling + resolve is different. instead of explicitly state ::operator(),
// pass in the entire class and extract out the operator() function from the compiler.
// NOTE: trade-off here is that there can only be ONE operator()
template <std::meta::info T, typename ... TmplArgs>
    requires (detail::is_class_type(T))
consteval auto generate_function_meta()
{
    static constexpr auto operator_functions = detail::retrieve_all_operator_functions<T>();
    static_assert(std::ranges::size(operator_functions) == 1, "type does not have any non-special operator functions?");

    constexpr auto op_func = operator_functions[0];
    constexpr bool is_operator_function = std::meta::is_operator_function_template(op_func) || std::meta::is_operator_function(op_func);
    static_assert(is_operator_function, "no operator (template) function found?");

    return generate_function_meta<op_func, TmplArgs...>();
}

} // namespace krrs::reflect
