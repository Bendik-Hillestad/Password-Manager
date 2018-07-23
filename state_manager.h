#ifndef PM_STATE_MANAGER_H
#define PM_STATE_MANAGER_H
#pragma once

#include <type_traits>
#include <cstdint>

namespace pm
{
    using state_value_t = std::uint16_t;

    template<state_value_t A, state_value_t B>
    struct is_state_transition : std::false_type
    {};

    template<state_value_t A, state_value_t B>
    constexpr inline auto is_state_transition_v = is_state_transition<A, B>::value;

    template<state_value_t A, state_value_t B>
    constexpr bool do_transition(state_value_t* const &state)
    {
        //Compile-time check that it's valid
        static_assert(is_state_transition_v<A, B>, "Invalid state transition!");

        //Run-time check to see if the state correct
        if (*state == A)
        {
            //Change the state
            *state = B;

            //Return success
            return true;
        }
        
        //Do nothing
        return false;
    }
};

#endif
