#include "sob.hpp"
#include <array>
struct Main
{
    sopho::BuildType build_type{sopho::BuildType::CXX};
    std::array<const char *, 4> args{"g++", "main.cpp", "-o", "main"};
};

int main()
{
    sopho::BuildTarget<Main>{}.build();
    return 0;
}
