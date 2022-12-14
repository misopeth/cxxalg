#pragma once

#include "impl/common.hxx"

#include <algorithm>
#include <compare>
#include <exception>
#include <functional>
#include <initializer_list>
#include <memory>
#include <type_traits>
#include <utility>

#include <cstddef>

#define MOV(...) static_cast<std::remove_reference_t<decltype(__VA_ARGS__)>&&>(__VA_ARGS__)
#define FWD(...) static_cast<decltype(__VA_ARGS__)&&>(__VA_ARGS__)

namespace cxxalg {
    template<typename... Types>
    class variant;

    inline constexpr std::size_t variant_npos = -1;

    // monostate
    struct monostate { };
    inline constexpr bool operator== (monostate, monostate) noexcept { return true; }
    inline constexpr auto operator<=>(monostate, monostate) noexcept { return std::strong_ordering::equal; }

    template<typename> struct variant_size;
    template<typename... Types> struct variant_size<variant<Types...>      >: std::integral_constant<std::size_t, sizeof...(Types)> { };
    template<typename... Types> struct variant_size<variant<Types...> const>: std::integral_constant<std::size_t, sizeof...(Types)> { };
    template<typename T> inline constexpr auto variant_size_v = variant_size<T>::value;

    template<std::size_t, typename>
    struct variant_alternative;
    template<std::size_t I, typename Head, typename... Tail>
    struct variant_alternative<I, variant<Head, Tail...>>: variant_alternative<I - 1, variant<Tail...>> { };
    template<typename Head, typename... Tail>
    struct variant_alternative<0, variant<Head, Tail...>> { using type = Head; };
    template<std::size_t I, typename T>
    struct variant_alternative<I, T const> { using type = std::add_const_t<typename variant_alternative<I, T>::type>; };
    template<std::size_t I, typename T>
    using variant_alternative_t = variant_alternative<I, T>::type;

    struct bad_variant_access: std::exception {
        using std::exception::exception;
        auto what() const noexcept -> char const* override { return "bad_variant_access"; }
    };

    namespace impl {
        template<typename...>
        inline constexpr std::size_t index_of;

        template<typename T, typename Head, typename... Tail>
        inline constexpr std::size_t index_of<T, Head, Tail...> = 1 + index_of<T, Tail...>;

        template<typename T, typename... Types>
            requires ((not std::is_same_v<T, Types>) and ...)
        inline constexpr std::size_t index_of<T, T, Types...> = 0;

        template<typename... Fs>
        struct overloaded: Fs... {
            using Fs::operator()...;
        };

        template<typename T>
        struct callable_type_identity {
            auto operator()(T) const noexcept -> T;
                //requires requires(T&& t, T a[]) { { a[0] = FWD(t) }; };
        };

        template<typename T, typename... Types>
        struct best_type_conversion: std::invoke_result<overloaded<callable_type_identity<Types>...>, T> { };

        template<typename T, typename... Types>
        using best_type_conversion_t = best_type_conversion<T, Types...>::type;

        template<typename T>
        struct noop_special_members {
            static constexpr void copy_construct(void*, void const*) noexcept { }
            static constexpr void move_construct(void*, void*) noexcept { }
            static constexpr void copy_assign(void*, void const*) noexcept { }
            static constexpr void move_assign(void*, void*) noexcept { }
        };
    }

    template<std::size_t I, typename... Ts>
        constexpr auto get(variant<Ts...>&) -> variant_alternative_t<I, variant<Ts...>>&;
    template<std::size_t I, typename... Ts>
        constexpr auto get(variant<Ts...>&&) -> variant_alternative_t<I, variant<Ts...>>&&;
    template<std::size_t I, typename... Ts>
        constexpr auto get(variant<Ts...> const&) -> variant_alternative_t<I, variant<Ts...>> const&;
    template<std::size_t I, typename... Ts>
        constexpr auto get(variant<Ts...> const&&) -> variant_alternative_t<I, variant<Ts...>> const&&;

    template<std::size_t I, typename... Ts>
        constexpr auto get_if(variant<Ts...>*) noexcept
            -> std::add_pointer_t<variant_alternative_t<I, variant<Ts...>>>;
    template<std::size_t I, typename... Ts>
        constexpr auto get_if(variant<Ts...> const*) noexcept
            -> std::add_pointer_t<variant_alternative_t<I, variant<Ts...>> const>;

