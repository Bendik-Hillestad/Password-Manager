#include "to_base.h"
#include "crypto.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>

static char const wow[]{ "abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ" };

void ShowInput(bool enable)
{
    //Get the current console mode
    DWORD  mode;
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    GetConsoleMode(hStdin, &mode);

    //Set/Clear the echo bit
    if (enable) mode |=  ENABLE_ECHO_INPUT;
    else        mode &= ~ENABLE_ECHO_INPUT;

    //Change mode
    SetConsoleMode(hStdin, mode);
}

void main()
{
    std::cout << "Enter password: ";
    ShowInput(false);
    std::string password;
    std::cin >> password;
    std::cout << "******" << std::endl;
    ShowInput(true);
    std::string test;
    std::cin >> test;
    char* buf = nullptr;
    std::size_t buf_len = 0;
    char* buf2 = nullptr;
    std::size_t buf2_len = 0;

    bool success = pm::encrypt(password, static_cast<void const*>(&wow[0]), sizeof(wow), reinterpret_cast<void**>(&buf), &buf_len);
    success = pm::decrypt(password, buf, buf_len, reinterpret_cast<void**>(&buf2), &buf2_len);
    for (int i = 0; i < 10; i++)
    {
        std::size_t rand;
        pm::get_random_bytes(&rand, sizeof(rand));
        std::cout << pm::to_base(rand, wow) << std::endl;
    }
}