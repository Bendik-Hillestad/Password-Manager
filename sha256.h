#ifndef PM_SHA256_H
#define PM_SHA256_h
#pragma once

/*
 * Implements SHA-256 as described in FIPS PUB 180-4 (August 2015).
 */

#include <array>
#include <cstddef>
#include <cstdint>

namespace pm::security
{
    struct sha256
    {
        static constexpr std::size_t   const block_length       = 512 / 8;
        static constexpr std::size_t   const digest_length      = 256 / 8;
        static constexpr std::uint64_t const max_message_length = 0x2000000000000000ull;

        using byte = std::uint8_t;
        using word = std::uint32_t;

        using result_type = std::array<byte, digest_length>;
        using state_type  = std::array<word, digest_length / sizeof(word)>;

        /* 
         * (Optional) Pads the message according to the specification,
         * ensuring that its size is a multiple of the block length
         * and that the length is encoded in the final block.
         * Note: Allocates new memory.
         */
        static byte* pad(byte const* data, std::uint64_t* size) noexcept;

        using context = struct sha256_context
        {
        public:
            /*
             * Prepares or resets the context. Must be called
             * before computing the hash of a message.
             */
            void init() noexcept;

            /* 
             * Feeds a single block to the SHA-256 transform,
             * updating the intermediate hash value of the
             * message. Must be called for each block length
             * sized chunk of the message.
             */
            void update (byte const* data) noexcept;

            /* 
             * Retrieves the message digest.
             */
            result_type get_digest() noexcept;   

        private:
            state_type state;
        };

        sha256() = delete;
    };
};

#endif
