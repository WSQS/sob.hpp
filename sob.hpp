#pragma once
#include <sstream>
#include <iostream>
#include <string_view>
#include <cassert>

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

namespace sopho
{

    enum class BuildType
    {
        CXX
    };

    template <typename T>
    struct BuildTarget : public T
    {
        void build()
        {
            std::string command{};
            switch (this->build_type)
            {
            case BuildType::CXX:
            {
                std::stringstream ss{};
                for (const auto &arg : this->args)
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
            std::system(command.data());
            std::cout << type_name<T>() << " finished" << std::endl;
        }
    };
}
