#pragma once

#include <tuple>
#include <variant>

namespace sopho
{
    namespace detail
    {

        // Declaration: Map takes a Mapper template and a Tuple type
        template <template <typename> class Mapper, typename List>
        struct MapImpl;

        // Specialization: Unpack the tuple types (Ts...), apply Mapper to each, repack.
        template <template <typename> class Mapper, typename... Ts>
        struct MapImpl<Mapper, std::tuple<Ts...>>
        {
            // The "return value" of a metafunction is usually defined as 'type'
            using type = std::tuple<Mapper<Ts>...>;
        };

        template <template <typename> class Mapper, typename... Ts>
        struct MapImpl<Mapper, std::variant<Ts...>>
        {
            // The "return value" of a metafunction is usually defined as 'type'
            using type = std::variant<Mapper<Ts>...>;
        };
    } // namespace detail

    // Helper alias for cleaner syntax (C++14 style aliases)
    template <template <typename> class Mapper, typename List>
    using Map = typename detail::MapImpl<Mapper, List>::type;

    namespace detail
    {
        template <template <typename, typename> typename Folder, typename Value, typename List>
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

        template <template <typename, typename> typename Folder, typename Value, typename T, typename... Ts>
        struct FoldlImpl<Folder, Value, std::variant<T, Ts...>>
        {
            // The "return value" of a metafunction is usually defined as 'type'
            using type = typename FoldlImpl<Folder, typename Folder<Value, T>::type, std::variant<Ts...>>::type;
        };

        template <template <typename, typename> typename Folder, typename Value>
        struct FoldlImpl<Folder, Value, std::variant<>>
        {
            // The "return value" of a metafunction is usually defined as 'type'
            using type = Value;
        };

    } // namespace detail

    template <template <typename, typename> class Folder, typename Value, typename List>
    using Foldl = typename detail::FoldlImpl<Folder, Value, List>::type;
} // namespace sopho