    template<typename... Types>
    class variant {
        static_assert(sizeof...(Types) != 0);
        static_assert(not (std::is_void_v<Types> or ...));
        static_assert(not (std::is_array_v<Types> or ...));
        static_assert(not (std::is_reference_v<Types> or ...));

        alignas(Types...) std::byte storage_[std::max({sizeof(Types)...})];
        std::size_t index_ = variant_npos;

        static constexpr bool all_copy_constructible = (impl::copy_constructible<Types> and ...);
        static constexpr bool all_move_constructible = (impl::move_constructible<Types> and ...);
        static constexpr bool all_copy_assignable    = (impl::copy_assignable<Types>    and ...);
        static constexpr bool all_move_assignable    = (impl::move_assignable<Types>    and ...);

        static constexpr bool all_trivially_destructible       = (impl::trivially_destructible<Types>       and ...);
        static constexpr bool all_trivially_copy_constructible = (impl::trivially_copy_constructible<Types> and ...);
        static constexpr bool all_trivially_move_constructible = (impl::trivially_move_constructible<Types> and ...);
        static constexpr bool all_trivially_copy_assignable    = (impl::trivially_copy_assignable<Types>    and ...);
        static constexpr bool all_trivially_move_assignable    = (impl::trivially_move_assignable<Types>    and ...);

        static constexpr bool all_nothrow_move_constructible = (std::is_nothrow_move_constructible_v<Types> and ...);
        static constexpr bool all_nothrow_move_assignable    = (std::is_nothrow_move_assignable_v<Types>    and ...);
        static constexpr bool all_nothrow_swappable          = (std::is_nothrow_swappable_v<Types>          and ...);

    public:
        // (destructor)
        constexpr ~variant() requires all_trivially_destructible = default;
        constexpr ~variant()
        {
            if (not valueless_by_exception()) [[likely]]
                destroy_[index()](storage_);
        }

        // (constructor)
        // 1
        template<typename A = variant_alternative_t<0, variant>>
        constexpr variant() noexcept(std::is_nothrow_default_constructible_v<A>)
            requires std::is_default_constructible_v<A>
        {
            std::construct_at(get_as<A>());
            index_ = 0;
        }
        // 2
        constexpr variant(variant const&) requires all_trivially_copy_constructible = default;
        constexpr variant(variant const& that)
            requires (not all_trivially_copy_constructible) and all_copy_constructible
        {
            if (not that.valueless_by_exception()) { [[likely]]
                copy_construct_[that.index_](storage_, that.storage_);
                index_ = that.index_;
            }
        }
        // 3
        constexpr variant(variant&&) requires all_trivially_move_constructible = default;
        constexpr variant(variant&& that) noexcept(all_nothrow_move_constructible)
            requires (not all_trivially_move_constructible) and all_move_constructible
        {
            if (not that.valueless_by_exception()) { [[likely]]
                move_construct_[that.index_](storage_, that.storage_);
                index_ = that.index_;
            }
        }
        // 4
        template<typename T, typename A = impl::best_type_conversion_t<T, Types...>>
        constexpr variant(T&& t)
            noexcept(std::is_nothrow_constructible_v<A, T>)
            requires std::is_constructible_v<A, T>
                 and (not std::is_same_v<std::remove_cvref_t<T>, variant>)
                 and (not impl::is_in_place_type_v<std::remove_cvref_t<T>>)
                 and (not impl::is_in_place_index_v<std::remove_cvref_t<T>>)
        {
            std::construct_at(get_as<A>(), FWD(t));
            index_ = impl::index_of<A, Types...>;
        }
        // 5
        template<typename T, typename... Args>
        constexpr explicit variant(std::in_place_type_t<T>, Args&&... args)
            requires std::is_constructible_v<T, Args...>
                 and ((std::is_same_v<T, Types> + ...) == 1)
        {
            std::construct_at(get_as<T>(), FWD(args)...);
            index_ = impl::index_of<T, Types...>;
        }
        // 6
        template<typename T, typename U, typename... Args>
        constexpr explicit variant(std::in_place_type_t<T>, std::initializer_list<U> il, Args&&... args)
            requires std::is_constructible_v<T, std::initializer_list<U>&, Args...>
                 and ((std::is_same_v<T, Types> + ...) == 1)
        {
            std::construct_at(get_as<T>(), il, FWD(args)...);
            index_ = impl::index_of<T, Types...>;
        }
        // 7
        template<std::size_t I, typename... Args, typename A = variant_alternative_t<I, variant>>
        constexpr explicit variant(std::in_place_index_t<I>, Args&&... args)
            requires (I < sizeof...(Types)) and std::is_constructible_v<A, Args...>
        {
            std::construct_at(get_as<A>(), FWD(args)...);
            index_ = I;
        }
        // 8
        template<std::size_t I, typename U, typename... Args, typename A = variant_alternative_t<I, variant>>
        constexpr explicit variant(std::in_place_index_t<I>, std::initializer_list<U> il, Args&&... args)
            requires (I < sizeof...(Types)) and std::is_constructible_v<A, std::initializer_list<U>&, Args...>
        {
            std::construct_at(get_as<A>(), il, FWD(args)...);
            index_ = I;
        }

