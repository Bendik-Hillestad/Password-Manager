#include "screen.h"

#include <cstdlib>
#include <vector>
#include <array>
#include <malloc.h>
#include <type_traits>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

static_assert(std::is_same_v<pm::ui::buffer_handle, HANDLE>);

static constexpr DWORD READ_LINE_VISIBLE = ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_QUICK_EDIT_MODE | ENABLE_INSERT_MODE | ENABLE_PROCESSED_INPUT | ENABLE_EXTENDED_FLAGS;
static constexpr DWORD READ_LINE_HIDDEN  = ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT;

std::uint16_t get_foreground_color(pm::ui::color foreground)
{
    switch (foreground)
    {
        case pm::ui::color::BLACK:   return 0;
        case pm::ui::color::MAROON:  return FOREGROUND_RED;
        case pm::ui::color::GREEN:   return FOREGROUND_GREEN;
        case pm::ui::color::NAVY:    return FOREGROUND_BLUE;
        case pm::ui::color::OLIVE:   return FOREGROUND_RED   | FOREGROUND_GREEN;
        case pm::ui::color::PURPLE:  return FOREGROUND_RED   | FOREGROUND_BLUE;
        case pm::ui::color::TEAL:    return FOREGROUND_GREEN | FOREGROUND_BLUE;
        case pm::ui::color::GREY:    return FOREGROUND_RED   | FOREGROUND_GREEN | FOREGROUND_BLUE;

        case pm::ui::color::RED:     return FOREGROUND_INTENSITY | FOREGROUND_RED;
        case pm::ui::color::LIME:    return FOREGROUND_INTENSITY | FOREGROUND_GREEN;
        case pm::ui::color::BLUE:    return FOREGROUND_INTENSITY | FOREGROUND_BLUE;
        case pm::ui::color::YELLOW:  return FOREGROUND_INTENSITY | FOREGROUND_RED   | FOREGROUND_GREEN;
        case pm::ui::color::MAGENTA: return FOREGROUND_INTENSITY | FOREGROUND_RED   | FOREGROUND_BLUE;
        case pm::ui::color::CYAN:    return FOREGROUND_INTENSITY | FOREGROUND_GREEN | FOREGROUND_BLUE;
        case pm::ui::color::WHITE:   return FOREGROUND_INTENSITY | FOREGROUND_RED   | FOREGROUND_GREEN | FOREGROUND_BLUE;

        default: __assume(0);
    }
}

std::uint16_t get_background_color(pm::ui::color foreground)
{
    switch (foreground)
    {
        case pm::ui::color::BLACK:   return 0;
        case pm::ui::color::MAROON:  return BACKGROUND_RED;
        case pm::ui::color::GREEN:   return BACKGROUND_GREEN;
        case pm::ui::color::NAVY:    return BACKGROUND_BLUE;
        case pm::ui::color::OLIVE:   return BACKGROUND_RED   | BACKGROUND_GREEN;
        case pm::ui::color::PURPLE:  return BACKGROUND_RED   | BACKGROUND_BLUE;
        case pm::ui::color::TEAL:    return BACKGROUND_GREEN | BACKGROUND_BLUE;
        case pm::ui::color::GREY:    return BACKGROUND_RED   | BACKGROUND_GREEN | BACKGROUND_BLUE;

        case pm::ui::color::RED:     return BACKGROUND_INTENSITY | BACKGROUND_RED;
        case pm::ui::color::LIME:    return BACKGROUND_INTENSITY | BACKGROUND_GREEN;
        case pm::ui::color::BLUE:    return BACKGROUND_INTENSITY | BACKGROUND_BLUE;
        case pm::ui::color::YELLOW:  return BACKGROUND_INTENSITY | BACKGROUND_RED   | BACKGROUND_GREEN;
        case pm::ui::color::MAGENTA: return BACKGROUND_INTENSITY | BACKGROUND_RED   | BACKGROUND_BLUE;
        case pm::ui::color::CYAN:    return BACKGROUND_INTENSITY | BACKGROUND_GREEN | BACKGROUND_BLUE;
        case pm::ui::color::WHITE:   return BACKGROUND_INTENSITY | BACKGROUND_RED   | BACKGROUND_GREEN | BACKGROUND_BLUE;

        default: __assume(0);
    }
}

