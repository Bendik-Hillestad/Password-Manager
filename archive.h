#ifndef PM_ARCHIVE_H
#define PM_ARCHIVE_H
#pragma once

//4 bytes magic ("BHPM" "Bendik Hillestad Password Manager")
//1 byte major version
//1 byte zeroes
//1 byte minor version
//1 byte zeroes
//After this encrypted
//4 bytes entry_count
//4 bytes entry_size
//entries[entry_count]
//    1 byte id_len
//    1 byte pass_len
//    char[id_len] id
//    char[pass_len] pass
//    4 byte CRC32
//4 byte CRC32

#include <cstdint>
#include <cstddef>

namespace pm
{
    struct entry
    {
        char const* identifier;
        char const* password;
    };

    struct archive
    {
        uint32_t entry_count;
        entry* entries;
    };

    archive read_archive(void* data, std::size_t len) noexcept;
    void test();
    void test2();
};

#endif
