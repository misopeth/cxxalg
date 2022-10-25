#pragma once

#include <initializer_list>
#include <memory>
#include <type_traits>
#include <utility>

#include <cstddef>

#define MOV(...) static_cast<std::remove_reference_t<decltype(__VA_ARGS__)>&&>(__VA_ARGS__)
#define FWD(...) static_cast<decltype(__VA_ARGS__)&&>(__VA_ARGS__)

namespace cxxalg::impl {
    template<typename T>
    concept trivially_destructible = std::is_trivially_destructible_v<T>;

    template<typename T>
    concept copy_constructible = std::is_copy_constructible_v<T>;

    template<typename T>
    concept trivially_copy_constructible = copy_constructible<T> and std::is_trivially_copy_constructible_v<T>;

    template<typename T>
    concept move_constructible = std::is_move_constructible_v<T>;

    template<typename T>
    concept trivially_move_constructible = move_constructible<T> and std::is_trivially_move_constructible_v<T>;

    template<typename T>
    concept copy_assignable = copy_constructible<T> and std::is_copy_assignable_v<T>;

    template<typename T>
    concept trivially_copy_assignable = copy_assignable<T> and trivially_destructible<T>
        and std::is_trivially_copy_constructible_v<T>
        and std::is_trivially_copy_assignable_v<T>;

    template<typename T>
    concept move_assignable = move_constructible<T> and std::is_move_assignable_v<T>;

    template<typename T>
    concept trivially_move_assignable = move_assignable<T> and trivially_destructible<T>
        and std::is_trivially_move_constructible_v<T>
        and std::is_trivially_move_assignable_v<T>;

    template<typename>   struct is_in_place_type: std::false_type { };
    template<typename T> struct is_in_place_type<std::in_place_type_t<T>>: std::true_type { };
    template<typename T> inline constexpr bool is_in_place_type_v = is_in_place_type<T>::value;

    template<typename>      struct is_in_place_index: std::false_type { };
    template<std::size_t I> struct is_in_place_index<std::in_place_index_t<I>>: std::true_type { };
    template<typename T>    inline constexpr bool is_in_place_index_v = is_in_place_index<T>::value;

    template<typename T>
    struct special_members {
        static constexpr auto get(void      * ptr) noexcept { return static_cast<T      *>(ptr); }
        static constexpr auto get(void const* ptr) noexcept { return static_cast<T const*>(ptr); }

        template<typename... Args>
        static constexpr auto construct(void* dst, Args&&... args)
            noexcept(std::is_nothrow_constructible_v<T, Args...>)
            requires std::is_constructible_v<T, Args...>
        {
            return std::construct_at(get(dst), FWD(args)...);
        }

        template<typename U, typename... Args>
        static constexpr auto construct(void* dst, std::initializer_list<U> il, Args&&... args)
            noexcept(std::is_nothrow_constructible_v<T, std::initializer_list<U>&, Args...>)
            requires std::is_constructible_v<T, std::initializer_list<U>&, Args...>
        {
            return std::construct_at(get(dst), il, FWD(args)...);
        }

        static constexpr void destroy(void* ptr) noexcept
        {
            std::destroy_at(get(ptr));
        }

        static constexpr void copy_construct(void* dst, void const* src)
            noexcept(std::is_nothrow_copy_constructible_v<T>)
        {
            std::construct_at(get(dst), *get(src));
        }

        static constexpr void move_construct(void* dst, void* src)
            noexcept(std::is_nothrow_move_constructible_v<T>)
        {
            std::construct_at(get(dst), MOV(*get(src)));
        }

        static constexpr void copy_assign(void* dst, void const* src)
            noexcept(std::is_nothrow_copy_assignable_v<T>)
        {
            *get(dst) = *get(src);
        }

        static constexpr void move_assign(void* dst, void* src)
            noexcept(std::is_nothrow_move_assignable_v<T>)
        {
            *get(dst) = MOV(*get(src));
        }

        static constexpr void swap(void* a, void* b)
            noexcept(std::is_nothrow_swappable_v<T>)
        {
            using std::swap;
            swap(*get(a), *get(b));
        }
    };
}

#undef MOV
#undef FWD
