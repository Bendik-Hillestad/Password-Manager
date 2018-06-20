#include "archive.h"

using std::uint8_t;
using std::uint32_t;

struct bhpm_header
{
    uint8_t  magic[4];
    uint8_t  major_version;
    uint8_t  pad;
    uint8_t  minor_version;
    uint8_t  pad;
};

struct bhpm_entries_header
{
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

struct bhpm_footer
{
    uint32_t crc32;
};

pm::archive pm::read_archive(void* data) noexcept
{
    return {};
}