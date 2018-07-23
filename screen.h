#ifndef PM_SCREEN_H
#define PM_SCREEN_H
#pragma once

#include <cstdint>
#include <string_view>

namespace pm::ui
{
    enum class color : std::uint8_t
    {
        BLACK,
        MAROON,
        GREEN,
        NAVY,
        OLIVE,
        PURPLE,
        TEAL,
        GREY,
        RED,
        LIME,
        BLUE,
        YELLOW,
        MAGENTA,
        CYAN,
        WHITE
    };

    enum class align : std::uint8_t
    {
        LEFT,
        MIDDLE,
        RIGHT
    };

    using buffer_handle = void*;

    struct screen
    {
    public:
        screen() noexcept : valid{ false } {}

        static bool make_screen(screen* const &dst) noexcept;

        void  clear_screen(color clearColor) noexcept;
        void  paint       (color paintColor, std::int16_t x, std::int16_t y, std::int16_t width, std::int16_t height) noexcept;
        void  write       (color fgColor, std::string_view text) noexcept;
        void  write       (color fgColor, std::string_view text, std::int16_t x, std::int16_t y, align hAlign = align::LEFT) noexcept;
        char* read_text   (color fgColor, color bgColor) noexcept;
        char* read_hidden () noexcept;

        std::int16_t get_width () const noexcept;
        std::int16_t get_height() const noexcept;

    private:
        bool valid;

        buffer_handle buffer;
        std::int16_t  width;
        std::int16_t  height;
    };
};

#endif
