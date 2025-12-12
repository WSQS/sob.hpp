#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <array>
#include <cstdint>
namespace sopho
{
    template <std::size_t Size>
    struct StaticString
    {
        std::array<char, Size> raw{};
        constexpr StaticString() = default;
        constexpr StaticString(const char (&str)[Size + 1])
        {
            for (std::size_t i = 0; i < Size; ++i)
            {
                raw[i] = str[i];
            }
        }
        constexpr std::size_t size() const { return Size; }
        constexpr std::string_view view() const { return std::string_view{raw.data(), size()}; }
        constexpr char operator[](std::size_t idx) const { return raw[idx]; }
        template <std::size_t M>
        constexpr bool has_suffix(const StaticString<M>& suffix) const
        {
            // Case: Suffix is longer than the string -> definitely false
            if (M > Size)
            {
                return false;
            }
            // Architecture Note: We implement manual loop instead of string_view::ends_with
            // to ensure maximum compatibility with older C++ standards (C++14/17)
            // and guarantee constexpr execution.
            for (std::size_t i = 0; i < M; ++i)
            {
                if (raw[Size - M + i] != suffix[i])
                {
                    return false;
                }
            }
            return true;
        }
        template <std::size_t M>
        constexpr StaticString<Size - M> strip_suffix() const
        {
            static_assert(M <= Size, "Suffix is longer than the string itself");
            StaticString<Size - M> result{};
            for (std::size_t i = 0; i < Size - M; ++i)
            {
                result.raw[i] = raw[i];
            }
            return result;
        }
        template <std::size_t M>
        constexpr StaticString<Size + M> append(StaticString<M> suffix) const
        {
            StaticString<Size + M> result{};
            for (std::size_t i = 0; i < Size; ++i)
            {
                result.raw[i] = raw[i];
            }
            for (std::size_t i = 0; i < M; ++i)
            {
                result.raw[i + Size] = suffix[i];
            }
            return result;
        }
    };
    template <std::size_t N>
    StaticString(const char (&)[N]) -> StaticString<N - 1>;
} // namespace sopho
#include <filesystem>
#include <fstream>
#include <set>
#include <string>
#include <vector>
namespace sopho
{
    std::string read_file(std::filesystem::path fs_path)
    {
        std::ifstream file_stream(fs_path, std::ios::binary);
        assert(file_stream.is_open());
        const auto file_size = std::filesystem::file_size(fs_path);
        std::string content;
        content.resize(file_size);
        file_stream.read(content.data(), file_size);
        return content;
    }
    std::vector<std::string_view> split_lines(std::string_view str)
    {
        std::vector<std::string_view> result;
        size_t start = 0;
        while (start < str.size())
        {
            size_t end = str.find('\n', start);
            if (end == std::string_view::npos)
            {
                result.emplace_back(str.substr(start));
                break;
            }
            size_t line_end = end;
            if (line_end > start && str[line_end - 1] == '\r')
            {
                line_end -= 1;
            }
            result.emplace_back(str.substr(start, line_end - start));
            start = end + 1;
        }
        return result;
    }
    std::string_view ltrim(std::string_view sv)
    {
        size_t i = 0;
        while (i < sv.size() && (sv[i] == ' ' || sv[i] == '\t'))
        {
            ++i;
        }
        return sv.substr(i);
    }
    bool starts_with(std::string_view sv, std::string_view prefix)
    {
        return sv.size() >= prefix.size() && sv.compare(0, prefix.size(), prefix) == 0;
    }
    struct Context
    {
        std::filesystem::path include_path{};
        std::vector<std::string> file_content{};
        std::set<std::string_view> std_header{};
    };
    std::vector<std::string_view> collect_file(std::string_view file_path, Context& context)
    {
        std::vector<std::string_view> result{};
        std::filesystem::path fs_path = file_path;
        assert(std::filesystem::exists(fs_path));
        context.file_content.emplace_back(read_file(fs_path));
        auto lines = split_lines(std::string_view(context.file_content.back()));
        for (const auto& line : lines)
        {
            auto line_content = ltrim(line);
            if (line_content.length() == 0)
            {
                continue;
            }
            if (line_content[0] != '#')
            {
                result.emplace_back(line);
                continue;
            }
            line_content = line_content.substr(1);
            line_content = ltrim(line_content);
            if (starts_with(line_content, "include"))
            {
                line_content = line_content.substr(7);
                line_content = ltrim(line_content);
                if (line_content[0] == '<')
                {
                    line_content = line_content.substr(1);
                    auto index = line_content.find('>');
                    assert(index != std::string_view::npos);
                    auto file_name = line_content.substr(0, index);
                    if (context.std_header.find(file_name) != context.std_header.end())
                    {
                        continue;
                    }
                    context.std_header.emplace(file_name);
                    result.emplace_back(line);
                    continue;
                }
                else if (line_content[0] == '"')
                {
                    line_content = line_content.substr(1);
                    auto index = line_content.find('"');
                    assert(index != std::string_view::npos);
                    std::string file_name{line_content.substr(0, index)};
                    std::filesystem::path new_fs_path = fs_path.parent_path() / file_name;
                    if (!std::filesystem::exists(new_fs_path))
                    {
                        new_fs_path = context.include_path / file_name;
                    }
                    auto file_content = collect_file(std::string_view(new_fs_path.string()), context);
                    result.insert(result.end(), file_content.begin(), file_content.end());
                }
                else
                {
                    assert(!"not a valid header format");
                }
            }
            else if (starts_with(line_content, "pragma"))
            {
                line_content = line_content.substr(6);
                line_content = ltrim(line_content);
                if (starts_with(line_content, "once"))
                {
                    continue;
                }
                result.emplace_back(line);
            }
            else
            {
                result.emplace_back(line);
            }
        }
        return result;
    }
    void single_header_generator(std::string_view file_path)
    {
        Context context{};
        std::filesystem::path fs_path = file_path;
        assert(std::filesystem::exists(fs_path));
        context.include_path = fs_path.parent_path();
        auto lines = collect_file(file_path, context);
        std::ofstream out("sob.hpp", std::ios::binary);
        assert(out.is_open());
        for (auto sv : lines)
        {
            out.write(sv.data(), sv.size());
            out.put('\n');
        }
    }
} // namespace sopho
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
            return Context::build_prefix.append(source.template strip_suffix<4>().append(Context::obj_postfix));
        }
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
            using DependentBuilder = Foldl<BuildFolder, DumbBuilder, Map<CxxBuilder, typename Target::Dependent>>;
            template <typename T>
            struct SourceToTarget
            {
                constexpr static auto target = source_to_target(T::source);
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
                Foldl<TargetStringFolder, DumbTargetString, Map<SourceToTarget, typename Target::Dependent>>;
            static void build()
            {
                DependentBuilder::build();
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
                    ss << DependentNameCollector::target.view();
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
