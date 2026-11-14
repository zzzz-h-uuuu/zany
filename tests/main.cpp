#include <zany.hpp>
#include <cassert>
#include <iostream>

int main()
{
    zany a(42);

    auto ptr = any_cast<int>(&a);

    assert(ptr != nullptr);
    assert(*ptr == 42);

    std::cout << "zany basic test passed!\n";

    return 0;
}