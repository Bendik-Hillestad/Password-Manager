#pragma warning(disable:4996) //Disable useless warning

#include "archive.h"
#include "crypto.h"
#include "xorshift.h"

#include <algorithm>
#include <cstdint>
#include <cstring>

using std::uint8_t;
using std::uint32_t;
using std::uint64_t;

#pragma pack(push, 1)
struct bhpm_header
{
    uint8_t  magic[8];
    uint8_t  major_version;
    uint8_t  pad1;
    uint8_t  minor_version;
    uint8_t  pad2;
    uint64_t iv[2];
};

struct bhpm_data_hash
{
    uint64_t hash[4];
};

struct bhpm_entry_header
{
    uint16_t id_len   : 6;
    uint16_t pass_len : 6;
    uint16_t garbage  : 4;
};

struct bhpm_xorshift_seed
{
    uint64_t seed[2];
};
#pragma pack(pop)

template<typename T>
static T consume(pm::span<uint8_t>* data)
{
    //Read the value
    auto result = *reinterpret_cast<T const*>(data->data());

    //Adjust the span
    *data = data->slice(sizeof(T));

    //Return the result
    return result;
}

template<typename T>
static T* consume_array(pm::span<uint8_t>* data, std::size_t count)
{
    //Allocate memory
    auto* result = new T[count];

    //Copy data into the allocated block
    std::memcpy(result, data->data(), sizeof(T) * count);

    //Adjust the span
    *data = data->slice(sizeof(T) * count);

    //Return the result
    return result;
}
 
static pm::span<uint8_t> xorshift_data(pm::span<uint8_t> input, pm::xorshift_state* xs) noexcept
{
    //Make a copy of the data
    auto* copy = new uint8_t[input.size()];
    std::copy(input.cbegin(), input.cend(), copy);

    //Reinterpret as an array of u64
    auto* arr = reinterpret_cast<uint64_t*>(copy);
    auto size = input.size() / sizeof(uint64_t);

    //Iterate over the array and xor it
    for (decltype(size) i = 0; i < size; i++)
        arr[i] ^= xs->next();

    //Reinterpret back as an array of u8
    copy = reinterpret_cast<uint8_t*>(arr);

    //Return in a span object
    return { copy, input.size() };
}

static constexpr uint32_t FourCC(char const(&magic)[5])
{
    return ((magic[3] << 24) | 
            (magic[2] << 16) | 
            (magic[1] <<  8) | 
            (magic[0] <<  0));
}

static bool check_magic(uint8_t(&magic)[8])
{
    return(((*reinterpret_cast<uint32_t*>(magic))     == FourCC("BHPM")) &&
           ((*reinterpret_cast<uint32_t*>(&magic[4])) == 0x11223344UL));
}

std::vector<pm::entry> pm::read_archive(span<std::uint8_t> data) noexcept
{
    auto password = "1234";

    //Read the main header
    auto header = consume<bhpm_header>(&data);

    //Verify the header
    if (!check_magic(header.magic)) return {};
    if (header.major_version != 1)  return {};
    if (header.pad1 != 0)           return {};
    if (header.minor_version != 0)  return {};
    if (header.pad2 != 0)           return {};

    //Decrypt the encrypted block
    auto  unencrypted_data = pm::owned_byte_array{ nullptr };
    auto  unencrypted_len  = std::size_t{ 0 };
    auto  success = pm::decrypt(pm::span<uint8_t>{ data }, pm::span<uint8_t>{ reinterpret_cast<uint8_t*>(const_cast<char*>(password)), 4 }, span<uint8_t>{reinterpret_cast<uint8_t*>(header.iv), 16}, &unencrypted_data, &unencrypted_len);
    if (success != ntstatus_t::SUCCESS) return {};

    //Wrap in a span
    data = pm::span<std::uint8_t>{ unencrypted_data.get(), static_cast<std::ptrdiff_t>(unencrypted_len) };

    //Read the xorshift seed
    auto seed_span = data.slice(data.size() - sizeof(bhpm_xorshift_seed));
    auto xs_state = pm::xorshift_state{ consume<bhpm_xorshift_seed>(&seed_span).seed };

    //Xorshift the data
    data = xorshift_data(data.slice(0, 48), &xs_state);

    //Read the hash
    auto hash = consume<bhpm_data_hash>(&data);

    //Read all the entries
    auto result = std::vector<pm::entry>{};
    auto is_final = false;
    while (!is_final)
    {
        //Read the entry header
        auto entry_header = consume<bhpm_entry_header>(&data);

        //Set the final flag
        is_final = (entry_header.id_len == 0) && (entry_header.pass_len == 0);

        //Check that we're not done
        if (!is_final)
        {
            //Read the ID and password 
            auto* id   = consume_array<char>(&data, entry_header.id_len);
            auto* pass = consume_array<char>(&data, entry_header.pass_len);

            //Push values into vector
            result.push_back(pm::entry
            {
                pm::span<char>{id,   entry_header.id_len},
                pm::span<char>{pass, entry_header.pass_len},
            });
        }
    }

    //Return the result
    return result;
}

#include <iostream>
#include <fstream>
#include <limits>

static uint8_t arr[]
{ 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, //HASH
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, //
    0xC3, 0xA0, 0x41, 0x42, 0x43, 0x44, 0x43, 0x45,                                                 //DATA
    0x00, 0x00,                                                                                     //END MARKER
    0x8A, 0x24, 0xAE, 0xD0, 0x45, 0xD8,                                                             //PADDING
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, //XORSHIFT SEED
};

void pm::test()
{
    auto password = "1234";

    std::ofstream test;
    test.open("test.bhpm", std::ios::binary | std::ios::out);

    test << "BHPM";
    uint32_t magic = 0x11223344;
    test.write(reinterpret_cast<char const*>(&magic), sizeof(uint32_t));
    test << (uint8_t)1;
    test << (uint8_t)0;
    test << (uint8_t)0;
    test << (uint8_t)0;

    uint8_t iv[16];
    auto err = pm::get_random_bytes(iv);
    test.write(reinterpret_cast<char*>(iv), 16);

    decltype(arr) arr2{};
    pm::xorshift_state xs_state{ {0x0807060504030201ULL, 0x1615141312111009ULL} };
    auto arrspan = pm::span<uint8_t>{ arr };
    xorshift_data(arrspan.slice(0, 48), &xs_state).copy_to(arr2, sizeof(arr2));
    arrspan.slice(48).copy_to(&arr2[48], sizeof(arr) - 48);

    auto data    = pm::owned_byte_array{ nullptr };;
    auto datalen = std::size_t{ 0 };
    err = pm::encrypt(span<uint8_t>{ arr2 }, pm::span<uint8_t>{ reinterpret_cast<uint8_t*>(const_cast<char*>(password)), 4 }, span<uint8_t>{iv}, &data, &datalen);
    test.write(reinterpret_cast<char*>(data.get()), datalen);

    test.flush();
    test.close();
}

void pm::test2()
{
    std::ifstream test;
    test.open("test.bhpm", std::ios::binary | std::ios::in);

    test.ignore(std::numeric_limits<std::streamsize>::max());
    auto size = test.gcount();
    test.clear();
    test.seekg(0, std::ios::beg);

    auto* data = new char[static_cast<std::size_t>(size)];
    test.read(data, size);
    test.close();

    pm::read_archive(span<std::uint8_t>{ reinterpret_cast<std::uint8_t*>(data), size });
}