std::uint16_t extract_foreground_color(std::uint16_t attribute)
{
    return attribute & (FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
}

std::uint16_t extract_background_color(std::uint16_t attribute)
{
    return attribute & (BACKGROUND_INTENSITY | BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE);
}

bool pm::ui::screen::make_screen(screen* const &dst) noexcept
{
    HANDLE                     hOutput = INVALID_HANDLE_VALUE;
    HANDLE                     hBuffer = INVALID_HANDLE_VALUE;
    CONSOLE_SCREEN_BUFFER_INFO info;

    //Check that the destination is not a nullptr
    if (dst == nullptr) return false;

    //Get default output handle
    hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOutput == INVALID_HANDLE_VALUE)
    {
        std::fprintf(stderr, "GetStdHandle returned invalid handle value.\n");
        goto exit;
    }

    //Create a new screen buffer
    hBuffer = CreateConsoleScreenBuffer
    (
        GENERIC_READ    | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        CONSOLE_TEXTMODE_BUFFER,
        nullptr
    );
    if (hBuffer == INVALID_HANDLE_VALUE)
    {
        std::fprintf(stderr, "CreateConsoleScreenBuffer failed - (%d).\n", GetLastError());
        goto exit;
    }

    //Use our new buffer
    if (!SetConsoleActiveScreenBuffer(hBuffer))
    {
        std::fprintf(stderr, "SetConsoleActiveScreenBuffer failed - (%d).\n", GetLastError());
        goto exit;
    }

    //Get the size of the screen buffer
    if (!GetConsoleScreenBufferInfo(hBuffer, &info))
    {
        std::fprintf(stderr, "GetConsoleScreenBufferInfo failed - (%d).\n", GetLastError());
        goto exit;
    }

    //Store the buffer
    dst->valid  = true;
    dst->buffer = hBuffer;
    dst->width  = info.dwSize.X;
    dst->height = info.dwSize.Y;

exit:
    //Check if we produced a valid screen
    if (dst->valid == true) return true;

    //Check if we got the default output handle
    if (hOutput != INVALID_HANDLE_VALUE)
    {
        //Set as active
        SetConsoleActiveScreenBuffer(hOutput);
    }

    //Check if we created a buffer
    if (hBuffer != INVALID_HANDLE_VALUE)
    {
        //Cleanup
        CloseHandle(hBuffer);
    }

    //Return failure
    return false;
}

void pm::ui::screen::clear_screen(color clearColor) noexcept
{
    static constexpr COORD const topLeft{ 0, 0 };

    //Get the associated background color
    auto bg = get_background_color(clearColor);

    //Ignore how much is actually written
    DWORD dummy;

    //Clear out the buffer
    FillConsoleOutputAttribute (this->buffer, bg,  width * height, topLeft, &dummy);
    FillConsoleOutputCharacterA(this->buffer, ' ', width * height, topLeft, &dummy);
}

void pm::ui::screen::paint(color paintColor, std::int16_t x, std::int16_t y, std::int16_t width, std::int16_t height) noexcept
{
    //Get the associated background color
    auto bg = get_background_color(paintColor);

    //Setup the region we will write to
    SMALL_RECT rect;
    rect.Left   = x;
    rect.Top    = y;
    rect.Right  = x + width  - 1;
    rect.Bottom = y + height - 1;

    //Setup our cell which we will copy over the region
    CHAR_INFO cell;
    cell.Attributes     = bg;
    cell.Char.AsciiChar = ' ';

    //Allocate a temporary array on the stack to use
    auto  num = width * height;
    auto* buf = static_cast<CHAR_INFO*>(alloca(num * sizeof(CHAR_INFO)));

    //Fill it with our cell
    for (decltype(num) i = 0; i < num; i++) buf[i] = cell;

    //Apply it to the buffer
    WriteConsoleOutputA(this->buffer, buf, COORD{ width, height }, COORD{ 0, 0 }, &rect);
}

void pm::ui::screen::write(color fgColor, std::string_view text) noexcept
{
    //Get the length of the string
    auto strLen = static_cast<DWORD>(text.size());

    //Get the associated foreground color
    auto fg = get_foreground_color(fgColor);

    //Get current cursor position
    CONSOLE_SCREEN_BUFFER_INFO info;
    GetConsoleScreenBufferInfo(this->buffer, &info);

    //Get the attributes in the region we want to write to
    DWORD numAttrRead;
    auto* attr = static_cast<WORD*>(alloca(strLen * sizeof(WORD)));
    ReadConsoleOutputAttribute(this->buffer, attr, strLen, info.dwCursorPosition, &numAttrRead);

    //Extract the background and add the foreground color
    for (DWORD i = 0; i < numAttrRead; i++) 
        attr[i] = extract_background_color(attr[i]) | fg;

    //Write back the attributes
    DWORD numAttrWritten;
    WriteConsoleOutputAttribute(this->buffer, attr, strLen, info.dwCursorPosition, &numAttrWritten);

    //Write text to our buffer at the current cursor position
    DWORD numCharWritten;
    WriteConsoleOutputCharacterA(this->buffer, text.data(), strLen, info.dwCursorPosition, &numCharWritten);

    //Calculate the new cursor position
    SHORT newX = static_cast<SHORT>((info.dwCursorPosition.X + numCharWritten) % info.dwSize.X);
    SHORT newY = static_cast<SHORT>(info.dwCursorPosition.Y + ((info.dwCursorPosition.X + numCharWritten) / info.dwSize.X));

    //Move cursor
    SetConsoleCursorPosition(this->buffer, COORD{ newX, newY });
}

