#pragma once
#include <cassert>
#include <cstddef>
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

namespace sopho
{
    namespace detail
    {

        // Declaration: Map takes a Tuple type and a Mapper template
        template <typename Tuple, template <typename> class Mapper>
        struct MapImpl;

        // Specialization: Unpack the tuple types (Ts...), apply Mapper to each, repack.
        template <typename... Ts, template <typename> class Mapper>
        struct MapImpl<std::tuple<Ts...>, Mapper>
        {
            // The "return value" of a metafunction is usually defined as 'type'
            using type = std::tuple<Mapper<Ts>...>;
        };
    } // namespace

    // Helper alias for cleaner syntax (C++14 style aliases)
    template <typename Tuple, template <typename> class Mapper>
    using Map = typename detail::MapImpl<Tuple, Mapper>::type;


    // Generic template declaration
    template <typename Tuple, template <typename> class Policy>
    struct TupleExpander;

    // Specialization for std::tuple<Deps...>
    template <typename... Tuple, template <typename> class Policy>
    struct TupleExpander<std::tuple<Tuple...>, Policy>
    {
        static void execute()
        {
            // Build each dependency in order
            (Policy<Tuple>::execute(), ...);
        }
    };

    // Specialization for empty tuple
    template <template <typename> class Policy>
    struct TupleExpander<std::tuple<>, Policy>
    {
        static void execute()
        {
            // No dependencies, do nothing
        }
    };


    template <typename T, typename = void>
    struct has_source : std::false_type
    {
    };

    template <typename T>
    struct has_source<T, std::void_t<decltype(T::source)>> : std::true_type
    {
    };

    template <typename T>
    inline constexpr bool has_source_v = has_source<T>::value;

    template <typename Context>
    struct CxxToolchain
    {

        template <typename Target>
        struct CxxBuilder;

        template <typename Target>
        struct BuilderWrapper
        {
            static void execute() { CxxBuilder<Target>::build(); }
        };

        template <size_t Size>
        constexpr static auto source_to_target(StaticString<Size> source)
        {
            return source.template strip_suffix<4>().append(Context::obj_postfix);
        }

        template <typename Target>
        struct CxxBuilder
        {

            template <typename Tuple, size_t... Is>
            constexpr static void append_deps_impl(std::stringstream& ss, std::index_sequence<Is...>)
            {
                ((ss << " " << source_to_target(std::tuple_element_t<Is, Tuple>::source).view()), ...);
            }

            constexpr static void append_dependencies_artifacts(std::stringstream& ss)
            {
                using DepTuple = typename Target::Dependent;
                constexpr size_t Size = std::tuple_size_v<DepTuple>;
                if constexpr (Size > 0)
                {
                    append_deps_impl<DepTuple>(ss, std::make_index_sequence<Size>{});
                }
            }


            static void build()
            {

                sopho::TupleExpander<typename Target::Dependent, BuilderWrapper>::execute();
                std::string command{};
                std::stringstream ss{};
                ss << Context::cxx;

                if constexpr (has_source_v<Target>)
                {
                    static_assert(!Target::source.view().empty(), "Source file cannot be empty");

                    ss << " -c " << Target::source.view() << " -o " << source_to_target(Target::source).view();
                }
                else
                {
                    static_assert(std::tuple_size_v<typename Target::Dependent> > 0,
                                  "Link target must have dependencies (object files)");

                    append_dependencies_artifacts(ss);
                    ss << " -o " << Target::target.view();
                }

                command = ss.str();

                std::cout << type_name<Target>() << ":" << command << std::endl;
                std::system(command.data());
                std::cout << type_name<Target>() << ":finished" << std::endl;
            }
        };
    };

} // namespace sopho
