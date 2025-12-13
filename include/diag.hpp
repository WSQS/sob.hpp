#pragma once
#include <cstdint>
#include <filesystem>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <string>
#include <variant>
#include "meta.hpp"

#define JOIN2(a, b) a##b
#define SOPHO_DETAIL_JOIN2(a, b) JOIN2(a, b)
#define SOPHO_STACK()                                                                                                  \
    StackScope SOPHO_DETAIL_JOIN2(_sopho_stack_scope_, __COUNTER__) { __FILE__, __func__, __LINE__ }
#define SOPHO_VALUE(value)                                                                                             \
    StackValue SOPHO_DETAIL_JOIN2(_sopho_stack_value_, __COUNTER__) { std::string(#value), value }


#define SOPHO_SOURCE_LOCATION                                                                                          \
    ::sopho::SourceLocation { __FILE__, __func__, static_cast<std::uint32_t>(__LINE__) }

// expr + variadic msg parts (at least one msg token is required by your convention)
#define SOPHO_ASSERT(expr, ...)                                                                                        \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(expr))                                                                                                   \
        {                                                                                                              \
            ::sopho::assert_fail(#expr, ::sopho::build_message(__VA_ARGS__), SOPHO_SOURCE_LOCATION);                   \
        }                                                                                                              \
    }                                                                                                                  \
    while (0)

namespace sopho
{
    struct SourceLocation
    {
        const char* file_name{};
        const char* function_name{};
        std::uint32_t line_number{};
    };

    template <typename T>
    struct ReferenceWrapper
    {
        using type = std::reference_wrapper<const T>;
    };

    [[noreturn]] inline void assert_fail(std::string_view expr, std::string msg, SourceLocation loc);

    // Build message only on failure path.
    template <class... Args>
    inline std::string build_message(Args&&... args)
    {
        std::ostringstream os;
        (os << ... << std::forward<Args>(args));
        return os.str();
    }

    using StackValueReference =
        Map<ReferenceWrapper, std::variant<std::int64_t, std::uint64_t, double, std::string, std::filesystem::path>>;

    struct StackInfo
    {
        SourceLocation source_location{};
        std::map<std::string, StackValueReference> stack_values{};
    };

    inline std::string stack_value_to_string(const StackValueReference& v)
    {
        return std::visit(
            [](const auto& ref) -> std::string
            {
                const auto& x = ref.get();
                using T = std::decay_t<decltype(x)>;

                if constexpr (std::is_same_v<T, std::string>)
                {
                    return x;
                }
                else if constexpr (std::is_same_v<T, std::filesystem::path>)
                {
                    // Note: string() is platform/native encoding; for cross-platform stable output you might prefer
                    // generic_u8string().
                    return x.string();
                }
                else
                {
                    // Covers int64_t / uint64_t / double
                    return std::to_string(x);
                }
            },
            v);
    }

    struct StackInfoInstance
    {
        std::list<StackInfo> stack_infos;
        static StackInfoInstance& get()
        {
            static thread_local StackInfoInstance instance{};
            return instance;
        }
    };

    struct StackScope
    {
        StackInfo* p_stack_info;
        StackScope(const char* file_name, const char* function_name, std::uint32_t line_number)
        {
            StackInfo stack_info{};
            stack_info.source_location.file_name = file_name;
            stack_info.source_location.function_name = function_name;
            stack_info.source_location.line_number = line_number;
            p_stack_info = &StackInfoInstance::get().stack_infos.emplace_front(stack_info);
        }
        ~StackScope()
        {
            SOPHO_ASSERT(!StackInfoInstance::get().stack_infos.empty(), "Stack has been cleared");
            SOPHO_ASSERT(&StackInfoInstance::get().stack_infos.front() == p_stack_info, "Stack Corrupted");
            StackInfoInstance::get().stack_infos.pop_front();
        }
        StackScope(const StackScope&) = delete;
        StackScope& operator=(const StackScope&) = delete;
        StackScope(StackScope&&) = delete;
        StackScope& operator=(StackScope&&) = delete;
    };

    struct StackValue
    {
        std::string name{};
        StackValue(std::string value_name, StackValueReference value)
        {
            name = value_name;
            StackInfoInstance::get().stack_infos.front().stack_values.insert_or_assign(value_name, value);
        }
        ~StackValue() { StackInfoInstance::get().stack_infos.front().stack_values.erase(name); }
        StackValue(const StackValue&) = delete;
        StackValue& operator=(const StackValue&) = delete;
        StackValue(StackValue&&) = delete;
        StackValue& operator=(StackValue&&) = delete;
    };

    void dump_callstack(std::ostringstream& ss)
    {
        auto& infos = StackInfoInstance::get().stack_infos;
        std::uint32_t size{0};
        for (const auto& info : infos)
        {
            ss << "stack" << size << ": " << info.source_location.file_name << ":" << info.source_location.line_number
               << "@" << info.source_location.function_name << std::endl;
            for (const auto& [key, value] : info.stack_values)
            {
                ss << "name:" << key << " value:" << stack_value_to_string(value) << std::endl;
            }
            ++size;
        }
    }

    [[noreturn]] inline void assert_fail(std::string_view expr, std::string msg, SourceLocation loc)
    {
        std::ostringstream ss;
        ss << "SOPHO_ASSERT failed: (" << expr << ")\n";

        if (!msg.empty())
        {
            ss << "Message: " << msg << "\n";
        }

        ss << "Location: " << (loc.file_name ? loc.file_name : "<unknown>") << ":" << loc.line_number << " @ "
           << (loc.function_name ? loc.function_name : "<unknown>") << "\n";

        dump_callstack(ss);

        std::cerr << ss.str();
        std::cerr.flush();
        std::abort();
    }

} // namespace sopho