void pm::ui::screen::write(color fgColor, std::string_view text, std::int16_t x, std::int16_t y, align hAlign) noexcept
{
    //Check the alignment
    switch (hAlign)
    {
        //Do nothing if LEFT

        case align::MIDDLE:
        {
            //Subtract half-width from x
            x -= text.length() / 2;
        } break;

        case align::RIGHT:
        {
            //Subtract width from x
            x -= text.length();
        } break;
    }

    //Move the cursor
    SetConsoleCursorPosition(this->buffer, COORD{ x, y });

    //Write the text
    this->write(fgColor, text);
}

char* pm::ui::screen::read_text(color fgColor, color bgColor) noexcept
{
    //Get the associated foreground/background colors
    auto fg = get_foreground_color(fgColor);
    auto bg = get_background_color(bgColor);

    //Any input from the user will overwrite the existing text attributes buffer, 
    //so we just force it to use a particular attribute.
    SetConsoleTextAttribute(this->buffer, fg | bg);

    //Get the input handle
    auto hStdin = GetStdHandle(STD_INPUT_HANDLE);

    //Set the input mode
    SetConsoleMode(hStdin, READ_LINE_VISIBLE);

    //Clear any existing input in the buffer
    FlushConsoleInputBuffer(hStdin);

    //Start our reading loop
    std::vector<char> vec{};
    while (true)
    {
        //Read some input
        std::array<char, 16> buf{}; DWORD read;
        ReadFile(hStdin, buf.data(), static_cast<DWORD>(buf.size()), &read, nullptr);

        //Look for the carriage return
        auto end = std::find(buf.begin(), buf.begin() + read, '\r');

        //Copy the input into the vector
        std::copy(buf.begin(), end, std::back_inserter(vec));

        //Exit if we found the carriage return
        if (*end == '\r') break;
    }

    //Clear any leftover input in the buffer
    FlushConsoleInputBuffer(hStdin);

    //Allocate a buffer on the heap to contain the value we read + zero terminator
    auto* ret = new char[vec.size() + 1]; auto* ptr = ret;

    //Copy from the vector into the array
    for (auto i = vec.begin(); i != vec.end(); i++, ptr++)
    {
        *ptr = *i;
    }
    ret[vec.size()] = '\0';

    //Return the string
    return ret;
}

char* pm::ui::screen::read_hidden() noexcept
{
    //Get the input handle
    auto hStdin = GetStdHandle(STD_INPUT_HANDLE);

    //Set the input mode
    SetConsoleMode(hStdin, READ_LINE_HIDDEN);

    //Clear any existing input in the buffer
    FlushConsoleInputBuffer(hStdin);

    //Start our reading loop
    std::vector<char> vec{};
    while (true)
    {
        //Read some input
        std::array<char, 16> buf{}; DWORD read;
        ReadFile(hStdin, buf.data(), static_cast<DWORD>(buf.size()), &read, nullptr);

        //Look for the carriage return
        auto end = std::find(buf.begin(), buf.begin() + read, '\r');

        //Copy the input into the vector
        std::copy(buf.begin(), end, std::back_inserter(vec));

        //Exit if we found the carriage return
        if (*end == '\r') break;
    }

    //Clear any leftover input in the buffer
    FlushConsoleInputBuffer(hStdin);

    //Allocate a buffer on the heap to contain the value we read + zero terminator
    auto* ret = new char[vec.size() + 1]; auto* ptr = ret;

    //Copy from the vector into the array
    for (auto i = vec.begin(); i != vec.end(); i++, ptr++)
    {
        *ptr = *i;
    }
    ret[vec.size()] = '\0';

    //Return the string
    return ret;
}

std::int16_t pm::ui::screen::get_width() const noexcept
{
    return this->width;
}

std::int16_t pm::ui::screen::get_height() const noexcept
{
    return this->height;
}
