#include "crypto.h"

#include <cstdint>
#include <cstring>
#include <algorithm>
#include <vector>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <wincrypt.h>

#pragma comment(lib, "crypt32.lib")

bool pm::get_random_bytes(void* buffer, std::size_t len)
{
    HCRYPTPROV hProv;

    //Attempt to acquire a context
    auto success = CryptAcquireContext(&hProv, nullptr, MS_DEF_PROV, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT | CRYPT_SILENT);
    if (!success) return false;

    //Generate bytes
    success = CryptGenRandom(hProv, static_cast<DWORD>(len), static_cast<BYTE*>(buffer));
    
    //Release the context and exit
    CryptReleaseContext(hProv, 0);
    return(success);
}

bool pm::encrypt(std::string_view password, void const* iv, void const* input, std::size_t input_len, void** output, std::size_t* output_len)
{
    HCRYPTPROV hProv;
    HCRYPTHASH hHash;
    HCRYPTKEY  hKey;

    //Attempt to acquire a context
    auto success = CryptAcquireContext(&hProv, nullptr, MS_ENH_RSA_AES_PROV, PROV_RSA_AES, CRYPT_VERIFYCONTEXT | CRYPT_SILENT);
    if (!success) return false;

    //Prepare to hash the password using SHA-256
    success = CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash);
    if (!success)
    {
        //Release resources and exit
        CryptReleaseContext(hProv, 0);
        return(success);
    }

    //Hash the password
    auto pass_data = static_cast<void const*>(password.data());
    success = CryptHashData(hHash, static_cast<BYTE const*>(pass_data), static_cast<DWORD>(password.length()), 0);
    if (!success)
    {
        //Release resources and exit
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return(success);
    }

    //Derive a key for AES-128
    success = CryptDeriveKey(hProv, CALG_AES_128, hHash, 0, &hKey);
    if (!success)
    {
        //Release resources and exit
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return(success);
    }

    //Set IV
    success = CryptSetKeyParam(hKey, KP_IV, static_cast<BYTE const*>(iv), 0);
    if (!success)
    {
        //Release resources and exit
        CryptDestroyKey(hKey);
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return(success);
    }

    //Setup block
    constexpr auto block_len = 128u;
    BYTE block[block_len]{};

    //Prepare output
    std::vector<BYTE> output_vector{};

    //Iterate over input data
    for (int i = 0; i < input_len; i += block_len)
    {
        //Copy data into the block
        auto data_len = static_cast<DWORD>(std::min<std::size_t>(block_len, input_len - i));
        std::memcpy(block, input, data_len);

        //Check if this is the final block
        bool is_final = ((input_len - i) <= block_len);

        //Encrypt the block
        success = CryptEncrypt(hKey, NULL, is_final, 0, block, &data_len, block_len);
        if (!success)
        {
            //Release resources and exit
            CryptDestroyKey(hKey);
            CryptDestroyHash(hHash);
            CryptReleaseContext(hProv, 0);
            return(success);
        }

        //Copy the data out
        std::copy(&block[0], &block[data_len], std::back_inserter(output_vector));

        //Clear the block
        std::memset(block, 0, block_len);
    }
    
    //Generate a buffer for the final output
    *output_len = output_vector.size();
    *output     = new BYTE[*output_len];
    
    //Copy the data from the vector into our new buffer
    std::memcpy(*output, output_vector.data(), *output_len);

    //Release resources and exit
    CryptDestroyKey(hKey);
    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);
    return(success);
}

bool pm::decrypt(std::string_view password, void const* iv, void const* input, std::size_t input_len, void** output, std::size_t* output_len)
{
    HCRYPTPROV hProv;
    HCRYPTHASH hHash;
    HCRYPTKEY  hKey;

    //Attempt to acquire a context
    auto success = CryptAcquireContext(&hProv, nullptr, MS_ENH_RSA_AES_PROV, PROV_RSA_AES, CRYPT_VERIFYCONTEXT | CRYPT_SILENT);
    if (!success) return false;

    //Prepare to hash the password using SHA-256
    success = CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash);
    if (!success)
    {
        //Release resources and exit
        CryptReleaseContext(hProv, 0);
        return(success);
    }

    //Hash the password
    auto pass_data = static_cast<void const*>(password.data());
    success = CryptHashData(hHash, static_cast<BYTE const*>(pass_data), static_cast<DWORD>(password.length()), 0);
    if (!success)
    {
        //Release resources and exit
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return(success);
    }

    //Derive a key for AES-128
    success = CryptDeriveKey(hProv, CALG_AES_128, hHash, 0, &hKey);
    if (!success)
    {
        //Release resources and exit
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return(success);
    }

    //Set IV
    success = CryptSetKeyParam(hKey, KP_IV, static_cast<BYTE const*>(iv), 0);
    if (!success)
    {
        //Release resources and exit
        CryptDestroyKey(hKey);
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return(success);
    }

    //Setup block
    constexpr auto block_len = 128u;
    BYTE block[block_len]{};

    //Prepare output
    std::vector<BYTE> output_vector{};

    //Iterate over input data
    for (int i = 0; i < input_len; i += block_len)
    {
        //Copy data into the block
        auto data_len = static_cast<DWORD>(std::min<std::size_t>(block_len, input_len - i));
        std::memcpy(block, input, data_len);

        //Check if this is the final block
        bool is_final = ((input_len - i) <= block_len);

        //Decrypt the block
        success = CryptDecrypt(hKey, NULL, is_final, 0, block, &data_len);
        if (!success)
        {
            //Release resources and exit
            CryptDestroyKey(hKey);
            CryptDestroyHash(hHash);
            CryptReleaseContext(hProv, 0);
            return(success);
        }

        //Copy the data out
        std::copy(&block[0], &block[data_len], std::back_inserter(output_vector));

        //Clear the block
        std::memset(block, 0, block_len);
    }
    
    //Generate a buffer for the final output
    *output_len = output_vector.size();
    *output     = new BYTE[*output_len];
    
    //Copy the data from the vector into our new buffer
    std::memcpy(*output, output_vector.data(), *output_len);

    //Release resources and exit
    CryptDestroyKey(hKey);
    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);
    return(success);
}
