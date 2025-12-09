#include <array>
#include <string_view>
#include <tuple>
#include "sob.hpp"
#include "static_string.hpp"

#ifdef _MSVC_LANG
#define SOPHO_CPP_VER _MSVC_LANG
#else
#define SOPHO_CPP_VER __cplusplus
#endif

// Helper function to convert version number to string
constexpr const char* get_cpp_standard_name()
{
    if (SOPHO_CPP_VER > 202002L)
        return "C++23 (or newer)";
    if (SOPHO_CPP_VER == 202002L)
        return "C++20";
    if (SOPHO_CPP_VER == 201703L)
        return "C++17";
    if (SOPHO_CPP_VER == 201402L)
        return "C++14";
    if (SOPHO_CPP_VER == 201103L)
        return "C++11";
    if (SOPHO_CPP_VER == 199711L)
        return "C++98";
    return "Unknown/Pre-standard";
}

struct CxxContext
{
    static constexpr std::string_view cxx{"g++"};
    static constexpr sopho::StaticString obj_postfix{".o"};
};

struct MainSource
{
    using Dependent = std::tuple<>;
    static constexpr sopho::StaticString source{"main.cpp"};
};
struct Main
{
    using Dependent = std::tuple<MainSource>;
    static constexpr sopho::StaticString target{"main"};
};


int main()
{
    std::cout << get_cpp_standard_name() << std::endl;
    sopho::CxxToolchain<CxxContext>::CxxBuilder<Main>::build();
    return 0;
}
