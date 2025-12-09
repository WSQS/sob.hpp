#include <array>
#include <tuple>
#include "sob.hpp"

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

template <typename T>
struct Cxx_Source : public T
{
    static constexpr sopho::StaticString suffix{".o"};
    static constexpr sopho::StaticString target{T::source.template strip_suffix<4>()};
    static constexpr std::array<std::string_view, 5> args{"g++", "-c", T::source.view(), "-o", target.view()};
};

struct MainSource
{
    static constexpr sopho::BuildType build_type{sopho::BuildType::CXX};
    using Dependent = std::tuple<>;
    static constexpr sopho::StaticString source{"main.cpp"};
};
struct Main
{
    static constexpr sopho::BuildType build_type{sopho::BuildType::CXX};
    using Dependent = std::tuple<Cxx_Source<MainSource>>;
    static constexpr std::array<const char*, 4> args{"g++", "main.o", "-o", "main"};
};


int main()
{
    std::cout << get_cpp_standard_name() << std::endl;
    sopho::BuildTarget<Main>::build();
    return 0;
}
