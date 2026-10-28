// Copyright (c) 2026 KiryuRS
// SPDX-License-Identifier: MIT

#pragma once

#include "concepts.hpp"
#include "detail/vtable.hpp"

namespace krrs::inteoc {

template <typename Interface>
class interface_ref : public detail::vtable<Interface>
{
    using interface_t = Interface;
    using base = detail::vtable<Interface>;

public:
    template <typename T, typename NoRefT = std::remove_reference_t<T>>
        requires std::is_lvalue_reference_v<T> && concepts::compliant_with<NoRefT, Interface>
    interface_ref(T&& obj) noexcept
    {
        static constexpr auto vtable_members = std::define_static_array(std::meta::nonstatic_data_members_of(^^base, std::meta::access_context::current()));
        template for (constexpr auto member : vtable_members)
        {
            this->[:member:].bind(obj);
        }
    }
};

} // namespace krrs::inteoc
