#ifndef PM_CRYPTO_H
#define PM_CRYPTO_H
#pragma once

#include <cstddef>
#include <string_view>

namespace pm
{
    //Fills a buffer with random bytes using a CSPRNG
    bool get_random_bytes(void* buffer, std::size_t len);

    //Encrypts a buffer with AES-128 using the provided password
    bool encrypt(std::string_view password, void const* input, std::size_t input_len, void** output, std::size_t* output_len);

    //Decrypts a buffer with AES-128 using the provided password
    bool decrypt(std::string_view password, void const* input, std::size_t input_len, void** output, std::size_t* output_len);
};

#endif
