#ifndef PM_APP_H
#define PM_APP_H
#pragma once

#include "state_manager.h"
#include "screen.h"

#include <cstdint>

namespace pm
{
    class app
    {
    public:
        app() noexcept;

        int run() noexcept;

    private:
        template<state_value_t state>
        bool handle() noexcept;

        ui::screen screen;
        state_value_t state;
    };
};

#endif
