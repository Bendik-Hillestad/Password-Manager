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

        std::uint8_t* get_bytes(std::uint32_t* count) noexcept
        {
            //Round up to a multiple of 8 bytes
            *count = ((*count) + (sizeof(std::uint64_t) - 1)) & ~(sizeof(std::uint64_t) - 1);

            //Allocate the buffer
            auto* buf = new std::uint8_t[*count];
            auto* p   = reinterpret_cast<std::uint64_t*>(buf);

            //Fill the buffer
            for (std::uint32_t i = 0; i < (*count) / sizeof(std::uint64_t); i++)
            {
                p[i] = this->next();
            }

            //Return the buffer
            return buf;
        }

    private:
        std::uint64_t state[2];
    };
};

#endif
