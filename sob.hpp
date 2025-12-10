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

        // Declaration: Map takes a Mapper template and a Tuple type
        template <template <typename> class Mapper, typename Tuple>
        struct MapImpl;

        // Specialization: Unpack the tuple types (Ts...), apply Mapper to each, repack.
        template <template <typename> class Mapper, typename... Ts>
        struct MapImpl<Mapper, std::tuple<Ts...>>
        {
            // The "return value" of a metafunction is usually defined as 'type'
            using type = std::tuple<Mapper<Ts>...>;
        };
    } // namespace detail

    // Helper alias for cleaner syntax (C++14 style aliases)
    template <template <typename> class Mapper, typename Tuple>
    using Map = typename detail::MapImpl<Mapper, Tuple>::type;

    namespace detail
    {
        template <template <typename, typename> typename Folder, typename Value, typename Tuple>
        struct FoldlImpl;

        template <template <typename, typename> typename Folder, typename Value, typename T, typename... Ts>
        struct FoldlImpl<Folder, Value, std::tuple<T, Ts...>>
        {
            // The "return value" of a metafunction is usually defined as 'type'
            using type = typename FoldlImpl<Folder, typename Folder<Value, T>::type, std::tuple<Ts...>>::type;
        };

        template <template <typename, typename> typename Folder, typename Value>
        struct FoldlImpl<Folder, Value, std::tuple<>>
        {
            // The "return value" of a metafunction is usually defined as 'type'
            using type = Value;
        };

    } // namespace detail

    template <template <typename, typename> class Folder, typename Tuple, typename Value>
    using Foldl = typename detail::FoldlImpl<Folder, Tuple, Value>::type;

    // Generic detection idiom core
    template <typename, template <typename> class, typename = void>
    struct is_detected : std::false_type
    {
    };

    template <typename T, template <typename> class Op>
    struct is_detected<T, Op, std::void_t<Op<T>>> : std::true_type
    {
    };

    template <typename T, template <typename> class Op>
    inline constexpr bool is_detected_v = is_detected<T, Op>::value;

    // Expression template: get type of 'T::source' (works for static and non-static)
    template <typename T>
    using detect_source = decltype(std::declval<T&>().source);

    // Final trait: has_source_v<T>
    template <typename T>
    inline constexpr bool has_source_v = is_detected_v<T, detect_source>;

    template <typename Context>
    struct CxxToolchain
    {

        template <typename Target>
        struct CxxBuilder;

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

            template <typename L, typename R>
            struct BuildFolder
            {
                struct Builder
                {
                    static void build()
                    {
                        L::build();
                        R::build();
                    }
                };
                using type = Builder;
            };

            struct DumbBuilder
            {
                static void build() {}
            };


            static void build()
            {

                Foldl<BuildFolder, DumbBuilder, Map<CxxBuilder, typename Target::Dependent>>::build();

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
