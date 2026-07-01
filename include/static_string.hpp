#pragma once
#include <array>
#include <cstdint>
#include <string_view>
namespace sopho
{
    template <std::size_t Size>
    struct StaticString
    {
        std::array<char, Size> data{};
        constexpr StaticString() = default;
        constexpr StaticString(const char (&str)[Size + 1])
        {
            for (std::size_t i = 0; i < Size; ++i)
            {
                data[i] = str[i];
            }
        }
        constexpr bool operator==(const StaticString&) const = default;
        constexpr std::size_t size() const { return Size; }
        constexpr std::string_view view() const { return std::string_view{data.data(), size()}; }
        constexpr char operator[](std::size_t idx) const { return data[idx]; }
    };

    template <std::size_t N>
    StaticString(const char (&)[N]) -> StaticString<N - 1>;

    template <std::size_t S, std::size_t M>
    constexpr bool has_suffix(const StaticString<S>& str, const StaticString<M>& suffix)
    {
        if (M > S)
        {
            return false;
        }
        for (std::size_t i = 0; i < M; ++i)
        {
            if (str[S - M + i] != suffix[i])
            {
                return false;
            }
        }
        return true;
    }

    template <std::size_t M, std::size_t S>
    constexpr StaticString<S - M> strip_suffix(const StaticString<S>& str, const StaticString<M>& suffix)
    {
        static_assert(M <= S, "Suffix is longer than the string itself");

        StaticString<S - M> result{};
        for (std::size_t i = 0; i < S - M; ++i)
        {
            result.data[i] = str[i];
        }

        return result;
    }

    template <std::size_t S, std::size_t M>
    constexpr StaticString<S + M> append(const StaticString<S>& a, const StaticString<M>& b)
    {
        StaticString<S + M> result{};
        for (std::size_t i = 0; i < S; ++i)
        {
            result.data[i] = a[i];
        }
        for (std::size_t i = 0; i < M; ++i)
        {
            result.data[i + S] = b[i];
        }

        return result;
    }
} // namespace sopho
