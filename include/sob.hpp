#pragma once
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include "diag.hpp"
#include "file_generator.hpp"
#include "meta.hpp"
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

    template <typename T>
    using detect_ldflags = decltype(std::declval<T&>().ldflags);

    template <typename T>
    inline constexpr bool has_ldflags_v = is_detected_v<T, detect_ldflags>;

    template <typename T>
    using detect_cxxflags = decltype(std::declval<T&>().cxxflags);

    template <typename T>
    inline constexpr bool has_cxxflags_v = is_detected_v<T, detect_cxxflags>;

    template <typename T>
    using detect_dependent_type = typename T::Dependent;

    template <typename T>
    inline constexpr bool has_dependent_v = is_detected_v<T, detect_dependent_type>;

    template <typename T, typename = void>
    struct dependent_or_empty
    {
        using type = std::tuple<>;
    };

    template <typename T>
    struct dependent_or_empty<T, std::void_t<typename T::Dependent>>
    {
        using type = typename T::Dependent;
    };

    template <typename T>
    using dependent_or_empty_t = typename dependent_or_empty<T>::type;

    template <typename Context>
    struct CxxToolchain
    {

        template <typename Target>
        struct CxxBuilder;

        template <size_t Size>
        constexpr static auto source_to_target(StaticString<Size> source)
        {
            return Context::build_prefix.append(source.template strip_suffix<4>().append(Context::obj_postfix));
        }

        template <typename Target>
        struct CxxBuilderWrapper
        {
            using type = CxxBuilder<Target>;
        };

        template <typename Target>
        struct CxxBuilder
        {

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

            using DependentBuilder =
                Foldl<BuildFolder, DumbBuilder, Map<CxxBuilderWrapper, typename Target::Dependent>>;

            template <typename T>
            struct SourceToTarget
            {
                struct Result
                {

                    constexpr static auto target = source_to_target(T::source);
                };
                using type = Result;
            };

            template <typename L, typename R>
            struct TargetStringFolder
            {
                struct TargetString
                {
                    static constexpr auto target = L::target.append(StaticString(" ")).append(R::target);
                };
                using type = TargetString;
            };

            struct DumbTargetString
            {
                static constexpr StaticString target{""};
            };

            using DependentNameCollector =
                Foldl<TargetStringFolder, DumbTargetString, Map<SourceToTarget, dependent_or_empty_t<Target>>>;

            static void build()
            {
                DependentBuilder::build();

                std::string command{};
                std::stringstream ss{};
                ss << Context::cxx;

                if constexpr (has_source_v<Target>)
                {
                    static_assert(!Target::source.view().empty(), "Source file cannot be empty");

                    auto target = source_to_target(Target::source);
                    ss << " -c " << Target::source.view() << Context::obj_prefix.view() << target.view();
                    std::filesystem::path target_path{target.view()};
                    std::filesystem::create_directories(target_path.parent_path());

                    if constexpr (has_cxxflags_v<Context>)
                    {
                        for (const auto& flag : Context::cxxflags)
                        {
                            ss << " " << flag;
                        }
                    }
                }
                else
                {
                    static_assert(std::tuple_size_v<typename Target::Dependent> > 0,
                                  "Link target must have dependencies (object files)");

                    ss << DependentNameCollector::target.view();
                    ss << Context::bin_prefix.view() << Target::target.view();
                    if constexpr (has_ldflags_v<Context>)
                    {
                        for (const auto& flag : Context::ldflags)
                        {
                            ss << " " << flag;
                        }
                    }
                }

                command = ss.str();

                std::cout << type_name<Target>() << ":" << command << std::endl;
                std::system(command.data());
                std::cout << type_name<Target>() << ":finished" << std::endl;
            }
        };
    };

} // namespace sopho