        // operator=
        // 1
        constexpr auto operator=(variant const&) -> variant& requires all_trivially_copy_assignable = default;
        constexpr auto operator=(variant const& that) -> variant&
            requires (not all_trivially_copy_assignable) and all_copy_assignable
        {
            if (this == &that) [[unlikely]]
                return *this;

            switch (valueless_by_exception() | that.valueless_by_exception() << 1) {
            case 0: [[likely]]
                if (index_ == that.index_) {
                    copy_assign_[index_](storage_, that.storage_);
                } else {
                    destroy_[index_](storage_);
                    index_ = variant_npos;
                    copy_construct_[that.index_](storage_, that.storage_);
                    index_ = that.index_;
                }
                break;
            case 1:
                copy_construct_[that.index_](storage_, that.storage_);
                index_ = that.index_;
                break;
            case 2:
                destroy_[index_](storage_);
                index_ = variant_npos;
                break;
            case 3:
            default:
                // Do nothing
                break;
            }
            return *this;
        }
        // 2
        constexpr auto operator=(variant&&) -> variant& requires all_trivially_move_assignable = default;
        constexpr auto operator=(variant&& that)
            noexcept(all_nothrow_move_constructible and all_nothrow_move_assignable) -> variant&
            requires (not all_trivially_move_assignable) and all_move_assignable
        {
            if (this == &that) [[unlikely]]
                return *this;

            switch (valueless_by_exception() | that.valueless_by_exception() << 1) {
            case 0: [[likely]]
                if (index_ == that.index_) {
                    move_assign_[index_](storage_, that.storage_);
                } else {
                    destroy_[index_](storage_);
                    index_ = variant_npos;
                    move_construct_[that.index_](storage_, that.storage_);
                    index_ = that.index_;
                }
                break;
            case 1:
                move_construct_[that.index_](storage_, that.storage_);
                index_ = that.index_;
                break;
            case 2:
                destroy_[index_](storage_);
                index_ = variant_npos;
                break;
            case 3:
            default:
                // Do nothing
                break;
            }
            return *this;
        }
        // 3
        template<typename T, typename A = impl::best_type_conversion_t<T, Types...>, auto I = impl::index_of<A, Types...>>
        constexpr auto operator=(T&& t)
            noexcept(std::is_nothrow_assignable_v<A&, T> and std::is_nothrow_constructible_v<A, T>) -> variant&
            requires std::is_constructible_v<A, T> and std::is_assignable_v<A&, T>
                 and (not std::is_same_v<std::remove_cvref_t<T>, variant>)
        {
            if (index() == I) {
                *get_as<A>() = FWD(t);
            } else {
                if constexpr (std::is_nothrow_constructible_v<A, T> or not std::is_nothrow_move_constructible_v<A>)
                    emplace<I>(FWD(t));
                else
                    emplace<I>(A(FWD(t)));
            }
            return *this;
        }

        // index
        constexpr auto index() const noexcept -> std::size_t
        {
            return index_;
        }

        // valueless_by_exception
        constexpr bool valueless_by_exception() const noexcept
        {
            return index_ == variant_npos;
        }

