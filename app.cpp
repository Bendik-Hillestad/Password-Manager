#pragma warning(disable:4996) //Disable useless warning

#include "app.h"
#include "archive.h"
#include "to_base.h"
#include "crypto.h"
#include "memory.h"
#include "span.h"
#include "xorshift.h"

#include <cstdio>
#include <iostream>
#include <filesystem>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace fs = std::filesystem;

namespace pm
{
    enum app_state : pm::state_value_t
    {
        entry,
        setup,
        login,
        main_view,
        create_pass,
        fetch_pass,
        quit
    };

#   define DECLARE_TRANSITION(A, B)     \
    template<>                          \
    struct is_state_transition          \
    <                                   \
        static_cast<state_value_t>(A),  \
        static_cast<state_value_t>(B)   \
    > : std::true_type {}

    DECLARE_TRANSITION(app_state::entry,       app_state::setup);
    DECLARE_TRANSITION(app_state::entry,       app_state::login);
    DECLARE_TRANSITION(app_state::setup,       app_state::main_view);
    DECLARE_TRANSITION(app_state::login,       app_state::main_view);
    DECLARE_TRANSITION(app_state::main_view,   app_state::create_pass);
    DECLARE_TRANSITION(app_state::main_view,   app_state::fetch_pass);
    DECLARE_TRANSITION(app_state::main_view,   app_state::quit);
    DECLARE_TRANSITION(app_state::create_pass, app_state::main_view);
    DECLARE_TRANSITION(app_state::fetch_pass,  app_state::main_view);

#   undef DECLARE_TRANSITION

    template<> bool app::handle<app_state::entry>      () noexcept;
    template<> bool app::handle<app_state::setup>      () noexcept;
    template<> bool app::handle<app_state::login>      () noexcept;
    template<> bool app::handle<app_state::main_view>  () noexcept;
    template<> bool app::handle<app_state::create_pass>() noexcept;
    template<> bool app::handle<app_state::fetch_pass> () noexcept;
    template<> bool app::handle<app_state::quit>       () noexcept;

    static constexpr decltype(auto) ARCHIVE_PATH = "archive.bhpm";

    app::app() noexcept
    {
        //Set the initial state
        this->state = app_state::entry;
    }

    int app::run() noexcept
    {
        //Guard against invalid entry to this function
        if (this->state != app_state::entry) return 0;

        //Initialise our screen
        ui::screen::make_screen(&this->screen);

        //Set our exit flag
        bool exit = false;

        //Run our program loop
        while (!exit)
        {
            switch (this->state)
            {
                case app_state::entry:
                {
                    exit = !this->handle<app_state::entry>();
                } break;

                case app_state::setup:
                {
                    exit = !this->handle<app_state::setup>();
                } break;

                case app_state::login:
                {
                    exit = !this->handle<app_state::login>();
                } break;

                case app_state::main_view:
                {
                    exit = !this->handle<app_state::main_view>();
                } break;

                case app_state::create_pass:
                {
                    exit = !this->handle<app_state::create_pass>();
                } break;

                case app_state::fetch_pass:
                {
                    exit = !this->handle<app_state::fetch_pass>();
                } break;

                case app_state::quit:
                {
                    exit = (this->handle<app_state::quit>(), true);
                } break;
            }
        }

        return 0;
    }

    template<>
    bool app::handle<app_state::entry>() noexcept
    {
        //Clear the screen
        this->screen.clear_screen(ui::color::NAVY);

        //Get the path to our archive file
        auto const path = fs::u8path(ARCHIVE_PATH);

        //Check if it exists
        std::error_code ec;
        if (fs::exists(path, ec))
        {
            //Get the file status
            auto status = fs::status(path, ec);

            //Check that it's a regular file and that we can edit it
            if (fs::is_regular_file(status) && status.permissions() == fs::perms::all)
            {
                //Transition to the login state
                return do_transition<app_state::entry, app_state::login>(&this->state);
            }
            else
            {
                //Abort
                return false;
            }
        }

        //Transition to the setup state
        return do_transition<app_state::entry, app_state::setup>(&this->state);
    }

    template<>
    bool app::handle<app_state::setup>() noexcept
    {
        //Print welcome message
        this->screen.write(ui::color::WHITE,   "**************************",   this->screen.get_width() / 2, 1, ui::align::MIDDLE);
        this->screen.write(ui::color::WHITE, "****** Welcome to BHPM! ******", this->screen.get_width() / 2, 2, ui::align::MIDDLE);
        this->screen.write(ui::color::WHITE,   "**************************",   this->screen.get_width() / 2, 3, ui::align::MIDDLE);
        
        //Prepare to read the master password
        char* pass = nullptr;
        while (true)
        {
            //Write note
            this->screen.write(ui::color::GREY, "Input is hidden.", 35, 13);

            //Ask for the master password
            this->screen.write(ui::color::WHITE, "Enter the master password: ", 35, 14, ui::align::RIGHT);
            auto tmp1 = this->screen.read_hidden();
            this->screen.write(ui::color::LIME, "******");

            //Ask for the master password again
            this->screen.write(ui::color::WHITE, "Retype master password: ", 35, 15, ui::align::RIGHT);
            auto tmp2 = this->screen.read_hidden();
            this->screen.write(ui::color::LIME, "******");

            //Break out if they're equal
            if (std::string_view(tmp1).compare(tmp2) == 0)
            {
                delete[] tmp2;

                pass = tmp1;

                break;
            }

            delete[] tmp1;
            delete[] tmp2;

            //Write error message
            this->screen.write(ui::color::RED, "Passwords did not match!", 35, 17);
        }

        //Create the file
        auto handle = CreateFileA(ARCHIVE_PATH, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, nullptr);

        //Check that it was successful
        if (handle != INVALID_HANDLE_VALUE)
        {
            char test[]{ "Hello" };
            DWORD written;
            WriteFile(handle, test, 5, &written, nullptr);

            CloseHandle(handle);

            //Transition to the main view state
            return do_transition<app_state::setup, app_state::main_view>(&this->state);
        }

        //Abort
        return false;
    }

    template<>
    bool app::handle<app_state::login>() noexcept
    {
        //Print welcome message
        this->screen.write(ui::color::WHITE,   "************************",  this->screen.get_width() / 2, 1, ui::align::MIDDLE);
        this->screen.write(ui::color::WHITE, "****** Welcome back! ******", this->screen.get_width() / 2, 2, ui::align::MIDDLE);
        this->screen.write(ui::color::WHITE,   "************************",  this->screen.get_width() / 2, 3, ui::align::MIDDLE);

        //Ask for the password
        this->screen.write(ui::color::WHITE, "Enter the password: ", 40, 14);
        auto pass = this->screen.read_hidden();

        //Write some stars
        this->screen.write(ui::color::LIME, "******");

        //Transition to the main view state
        return do_transition<app_state::login, app_state::main_view>(&this->state);
    }

    template<>
    bool app::handle<app_state::main_view>() noexcept
    {
        //TODO

        Sleep(10000);

        //Transition to the quit state
        return do_transition<app_state::main_view, app_state::quit>(&this->state);
    }

    template<>
    bool app::handle<app_state::create_pass>() noexcept
    {
        //TODO

        return true;
    }

    template<>
    bool app::handle<app_state::fetch_pass>() noexcept
    {
        //TODO

        return true;
    }

    template<>
    bool app::handle<app_state::quit>() noexcept
    {
        //TODO

        return true;
    }
}
