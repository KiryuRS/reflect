// Copyright (c) 2026 KiryuRS
// SPDX-License-Identifier: MIT

#pragma once

#include "detail/regular_mem_funcs.hpp"

#include <algorithm>
#include <concepts>

namespace krrs::inteoc::concepts {

template <typename T, typename Interface>
concept compliant_with = std::ranges::none_of(detail::regular_member_functions<Interface>(),
                                              [] (std::meta::info f) {
                                                  return detail::find_matching_function<T>(f) == std::meta::info{};
                                              });

} // namespace krrs::inteoc::concepts
