#include "to_base.h"
#include "crypto.h"

#include <iostream>

static char const wow[]{ "abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ" };

void main()
{
    char* buf = nullptr;
    std::size_t buf_len = 0;
    char* buf2 = nullptr;
    std::size_t buf2_len = 0;
    bool success = pm::encrypt("1234", static_cast<void const*>(&wow[0]), sizeof(wow), reinterpret_cast<void**>(&buf), &buf_len);
    success = pm::decrypt("1234", buf, buf_len, reinterpret_cast<void**>(&buf2), &buf2_len);
    for (int i = 0; i < 10; i++)
    {
        std::size_t rand;
        pm::get_random_bytes(&rand, sizeof(rand));
        std::cout << pm::to_base(rand, wow) << std::endl;
    }
}