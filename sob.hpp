#pragma once
#include <cassert>
#include <iostream>
#include <sstream>
#include <string_view>
#include <tuple>
#include "static_string.hpp"

template <class T>
constexpr std::string_view type_name()
{
#if defined(__clang__) || defined(__GNUC__)
    // __PRETTY_FUNCTION__ contains the type name in a compiler-specific format.
    std::string_view p = __PRETTY_FUNCTION__;
    // Example (clang): "constexpr std::string_view type_name() [T = int]"
    auto start = p.find("T = ");
    start = (start == std::string_view::npos) ? 0 : start + 4;
    // GCC/Clang format contains "; ..." after T = Type
    auto end = p.find(';', start);
    if (end == std::string_view::npos)
        end = p.find(']', start);
    return p.substr(start, end - start);
#elif defined(_MSC_VER)
    // __FUNCSIG__ contains the type name in a compiler-specific format.
    std::string_view p = __FUNCSIG__;
    // Example (MSVC): "class std::basic_string_view<char,struct std::char_traits<char> > __cdecl type_name<int>(void)"
    auto start = p.find("type_name<");
    start = (start == std::string_view::npos) ? 0 : start + 10;
    auto end = p.find(">(void)", start);
    return p.substr(start, end - start);
#else
    return "unknown";
#endif
}

#pragma message "Hello, from Sopho"

namespace sopho
{
    template <typename T>
    struct BuildTarget;
    // Generic template declaration
    template <typename DepTuple>
    struct DepRunner;

    // Specialization for std::tuple<Deps...>
    template <typename... Deps>
    struct DepRunner<std::tuple<Deps...>>
    {
        static void build()
        {
            // Build each dependency in order
            (BuildTarget<Deps>{}.build(), ...);
        }
    };

    // Specialization for empty tuple
    template <>
    struct DepRunner<std::tuple<>>
    {
        static void build()
        {
            // No dependencies, do nothing
        }
    };

    enum class BuildType
    {
        CXX
    };

    template <typename T>
    struct BuildTarget : public T
    {
        static void build()
        {
            DepRunner<typename T::Dependent>::build();
            std::string command{};
            switch (T::build_type)
            {
            case BuildType::CXX:
                {
                    std::stringstream ss{};
                    for (const auto& arg : T::args)
                    {
                        ss << arg << " ";
                    }
                    command = ss.str();
                }
                break;
            default:
                assert(!"Unknowen Build Type");
                break;
            }
            std::cout << type_name<T>() << ":" << command << std::endl;
            std::system(command.data());
            std::cout << type_name<T>() << ":finished" << std::endl;
        }
    };
} // namespace sopho
