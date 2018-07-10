#include "crypto.h"

#include <cstdint>
#include <cstring>
#include <vector>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <bcrypt.h>

#pragma comment(lib, "bcrypt.lib")

pm::ntstatus_t pm::get_random_bytes(std::uint8_t* buffer, std::size_t len) noexcept
{
    NTSTATUS          success = static_cast<NTSTATUS>(ntstatus_t::UNSUCCESSFUL);
    BCRYPT_ALG_HANDLE hRngAlg = nullptr;

    //Open algorithm provider
    success = BCryptOpenAlgorithmProvider(&hRngAlg, BCRYPT_RNG_ALGORITHM, nullptr, 0);
    if (success < 0)
    {
        //Go to cleanup
        goto cleanup;
    }

    //Generate random bytes
    success = BCryptGenRandom(hRngAlg, static_cast<PUCHAR>(buffer), static_cast<ULONG>(len), 0);

cleanup:
    //Close algorithm provider
    if (hRngAlg)
        BCryptCloseAlgorithmProvider(hRngAlg, 0);

    //Return the status
    return static_cast<ntstatus_t>(success);
}

pm::ntstatus_t pm::hash(pm::span<std::uint8_t> data, pm::owned_byte_array* result) noexcept
{
    NTSTATUS           success      = static_cast<NTSTATUS>(ntstatus_t::UNSUCCESSFUL);
    BCRYPT_ALG_HANDLE  hShaAlg      = nullptr;
    BCRYPT_HASH_HANDLE hHash        = nullptr;
    DWORD              cbData       = 0;
    DWORD              cbHashObject = 0;
    DWORD              cbHash       = 0;
    PBYTE              pbHashObject = nullptr;
    PBYTE              pbHash       = nullptr;

    //Open algorithm provider
    success = BCryptOpenAlgorithmProvider(&hShaAlg, BCRYPT_SHA256_ALGORITHM, nullptr, 0);
    if (success < 0)
    {
        //Go to cleanup
        goto cleanup;
    }

    //Find how much space the hash object needs
    success = BCryptGetProperty(hShaAlg, BCRYPT_OBJECT_LENGTH, reinterpret_cast<PBYTE>(&cbHashObject), sizeof(DWORD), &cbData, 0);
    if (success < 0)
    {
        //Go to cleanup
        goto cleanup;
    }

    //Allocate space for the hash object
    pbHashObject = static_cast<PBYTE>(HeapAlloc(GetProcessHeap(), 0, cbHashObject));
    if (pbHashObject == nullptr)
    {
        //Go to cleanup
        goto cleanup;
    }

    //Find how much space the hash result needs
    success = BCryptGetProperty(hShaAlg, BCRYPT_HASH_LENGTH, reinterpret_cast<PBYTE>(&cbHash), sizeof(DWORD), &cbData, 0);
    if (success < 0)
    {
        //Go to cleanup
        goto cleanup;
    }

    //Allocate space for the hash result
    pbHash = static_cast<PBYTE>(HeapAlloc(GetProcessHeap(), 0, cbHash));
    if (pbHash == nullptr)
    {
        //Go to cleanup
        goto cleanup;
    }

    //Prepare to hash the data
    success = BCryptCreateHash(hShaAlg, &hHash, pbHashObject, cbHashObject, nullptr, 0, 0);
    if (success < 0)
    {
        //Go to cleanup
        goto cleanup;
    }

    //Hash the data
    success = BCryptHashData(hHash, static_cast<PUCHAR>(const_cast<std::uint8_t*>(data.data())), static_cast<ULONG>(data.size()), 0);
    if (success < 0)
    {
        //Go to cleanup
        goto cleanup;
    }

    //Finish the hash
    success = BCryptFinishHash(hHash, pbHash, cbHash, 0);
    if (success < 0)
    {
        //Go to cleanup
        goto cleanup;
    }

    //Assign the result
    *result = std::make_unique<std::uint8_t[]>(cbHash);
    std::memcpy(result->get(), pbHash, cbHash);

cleanup:
    //Close algorithm provider
    if (hShaAlg)
        BCryptCloseAlgorithmProvider(hShaAlg, 0);

    //Destroy the hash
    if (hHash)
        BCryptDestroyHash(hHash);

    //Release the memory for the hash object
    if (pbHashObject)
        HeapFree(GetProcessHeap(), 0, pbHashObject);

    //Release the memory for the result
    if (pbHash)
        HeapFree(GetProcessHeap(), 0, pbHash);

    //Return the status
    return static_cast<ntstatus_t>(success);
}

