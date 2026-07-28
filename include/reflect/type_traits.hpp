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

template <typename T, std::meta::info TmplArg>
concept instance_of = detail::is_instance_of<^^T, TmplArg>();

} // namespace krrs::reflect
