#include "ntstatus.h"

#include <string>

struct ntstatus_category : std::error_category
{
    const char* name() const noexcept final override
    {
        return "ntstatus";
    }

    #define X(e) case pm::ntstatus_t::e: return #e;
    std::string message(int ec) const noexcept final override
    {
        switch (static_cast<pm::ntstatus_t>(ec))
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
            default: return "UNKNOWN_ERROR";
        }
    }
    #undef X
};

ntstatus_category const ntstatus_category_inst{};

std::error_code pm::make_error_code(pm::ntstatus_t status) noexcept
{
    return { static_cast<std::int32_t>(status), ntstatus_category_inst };
}