pm::ntstatus_t pm::encrypt(pm::span<std::uint8_t> input, pm::span<std::uint8_t> password, pm::span<std::uint8_t> iv, pm::owned_byte_array* output, std::size_t* output_len) noexcept
{
    NTSTATUS           success      = static_cast<NTSTATUS>(ntstatus_t::UNSUCCESSFUL);
    BCRYPT_ALG_HANDLE  hAesAlg      = nullptr;
    BCRYPT_KEY_HANDLE  hKey         = nullptr;
    DWORD              cbData       = 0;
    DWORD              cbKeyObject  = 0;
    DWORD const        cbHash       = 32;
    DWORD const        cbIV         = 16;
    DWORD              cbCipherText = 0;
    PBYTE              pbKeyObject  = nullptr;
    owned_byte_array   pbHash       = nullptr;
    PBYTE              pbIV         = nullptr;
    PBYTE              pbCipherText = nullptr;

    //Open algorithm provider
    success = BCryptOpenAlgorithmProvider(&hAesAlg, BCRYPT_AES_ALGORITHM, nullptr, 0);
    if (success < 0)
    {
        //Go to cleanup
        goto cleanup;
    }

    //Find how much space the key object needs
    success = BCryptGetProperty(hAesAlg, BCRYPT_OBJECT_LENGTH, reinterpret_cast<PBYTE>(&cbKeyObject), sizeof(DWORD), &cbData, 0);
    if (success < 0)
    {
        //Go to cleanup
        goto cleanup;
    }

    //Allocate space for the key object
    pbKeyObject = static_cast<PBYTE>(HeapAlloc(GetProcessHeap(), 0, cbKeyObject));
    if (pbKeyObject == nullptr)
    {
        //Go to cleanup
        goto cleanup;
    }

    //Check that the provided IV is long enough
    if (iv.size() < cbIV)
    {
        //Go to cleanup
        goto cleanup;
    }

    //Allocate space for the IV
    pbIV = static_cast<PBYTE>(HeapAlloc(GetProcessHeap(), 0, cbIV));
    if (pbIV == nullptr)
    {
        //Go to cleanup
        goto cleanup;
    }

    //Copy the provided IV into our new buffer
    std::memcpy(pbIV, iv.data(), cbIV);

    //Hash the input password
    success = static_cast<NTSTATUS>(pm::hash(password, &pbHash));
    if (success < 0)
    {
        //Go to cleanup
        goto cleanup;
    }

    //Generate the key object
    success = BCryptGenerateSymmetricKey(hAesAlg, &hKey, pbKeyObject, cbKeyObject, pbHash.get(), cbHash, 0);
    if (success < 0)
    {
        //Go to cleanup
        goto cleanup;
    }

    //Calculate the length of the cipher text
    success = BCryptEncrypt(hKey, const_cast<PUCHAR>(input.data()), static_cast<ULONG>(input.size()), nullptr, pbIV, cbIV, nullptr, 0, &cbCipherText, 0);
    if (success < 0)
    {
        //Go to cleanup
        goto cleanup;
    }

    //Allocate space for the cipher text
    pbCipherText = static_cast<PBYTE>(HeapAlloc(GetProcessHeap(), 0, cbCipherText));
    if (pbCipherText == nullptr)
    {
        //Go to cleanup
        goto cleanup;
    }

    //Encrypt the input
    success = BCryptEncrypt(hKey, const_cast<PUCHAR>(input.data()), static_cast<ULONG>(input.size()), nullptr, pbIV, cbIV, pbCipherText, cbCipherText, &cbData, 0);
    if (success < 0)
    {
        //Go to cleanup
        goto cleanup;
    }

    //Assign the result
    *output     = std::make_unique<std::uint8_t[]>(cbCipherText);
    *output_len = cbCipherText;
    std::memcpy(output->get(), pbCipherText, cbCipherText);

cleanup:
    //Close algorithm provider
    if (hAesAlg)
        BCryptCloseAlgorithmProvider(hAesAlg, 0);

    //Destroy the key
    if (hKey)
        BCryptDestroyKey(hKey);

    //Release the memory for the key object
    if (pbKeyObject)
        HeapFree(GetProcessHeap(), 0, pbKeyObject);

    //Release the memory for the IV
    if (pbIV)
        HeapFree(GetProcessHeap(), 0, pbIV);

    //Release the memory for the cipher text
    if (pbCipherText)
        HeapFree(GetProcessHeap(), 0, pbCipherText);

    //Return the status
    return static_cast<ntstatus_t>(success);
}

