#pragma once
#include <array>
#include <cstdint>
#include <string_view>
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
