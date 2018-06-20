#ifndef TMP_HELPER_H
#define TMP_HELPER_H
#pragma once

#include <type_traits>

namespace pm::detail
{
    //Helper for detecting non-bool integrals
    template<typename T>
    inline constexpr auto is_nonbool_integral_v = std::is_integral_v<T> && !std::is_same_v<std::remove_cv_t<T>, bool>;
};

//SFINAE helper to make code cleaner
#define WHERE(x) typename = typename std::enable_if_t<!!(x)>

#endif
