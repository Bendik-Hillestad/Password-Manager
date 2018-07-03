#ifndef PM_ARCHIVE_H
#define PM_ARCHIVE_H
#pragma once

#include "span.h"

#include <cstdint>
#include <vector>

namespace pm
{
    struct entry
    {
        span<char> identifier;
        span<char> password;
    };

	std::vector<entry> read_archive(span<std::uint8_t> data) noexcept;
	
    void test();
    void test2();
};

#endif
