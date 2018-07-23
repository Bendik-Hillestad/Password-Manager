#include "app.h"

#include <cstdio>

int main()
{
    std::printf("Loading...\n");

    return pm::app{}.run();
}