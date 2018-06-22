#include "archive.h"
#include "crypto.h"

#include <cstring>

using std::uint8_t;
using std::uint32_t;
using std::uint64_t;

#pragma pack(push, 1)
struct bhpm_header
{
    uint8_t  magic[4];
    uint8_t  major_version;
    uint8_t  pad1;
    uint8_t  minor_version;
    uint8_t  pad2;
    uint64_t iv[2];
};

struct bhpm_entries_header
{
    uint64_t hash[4];
    uint32_t entry_count;
    uint32_t entry_size;
};

struct bhpm_entry_header
{
    uint8_t id_len;
    uint8_t pass_len;
};

struct bhpm_entry_footer
{
    uint32_t crc32;
};
#pragma pack(pop)

template<typename T>
static T consume(void** ptr, std::size_t* len)
{
    //Read the value
    auto result = *static_cast<T*>(*ptr);

    //Adjust the pointer and length
    *ptr = static_cast<void*>(static_cast<char*>(*ptr) + sizeof(T));
    *len = *len - sizeof(T);

    //Return the result
    return(result);
}

template<typename T>
static T* consume_array(void** ptr, std::size_t* len, std::size_t count)
{
    //Allocate memory
    auto* result = new T[count];

    //Copy data into the allocated block
    std::memcpy(result, *ptr, sizeof(T) * count);

    //Adjust the pointer and length
    *ptr = static_cast<void*>(static_cast<char*>(*ptr) + sizeof(T) * count);
    *len = *len - sizeof(T) * count;

    //Return the result
    return(result);
}

static constexpr auto FourCC(char const(&magic)[5])
{
    return ((magic[3] << 24) | 
            (magic[2] << 16) | 
            (magic[1] <<  8) | 
            (magic[0] <<  0));
}

static bool check_magic(uint8_t(&magic)[4])
{
    return((*reinterpret_cast<uint32_t*>(magic)) == FourCC("BHPM"));
}

pm::archive pm::read_archive(void* data, std::size_t len) noexcept
{
    //Read the main header
    auto header = consume<bhpm_header>(&data, &len);

    //Verify the header
    if (!check_magic(header.magic)) return {};
    if (header.major_version != 1)  return {};
    if (header.pad1 != 0)           return {};
    if (header.minor_version != 0)  return {};
    if (header.pad2 != 0)           return {};

    //Decrypt the encrypted block
    void* unencrypted_data = nullptr;
    auto  unencrypted_len  = std::size_t{0};
    bool  success = pm::decrypt("1234", header.iv, data, len, &unencrypted_data, &unencrypted_len);
    if (!success) return {};

    //Read the entries header
    auto entries_header = consume<bhpm_entries_header>(&unencrypted_data, &unencrypted_len);

    //Verify that the remaining size is the entries
    if (unencrypted_len != entries_header.entry_size) return {};

    //Read all the entries
    archive result{ entries_header.entry_count, new entry[entries_header.entry_count] };
    for (uint32_t i = 0; i < entries_header.entry_count; i++)
    {
        //Read the entry header
        auto entry_header = consume<bhpm_entry_header>(&unencrypted_data, &unencrypted_len);

        //Read the ID and password
        result.entries[i] =
        {
            consume_array<char>(&unencrypted_data, &unencrypted_len, entry_header.id_len),
            consume_array<char>(&unencrypted_data, &unencrypted_len, entry_header.pass_len)
        };
        
        //Read the entry footer
        auto entry_footer = consume<bhpm_entry_footer>(&unencrypted_data, &unencrypted_len);
    }

    //Make sure we read everything
    if (unencrypted_len != 0) return {};

    //Return the result
    return(result);
}

#include <iostream>
#include <fstream>
#include <limits>

static uint8_t arr[]
{ 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x01,0x00,0x00,0x00,0x0C,0x00,0x00,0x00,0x03,0x03,0x41,0x42,0x43,0x44,0x43,0x45,0xd2,0x04,0x00,0x00
};

void pm::test()
{
    std::ofstream test;
    test.open("test.bhpm", std::ios::binary | std::ios::out);

    test << "BHPM";
    test << (uint8_t)1;
    test << (uint8_t)0;
    test << (uint8_t)0;
    test << (uint8_t)0;

    char iv[16];
    pm::get_random_bytes(iv, 16);
    test.write(iv, 16);

    void* data = nullptr;
    auto datalen = std::size_t{ 0 };
    pm::encrypt("1234", iv, arr, sizeof(arr), &data, &datalen);
    test.write(static_cast<char const*>(data), datalen);

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

    pm::read_archive(data, size);
}