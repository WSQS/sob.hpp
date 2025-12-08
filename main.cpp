#include <array>
#include <tuple>
#include "sob.hpp"

struct MainSource
{
    static constexpr sopho::BuildType build_type{sopho::BuildType::CXX};
    using Dependent = std::tuple<>;
    static constexpr std::string_view source{"main.cpp"};
    static constexpr std::array<std::string_view, 5> args{"g++", "-c", source, "-o", "main.o"};
};
struct Main
{
    static constexpr sopho::BuildType build_type{sopho::BuildType::CXX};
    using Dependent = std::tuple<MainSource>;
    static constexpr std::array<const char*, 4> args{"g++", "main.o", "-o", "main"};
};

template <typename T>
struct Cxx_Source : public T
{
    static constexpr std::string_view suffix{".o"};
    static constexpr std::string_view target{typename T::source.remove_suffix(3)};
};

int main()
{
    sopho::BuildTarget<Main>::build();
    return 0;
}
