#ifndef PM_CRYPTO_H
#define PM_CRYPTO_H
#pragma once

#include "ntstatus.h"
#include "span.h"

#include <cstdint>
#include <cstddef>
#include <memory>

namespace pm
{
    using owned_byte_array = std::unique_ptr<std::uint8_t[]>;

    //Fills a buffer with random bytes using a CSPRNG
    ntstatus_t get_random_bytes(std::uint8_t* buffer, std::size_t len) noexcept;

    //Fills a buffer with random bytes using a CSPRNG
    template<std::size_t N>
    ntstatus_t get_random_bytes(std::uint8_t(&buffer)[N]) noexcept
    {
        return get_random_bytes(buffer, N);
    }

    //Calculates the SHA-256 hash of the input data
    ntstatus_t hash(span<std::uint8_t> data, owned_byte_array* result) noexcept;

    //Encrypts the input with AES-128 using the provided password and initialization vector
    ntstatus_t encrypt(span<std::uint8_t> input, span<std::uint8_t> password, span<std::uint8_t> iv, owned_byte_array* output, std::size_t* output_len) noexcept;

    //Decrypts the input with AES-128 using the provided password and initialization vector
    ntstatus_t decrypt(span<std::uint8_t> input, span<std::uint8_t> password, span<std::uint8_t> iv, owned_byte_array* output, std::size_t* output_len) noexcept;
};

#endif
