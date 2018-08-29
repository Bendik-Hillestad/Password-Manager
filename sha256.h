#ifndef PM_SHA256_H
#define PM_SHA256_h
#pragma once

/*
 * Implements SHA-256 as described in FIPS PUB 180-4 (August 2015).
 */

#include <array>
#include <cstddef>
#include <cstdint>
#include <tuple>

namespace pm::security
{
    struct sha256
    {
        static constexpr std::size_t   const block_length       = 512 / 8;
        static constexpr std::size_t   const digest_length      = 256 / 8;
        static constexpr std::uint64_t const max_message_length = 0x2000000000000000ull;

        using byte = std::uint8_t;
        using word = std::uint32_t;

        /*
         * Computes the SHA-256 hash of a data string.
         */
        static byte* compute_hash(byte const* data, std::uint64_t data_length) noexcept;

        /*
         * A low-level hashing primitive.
         */
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
            void update(byte const* data) noexcept;

            /*
             * Pads and embeds the message length into the
             * final block and proceeds with feeding the
             * resulting block(s) to the SHA-256 transform.
             */
            void update_final
            (
                byte const* data,
                std::uint64_t data_length,
                std::uint64_t message_length
            ) noexcept;

            /* 
             * Retrieves the message digest.
             */
            byte* get_digest() noexcept;

        private:
            std::array<word, digest_length / sizeof(word)> state;
        };

        sha256() = delete;

    private:
        static std::tuple<byte*, std::uint64_t> pad_message
        (
            byte const* data,
            std::uint64_t data_length,
            std::uint64_t message_length
        ) noexcept;
    };
};

#endif
