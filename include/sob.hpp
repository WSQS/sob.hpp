#pragma once
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>
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

    struct CompileCommand
    {
        std::string directory{};
        std::vector<std::string> arguments{};
        std::string file{};
    };

    inline void write_compile_commands_json(const std::string& path, const std::vector<CompileCommand>& commands)
    {
        std::ofstream out(path);
        out << "[\n";
        for (size_t i = 0; i < commands.size(); ++i)
        {
            const auto& cmd = commands[i];
            out << "  {\n";
            out << "    \"directory\": \"" << cmd.directory << "\",\n";
            out << "    \"file\": \"" << cmd.file << "\",\n";
            out << "    \"arguments\": [";
            for (size_t j = 0; j < cmd.arguments.size(); ++j)
            {
                out << "\"" << cmd.arguments[j] << "\"";
                if (j + 1 < cmd.arguments.size())
                    out << ", ";
            }
            out << "]\n";
            out << "  }";
            if (i + 1 < commands.size())
                out << ",";
            out << "\n";
        }
        out << "]\n";
    }

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
            return append(append(Context::build_prefix, strip_suffix(source, StaticString{".cpp"})),
                          Context::obj_postfix);
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
                    static std::vector<CompileCommand> build()
                    {
                        std::vector<CompileCommand> result;
                        auto left = L::build();
                        auto right = R::build();
                        result.insert(result.end(), left.begin(), left.end());
                        result.insert(result.end(), right.begin(), right.end());
                        return result;
                    }
                };
                using type = Builder;
            };

            struct DumbBuilder
            {
                static std::vector<CompileCommand> build() { return {}; }
            };

            using DependentBuilder =
                Foldl<BuildFolder, DumbBuilder, Map<CxxBuilderWrapper, dependent_or_empty_t<Target>>>;

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
                    static constexpr auto target = append(append(L::target, StaticString(" ")), R::target);
                };
                using type = TargetString;
            };

            struct DumbTargetString
            {
                static constexpr StaticString target{""};
            };

            using DependentNameCollector =
                Foldl<TargetStringFolder, DumbTargetString, Map<SourceToTarget, dependent_or_empty_t<Target>>>;

            static std::vector<CompileCommand> build()
            {
                std::vector<CompileCommand> commands = DependentBuilder::build();

                std::string command{};
                std::vector<std::string> command_parts;
                std::stringstream ss{};
                ss << Context::cxx;
                command_parts.push_back(std::string{Context::cxx});

                if constexpr (has_source_v<Target>)
                {
                    static_assert(!Target::source.view().empty(), "Source file cannot be empty");

                    auto target = source_to_target(Target::source);
                    ss << " -c " << Target::source.view() << Context::obj_prefix.view() << target.view();
                    command_parts.push_back("-c");
                    command_parts.push_back(std::string(Target::source.view()));
                    command_parts.push_back(std::string(Context::obj_prefix.view()));
                    command_parts.push_back(std::string(target.view()));
                    std::filesystem::path target_path{target.view()};
                    std::filesystem::create_directories(target_path.parent_path());

                    if constexpr (has_cxxflags_v<Context>)
                    {
                        for (const auto& flag : Context::cxxflags)
                        {
                            ss << " " << flag;
                            command_parts.push_back(std::string(flag));
                        }
                    }

                    commands.emplace_back(CompileCommand{std::filesystem::current_path().string(), command_parts,
                                                         std::string{Target::source.view()}});
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

                return commands;
            }
        };
    };

} // namespace sopho