        // emplace
        // 1
        template<typename T, typename... Args>
        auto emplace(Args&&... args) -> T&
            requires std::is_constructible_v<T, Args...>
                 and ((std::is_same_v<T, Types> + ...) == 1)
        {
            return emplace<impl::index_of<T, Types...>>(FWD(args)...);
        }
        // 2
        template<typename T, typename U, typename... Args>
        auto emplace(std::initializer_list<U> il, Args&&... args) -> T&
            requires std::is_constructible_v<T, std::initializer_list<U>&, Args...>
                 and ((std::is_same_v<T, Types> + ...) == 1)
        {
            return emplace<impl::index_of<T, Types...>>(il, FWD(args)...);
        }
        // 3
        template<std::size_t I, typename... Args, typename A = variant_alternative_t<I, variant>>
        constexpr auto emplace(Args&&... args) -> A&
            requires std::is_constructible_v<A, Args...>
        {
            if (not valueless_by_exception()) {
                destroy_[index_](storage_);
                index_ = variant_npos;
            }
            std::construct_at(get_as<A>(), FWD(args)...);
            index_ = I;
            return *get_as<A>();
        }
        // 4
        template<std::size_t I, typename U, typename... Args, typename A = variant_alternative_t<I, variant>>
        constexpr auto emplace(std::initializer_list<U> il, Args&&... args) -> A&
            requires std::is_constructible_v<A, std::initializer_list<U>&, Args...>
        {
            if (not valueless_by_exception()) {
                destroy_[index_](storage_);
                index_ = variant_npos;
            }
            std::construct_at(get_as<A>(), il, FWD(args)...);
            index_ = I;
            return *get_as<A>();
        }

        // swap
        constexpr void swap(variant& that)
            noexcept(all_nothrow_move_constructible and all_nothrow_swappable)
        {
            switch (valueless_by_exception() | that.valueless_by_exception() << 1) {
            case 0: [[likely]]
                if (index_ == that.index_) {
                    swap_[index_](storage_, that.storage_);
                } else {
                    auto&& tmp = variant(MOV(*this));
                    *this = MOV(that);
                    that = MOV(tmp);
                }
                break;
            case 1:
                copy_construct_[that.index_](storage_, that.storage_);
                index_ = that.index_;
                destroy_[that.index_](that.storage_);
                that.index_ = variant_npos;
                break;
            case 2:
                copy_construct_[index_](that.storage_, storage_);
                that.index_ = index_;
                destroy_[index_](storage_);
                index_ = variant_npos;
                break;
            case 3:
            default:
                // Do nothing
                break;
            }
        }

    private:
        template<typename T> auto get_as()       noexcept { return reinterpret_cast<T*      >(&storage_); }
        template<typename T> auto get_as() const noexcept { return reinterpret_cast<T const*>(&storage_); }

        using dt_t = void(*)(void*)              noexcept(true);
        using cc_t = void(*)(void*, void const*) noexcept(false);
        using mc_t = void(*)(void*, void      *) noexcept(all_nothrow_move_constructible);
        using ca_t = void(*)(void*, void const*) noexcept(false);
        using ma_t = void(*)(void*, void      *) noexcept(all_nothrow_move_assignable);
        using sw_t = void(*)(void*, void      *) noexcept(all_nothrow_swappable);

        template<typename T> using dt_sm = impl::special_members<T>;
        template<typename T> using cc_sm = std::conditional_t<all_copy_constructible, impl::special_members<T>, impl::noop_special_members<T>>;
        template<typename T> using mc_sm = std::conditional_t<all_move_constructible, impl::special_members<T>, impl::noop_special_members<T>>;
        template<typename T> using ca_sm = std::conditional_t<all_copy_assignable,    impl::special_members<T>, impl::noop_special_members<T>>;
        template<typename T> using ma_sm = std::conditional_t<all_move_assignable,    impl::special_members<T>, impl::noop_special_members<T>>;
        template<typename T> using sw_sm = impl::special_members<T>;

        static constexpr auto destroy_        = std::array{ static_cast<dt_t>(&dt_sm<Types>::destroy)... };
        static constexpr auto copy_construct_ = std::array{ static_cast<cc_t>(&cc_sm<Types>::copy_construct)... };
        static constexpr auto move_construct_ = std::array{ static_cast<mc_t>(&mc_sm<Types>::move_construct)... };
        static constexpr auto copy_assign_    = std::array{ static_cast<ca_t>(&ca_sm<Types>::copy_assign)... };
        static constexpr auto move_assign_    = std::array{ static_cast<ma_t>(&ma_sm<Types>::move_assign)... };
        static constexpr auto swap_           = std::array{ static_cast<sw_t>(&sw_sm<Types>::swap)... };

