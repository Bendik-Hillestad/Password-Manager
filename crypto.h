#ifndef PM_CRYPTO_H
#define PM_CRYPTO_H
#pragma once

#include "span.h"

#include <cstdint>
#include <cstddef>

namespace pm
{
    //Fills a buffer with random bytes using a CSPRNG
    bool get_random_bytes(std::uint8_t* buffer, std::size_t len) noexcept;

    //Fills a buffer with random bytes using a CSPRNG
    template<std::size_t N>
    bool get_random_bytes(std::uint8_t(&buffer)[N]) noexcept
    {
        return get_random_bytes(buffer, N);
    }

    //Calculates the SHA-256 hash of the input data
    bool hash(span<std::uint8_t> data, uint8_t** result) noexcept;

    //Encrypts the input with AES-128 using the provided password and initialization vector
    bool encrypt(span<std::uint8_t> input, span<std::uint8_t> password, span<std::uint8_t> iv, uint8_t** output, std::size_t* output_len) noexcept;

    //Decrypts the input with AES-128 using the provided password and initialization vector
    bool decrypt(span<std::uint8_t> input, span<std::uint8_t> password, span<std::uint8_t> iv, uint8_t** output, std::size_t* output_len) noexcept;
};

#endif
