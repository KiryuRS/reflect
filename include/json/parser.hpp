#pragma once

#include "conversions.hpp"

namespace krrs::json {

template <typename T>
    requires reflect::concepts::krrs_reflectable<T>
std::string serialize(const T& obj)
{
    return internal::convert<T>::encode(obj);
}

} // namespace krrs::json