        template<std::size_t I, typename... Ts>
            friend constexpr auto get(variant<Ts...>&) -> variant_alternative_t<I, variant<Ts...>>&;
        template<std::size_t I, typename... Ts>
            friend constexpr auto get(variant<Ts...>&&) -> variant_alternative_t<I, variant<Ts...>>&&;
        template<std::size_t I, typename... Ts>
            friend constexpr auto get(variant<Ts...> const&) -> variant_alternative_t<I, variant<Ts...>> const&;
        template<std::size_t I, typename... Ts>
            friend constexpr auto get(variant<Ts...> const&&) -> variant_alternative_t<I, variant<Ts...>> const&&;

        template<std::size_t I, typename... Ts>
            friend constexpr auto get_if(variant<Ts...>*) noexcept
                -> std::add_pointer_t<variant_alternative_t<I, variant<Ts...>>>;
        template<std::size_t I, typename... Ts>
            friend constexpr auto get_if(variant<Ts...> const*) noexcept
                -> std::add_pointer_t<variant_alternative_t<I, variant<Ts...>> const>;
    };

    // holds_alternative
    template<typename T, typename... Types>
    inline constexpr bool holds_alternative(variant<Types...> const& v) noexcept
    {
        auto i = std::size_t(0);
        return ((std::is_same_v<T, Types> and (v.index() == i++)) or ...);
    }

    // get
    // 1
    template<std::size_t I, typename... Types>
    inline constexpr auto get(variant<Types...>& v) -> variant_alternative_t<I, variant<Types...>>&
    {
        if (v.index() != I)
            throw bad_variant_access();

        using T = variant_alternative_t<I, variant<Types...>>;
        return *v.template get_as<T>();
    }
    template<std::size_t I, typename... Types>
    inline constexpr auto get(variant<Types...>&& v) -> variant_alternative_t<I, variant<Types...>>&&
    {
        if (v.index() != I)
            throw bad_variant_access();

        using T = variant_alternative_t<I, variant<Types...>>;
        return MOV(*v.template get_as<T>());
    }
    template<std::size_t I, typename... Types>
    inline constexpr auto get(variant<Types...> const& v) -> variant_alternative_t<I, variant<Types...>> const&
    {
        if (v.index() != I)
            throw bad_variant_access();

        using T = variant_alternative_t<I, variant<Types...>>;
        return *v.template get_as<T>();
    }
    template<std::size_t I, typename... Types>
    inline constexpr auto get(variant<Types...> const&& v) -> variant_alternative_t<I, variant<Types...>> const&&
    {
        if (v.index() != I)
            throw bad_variant_access();

        using T = variant_alternative_t<I, variant<Types...>>;
        return MOV(*v.template get_as<T>());
    }
    // 2
    template<typename T, typename... Types>
        requires ((std::is_same_v<T, Types> + ...) == 1)
    inline constexpr auto get(variant<Types...>& v) -> T&
    {
        return get<impl::index_of<T, Types...>>(v);
    }
    template<typename T, typename... Types>
        requires ((std::is_same_v<T, Types> + ...) == 1)
    inline constexpr auto get(variant<Types...>&& v) -> T&&
    {
        return get<impl::index_of<T, Types...>>(MOV(v));
    }
    template<typename T, typename... Types>
        requires ((std::is_same_v<T, Types> + ...) == 1)
    inline constexpr auto get(variant<Types...> const& v) -> T const&
    {
        return get<impl::index_of<T, Types...>>(v);
    }
    template<typename T, typename... Types>
        requires ((std::is_same_v<T, Types> + ...) == 1)
    inline constexpr auto get(variant<Types...> const&& v) -> T const&&
    {
        return get<impl::index_of<T, Types...>>(MOV(v));
    }

    // get if
    // 1
    template<std::size_t I, typename... Types>
    inline constexpr auto get_if(variant<Types...>* v) noexcept
        -> std::add_pointer_t<variant_alternative_t<I, variant<Types...>>>
    {
        using T = variant_alternative_t<I, variant<Types...>>;
        return (v and v->index() == I) ? v->template get_as<T>() : nullptr;
    }
    template<std::size_t I, typename... Types>
    inline constexpr auto get_if(variant<Types...> const* v) noexcept
        -> std::add_pointer_t<variant_alternative_t<I, variant<Types...>> const>
    {
        using T = variant_alternative_t<I, variant<Types...>>;
        return (v and v->index() == I) ? v->template get_as<T const>() : nullptr;
    }
    // 2
    template<typename T, typename... Types>
        requires ((std::is_same_v<T, Types> + ...) == 1)
    inline constexpr auto get_if(variant<Types...>* v) noexcept -> std::add_pointer_t<T>
    {
        return get_if<impl::index_of<T, Types...>>(v);
    }
    template<typename T, typename... Types>
        requires ((std::is_same_v<T, Types> + ...) == 1)
    inline constexpr auto get_if(variant<Types...> const* v) noexcept -> std::add_pointer_t<T const>
    {
        return get_if<impl::index_of<T, Types...>>(v);
    }

