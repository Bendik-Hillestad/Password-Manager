#pragma once

#include <cstdint>
#include <string_view>

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

    #define X(e) case ntstatus_t::e: return #e;
    static constexpr std::string_view to_string(ntstatus_t status) noexcept
    {
        switch (status)
        {
            X(UNSUCCESSFUL)
            X(INVALID_HANDLE)
            X(INVALID_PARAMETER)
            X(NO_MEMORY)
            X(BUFFER_TOO_SMALL)
            X(NOT_SUPPORTED)
            X(INVALID_BUFFER_SIZE)
            X(NOT_FOUND)
            X(SUCCESS)
            default: return "Unknown status";
        }
    }
    #undef CASE
};