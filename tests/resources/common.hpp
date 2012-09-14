#include "buffer.h"

#include <iostream>

const bool ios_hook = std::ios::sync_with_stdio(false);

bool writeAll()
{
    std::cout.write(buf, BUFSIZE);
    std::cout.flush();
    assert(std::cout);
    return true;
}

bool readAll()
{
    std::cin.read(buf, BUFSIZE);
    if (std::cin.gcount())
    {
        assert(std::cin.gcount() == BUFSIZE);
        assert(std::cin);
        return true;
    }
    else
    {
        return false;
    }
}