    namespace impl {
        template<typename Visitor, typename... Variants>
        class visitor_common {
            template<std::size_t... S>
            static constexpr auto get_full_size(std::index_sequence<S...>)
            {
                return (S * ... * 1);
            }

            template<std::size_t I, std::size_t... S>
            static constexpr auto divisor_for(std::index_sequence<S...>)
            {
                std::size_t i = 0, d = 1;
                (((i < I) ? (d *= S, ++i, true) : false) and ...);
                return d;
            }

            template<std::size_t I, std::size_t... S>
            static constexpr auto modulus_for(std::index_sequence<S...>)
            {
                std::size_t i = 0, m = 1;
                (((i <= I) ? (m *= S, ++i, true) : false) and ...);
                return m;
            }

            template<std::size_t... I>
            static constexpr auto get_divisors(std::index_sequence<I...>)
            {
                return std::index_sequence<divisor_for<I>(sizes)...>();
            }

            template<std::size_t... I>
            static constexpr auto get_moduli(std::index_sequence<I...>)
            {
                return std::index_sequence<modulus_for<I>(sizes)...>();
            }

            template<std::size_t I, typename Variant>
            static constexpr auto get_unchecked(Variant&& v) noexcept -> decltype(auto)
            {
                using T = variant_alternative_t<I, std::remove_reference_t<Variant>>;
                using U = std::conditional_t<std::is_const_v<T>, T const, T>;
                using V = std::conditional_t<std::is_lvalue_reference_v<Variant&&>, U&, U&&>;

                return reinterpret_cast<V>(v);
            }

        public:
            static constexpr auto sizes = std::index_sequence<variant_size_v<std::remove_reference_t<Variants>>...>();
            static constexpr auto indexes = std::index_sequence_for<Variants...>();
            static constexpr auto full_size = get_full_size(sizes);
            static constexpr auto full_indexes = std::make_index_sequence<full_size>();
            static constexpr auto divisors = get_divisors(indexes);
            static constexpr auto moduli = get_moduli(indexes);

            template<std::size_t I, std::size_t... Ds, std::size_t... Ms>
            static constexpr auto visit_at_common(
                Visitor&& vis, Variants&&... vars, std::index_sequence<Ds...>, std::index_sequence<Ms...>)
                -> decltype(auto)
            {
                return std::invoke(FWD(vis),
                    get_unchecked<(I % Ms) / Ds>(FWD(vars))...);
                    //get<(I % Ms) / Ds>(FWD(vars))...);
            }
        };

        template<typename Visitor, typename... Variants>
        struct visitor: visitor_common<Visitor, Variants...> {
            using result_type = std::invoke_result_t<Visitor, variant_alternative_t<0, std::remove_reference_t<Variants>>...>;

        private:
            template<typename R, std::size_t I, std::size_t... Ds, std::size_t... Ms>
            static constexpr auto is_same_result(std::index_sequence<Ds...>, std::index_sequence<Ms...>)
            {
                return std::is_same_v<R, std::invoke_result_t<Visitor, variant_alternative_t<(I % Ms) / Ds, std::remove_reference_t<Variants>>...>>;
            }

            template<typename R, std::size_t... I>
            static constexpr auto are_same_results(std::index_sequence<I...>)
            {
                return (is_same_result<R, I>(visitor::divisors, visitor::moduli) and ...);
            }

            template<std::size_t I>
            static constexpr auto visit_at(Visitor&& vis, Variants&&... vars) -> result_type
            {
                return visitor::template visit_at_common<I>(FWD(vis), FWD(vars)..., visitor::divisors, visitor::moduli);
            }

        public:
            static_assert(are_same_results<result_type>(visitor::full_indexes));

            static constexpr auto visitors = []<std::size_t... I>(std::index_sequence<I...>) {
                return std::array{ &visit_at<I>... }; }(visitor::full_indexes);
        };

        template<typename R, typename Visitor, typename... Variants>
        struct visitor_r: visitor_common<Visitor, Variants...> {
            using result_type = R;

