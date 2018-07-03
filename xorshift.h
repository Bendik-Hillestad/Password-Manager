#ifndef PM_XORSHIFT_H
#define PM_XORSHIFT_H
#pragma once

#include <cstdint>

namespace pm
{
    struct xorshift_state
    {
    public:
        constexpr xorshift_state(std::uint64_t const(&seed)[2]) noexcept
            : state{ seed[0], seed[1] }
        {}

        constexpr std::uint64_t next() noexcept
        {
            //Copy the state
            auto       x = this->state[0];
            auto const y = this->state[1];

            //Update the internal state
            this->state[0] = y;
            x ^= x << 23;
            this->state[1] = x ^ y ^ (x >> 17) ^ (y >> 26);

            //Return the next value
            return this->state[1] + y;
        }

    private:
        std::uint64_t state[2];
    };
};

#endif
