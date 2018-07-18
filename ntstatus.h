#ifndef PM_NTSTATUS_H
#define PM_NTSTATUS_H
#pragma once

#include <cstdint>
#include <system_error>

namespace pm
{
    enum class ntstatus_t : std::int32_t
    {
        UNSUCCESSFUL        = static_cast<std::int32_t>(0xC0000001L),
        INVALID_HANDLE      = static_cast<std::int32_t>(0xC0000008L),
        INVALID_PARAMETER   = static_cast<std::int32_t>(0xC000000DL),
        NO_MEMORY           = static_cast<std::int32_t>(0xC0000017L),
        BUFFER_TOO_SMALL    = static_cast<std::int32_t>(0xC0000023L),
        NOT_SUPPORTED       = static_cast<std::int32_t>(0xC00000BBL),
        INVALID_BUFFER_SIZE = static_cast<std::int32_t>(0xC0000206L),
        NOT_FOUND           = static_cast<std::int32_t>(0xC0000225L),

        SUCCESS             = static_cast<std::int32_t>(0x00000000L)
    };

    std::error_code make_error_code(ntstatus_t) noexcept;
};

namespace std
{
    template<>
    struct is_error_code_enum<pm::ntstatus_t> : true_type {};
};

#endif