        private:
            template<std::size_t I>
            static constexpr auto visit_at(Visitor&& vis, Variants&&... vars) -> result_type
            {
                return visitor_r::template visit_at_common<I>(FWD(vis), FWD(vars)..., visitor_r::divisors, visitor_r::moduli);
            }

        public:
            static constexpr auto visitors = []<std::size_t... I>(std::index_sequence<I...>) {
                return std::array{ &visit_at<I>... }; }(visitor_r::full_indexes);
        };
    }

    // visit
    // 1
    template<typename Visitor, typename... Variants>
    inline constexpr auto visit(Visitor&& vis, Variants&&... vars) -> decltype(auto)
    {
        if ((vars.valueless_by_exception() or ...))
            throw bad_variant_access();

        using visitor = impl::visitor<Visitor, Variants...>;
        auto index = [&]<std::size_t... D>(std::index_sequence<D...>) {
            return ((vars.index() * D) + ... + 0); }(visitor::divisors);
        return visitor::visitors[index](FWD(vis), FWD(vars)...);
    }
    // 2
    template<typename R, typename Visitor, typename... Variants>
    inline constexpr auto visit(Visitor&& vis, Variants&&... vars) -> R
    {
        if ((vars.valueless_by_exception() or ...))
            throw bad_variant_access();

        using visitor = impl::visitor_r<R, Visitor, Variants...>;
        auto index = [&]<std::size_t... D>(std::index_sequence<D...>) {
            return ((vars.index() * D) + ... + 0); }(visitor::divisors);
        return visitor::visitors[index](FWD(vis), FWD(vars)...);
    }

    // comparison
    template<typename... Types>
    inline constexpr bool operator==(variant<Types...> const& a, variant<Types...> const& b)
    {
        return a.index() != b.index() ? false
             : a.valueless_by_exception() ? true
             : visit(std::equal_to(), a, b);
    }
    template<typename... Types>
    inline constexpr bool operator!=(variant<Types...> const& a, variant<Types...> const& b)
    {
        return a.index() != b.index() ? true
             : a.valueless_by_exception() ? false
             : visit(std::not_equal_to(), a, b);
    }
    template<typename... Types>
    inline constexpr bool operator< (variant<Types...> const& a, variant<Types...> const& b)
    {
        return b.valueless_by_exception() ? false
             : a.valueless_by_exception() ? true
             : a.index() < b.index() ? true
             : a.index() > b.index() ? false
             : visit(std::less(), a, b);
    }
    template<typename... Types>
    inline constexpr bool operator<=(variant<Types...> const& a, variant<Types...> const& b)
    {
        return a.valueless_by_exception() ? true
             : b.valueless_by_exception() ? false
             : a.index() < b.index() ? true
             : a.index() > b.index() ? false
             : visit(std::less_equal(), a, b);
    }
    template<typename... Types>
    inline constexpr bool operator> (variant<Types...> const& a, variant<Types...> const& b)
    {
        return a.valueless_by_exception() ? false
             : b.valueless_by_exception() ? true
             : a.index() > b.index() ? true
             : a.index() < b.index() ? false
             : visit(std::greater(), a, b);
    }
    template<typename... Types>
    inline constexpr bool operator>=(variant<Types...> const& a, variant<Types...> const& b)
    {
        return b.valueless_by_exception() ? true
             : a.valueless_by_exception() ? false
             : a.index() > b.index() ? true
             : a.index() < b.index() ? false
             : visit(std::greater_equal(), a, b);
    }

    template<typename... Types>
    inline constexpr auto operator<=>(variant<Types...> const& a, variant<Types...> const& b)
        -> std::common_comparison_category_t<std::compare_three_way_result_t<Types>...>
    {
        return a.valueless_by_exception() and b.valueless_by_exception() ? std::strong_ordering::equal
             : a.valueless_by_exception() ? std::strong_ordering::less
             : b.valueless_by_exception() ? std::strong_ordering::greater
             : a.index() != b.index() ? (a.index() <=> b.index())
             : visit(std::compare_three_way(), a, b);
    }

    // swap
    template<typename... Types>
    inline constexpr void swap(variant<Types...>& a, variant<Types...>& b) noexcept(noexcept(a.swap(b)))
        requires ((std::is_move_constructible_v<Types> and std::is_swappable_v<Types>) and ...)
    {
        a.swap(b);
    }
}

#undef MOV
#undef FWD