pm::ntstatus_t pm::decrypt(pm::span<std::uint8_t> input, pm::span<std::uint8_t> password, pm::span<std::uint8_t> iv, pm::owned_byte_array* output, std::size_t* output_len) noexcept
{
    NTSTATUS           success      = static_cast<NTSTATUS>(ntstatus_t::UNSUCCESSFUL);
    BCRYPT_ALG_HANDLE  hAesAlg      = nullptr;
    BCRYPT_KEY_HANDLE  hKey         = nullptr;
    DWORD              cbData       = 0;
    DWORD              cbKeyObject  = 0;
    DWORD const        cbHash       = 32;
    DWORD const        cbIV         = 16;
    DWORD              cbClearText  = 0;
    PBYTE              pbKeyObject  = nullptr;
    owned_byte_array   pbHash       = nullptr;
    PBYTE              pbIV         = nullptr;
    PBYTE              pbClearText  = nullptr;

    //Open algorithm provider
    success = BCryptOpenAlgorithmProvider(&hAesAlg, BCRYPT_AES_ALGORITHM, nullptr, 0);
    if (success < 0)
    {
        //Go to cleanup
        goto cleanup;
    }

    //Find how much space the key object needs
    success = BCryptGetProperty(hAesAlg, BCRYPT_OBJECT_LENGTH, reinterpret_cast<PBYTE>(&cbKeyObject), sizeof(DWORD), &cbData, 0);
    if (success < 0)
    {
        //Go to cleanup
        goto cleanup;
    }

    //Allocate space for the key object
    pbKeyObject = static_cast<PBYTE>(HeapAlloc(GetProcessHeap(), 0, cbKeyObject));
    if (pbKeyObject == nullptr)
    {
        //Go to cleanup
        goto cleanup;
    }

    //Check that the provided IV is long enough
    if (iv.size() < cbIV)
    {
        //Go to cleanup
        goto cleanup;
    }

    //Allocate space for the IV
    pbIV = static_cast<PBYTE>(HeapAlloc(GetProcessHeap(), 0, cbIV));
    if (pbIV == nullptr)
    {
        //Go to cleanup
        goto cleanup;
    }

    //Copy the provided IV into our new buffer
    std::memcpy(pbIV, iv.data(), cbIV);

    //Hash the input password
    success = static_cast<NTSTATUS>(pm::hash(password, &pbHash));
    if (success < 0)
    {
        //Go to cleanup
        goto cleanup;
    }

    //Generate the key object
    success = BCryptGenerateSymmetricKey(hAesAlg, &hKey, pbKeyObject, cbKeyObject, pbHash.get(), cbHash, 0);
    if (success < 0)
    {
        //Go to cleanup
        goto cleanup;
    }

    //Calculate the length of the clear text
    success = BCryptDecrypt(hKey, const_cast<PUCHAR>(input.data()), static_cast<ULONG>(input.size()), nullptr, pbIV, cbIV, nullptr, 0, &cbClearText, 0);
    if (success < 0)
    {
        //Go to cleanup
        goto cleanup;
    }

    //Allocate space for the clear text
    pbClearText = static_cast<PBYTE>(HeapAlloc(GetProcessHeap(), 0, cbClearText));
    if (pbClearText == nullptr)
    {
        //Go to cleanup
        goto cleanup;
    }

    //Decrypt the input
    success = BCryptDecrypt(hKey, const_cast<PUCHAR>(input.data()), static_cast<ULONG>(input.size()), nullptr, pbIV, cbIV, pbClearText, cbClearText, &cbData, 0);
    if (success < 0)
    {
        //Go to cleanup
        goto cleanup;
    }

    //Assign the result
    *output     = std::make_unique<std::uint8_t[]>(cbClearText);
    *output_len = cbClearText;
    std::memcpy(output->get(), pbClearText, cbClearText);

cleanup:
    //Close algorithm provider
    if (hAesAlg)
        BCryptCloseAlgorithmProvider(hAesAlg, 0);

    //Destroy the key
    if (hKey)
        BCryptDestroyKey(hKey);

    //Release the memory for the key object
    if (pbKeyObject)
        HeapFree(GetProcessHeap(), 0, pbKeyObject);

    //Release the memory for the IV
    if (pbIV)
        HeapFree(GetProcessHeap(), 0, pbIV);

    //Release the memory for the clear text
    if (pbClearText)
        HeapFree(GetProcessHeap(), 0, pbClearText);

    //Return the status
    return static_cast<ntstatus_t>(success);
}
