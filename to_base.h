#ifndef PM_TO_BASE_H
#define PM_TO_BASE_H
#pragma once

#include <type_traits>
#include <cassert>
#include <cstdint>
#include <string>
#include <string_view>

namespace pm
{
    //SFINAE helper to make code cleaner
#   define WHERE(x) typename = typename std::enable_if_t<!!(x)>

    namespace detail
    {
        //Helper for detecting non-bool integrals
        template<typename T>
        inline constexpr auto is_nonbool_integral_v = std::is_integral_v<T> && !std::is_same_v<std::remove_cv_t<T>, bool>;
    };

    //Converts an integer to a given base represented by an alphabet.
    template<typename T, WHERE(detail::is_nonbool_integral_v<T>)>
    static std::string to_base(T const &value, std::string_view base) noexcept
    {
        //Handle special case for value = 0
        if (value == 0) return base.at(0);

        //Get the length of the base
        assert(base.length() !=  0);
        assert(base.length() < 256);
        auto base_len = static_cast<std::uint8_t>(base.length());

        //Setup our output string
        std::string result{ "" };

        //Enforce unsigned value
        auto tmp = static_cast<std::make_unsigned_t<T>>(value);

        //Iterate until value is 0
        while (tmp > 0)
        {
            //Extract the digit index and integer divide the value
            auto idx = static_cast<std::uint8_t>(tmp % base_len);
            tmp     /= base_len;

            //Insert the digit
            result += base.at(idx);
        }

        //Return the reversed string
        std::reverse(result.begin(), result.end());
        return result;
    }

#   undef WHERE
};

#endif
