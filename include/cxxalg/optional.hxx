#pragma once

#include "impl/common.hxx"

#include <compare>
#include <exception>
#include <initializer_list>
#include <memory>
#include <type_traits>
#include <utility>

#include <cstddef>

#define MOV(...) static_cast<std::remove_reference_t<decltype(__VA_ARGS__)>&&>(__VA_ARGS__)
#define FWD(...) static_cast<decltype(__VA_ARGS__)&&>(__VA_ARGS__)

namespace cxxalg {
    struct bad_optional_access: std::exception {
        using std::exception::exception;
        auto what() const noexcept -> char const* override { return "bad_optional_access"; }
    };

    template<typename T>
    struct tombstone_traits {
        static constexpr auto spare_representations = std::size_t(0);
        static constexpr auto index(T const*) { return std::size_t(-1); }
        static constexpr void set_spare_representation(T*, std::size_t) = delete;
    };

    namespace impl { enum class nullopt_init { }; }
    struct nullopt_t { explicit constexpr nullopt_t(impl::nullopt_init) { } };
    inline constexpr auto nullopt = nullopt_t(impl::nullopt_init{});

    template<typename T, typename Traits = tombstone_traits<T>>
    class optional {
        static constexpr auto smallopt_ = (Traits::spare_representations != 0);
        alignas(T) std::byte buffer_[sizeof(T) + std::size_t(1 - smallopt_)] = {};

    public:
        using value_type = T;

        // (destructor)
        constexpr ~optional() requires impl::trivially_destructible<T> = default;
        constexpr ~optional()
        {
            if (engaged())
                std::destroy_at(get());
        }

        // (constructor)
        // 1
        constexpr optional() noexcept = default;
        constexpr optional(nullopt_t) noexcept { }
        // 2
        constexpr optional(optional const&) requires impl::trivially_copy_constructible<T> = default;
        constexpr optional(optional const& that) requires impl::copy_constructible<T>
        {
            if (that.has_value()) {
                std::construct_at(get(), *that);
                set_engaged();
            } else {
                set_disengaged();
            }
        }
        // 3
        constexpr optional(optional&&) requires impl::trivially_move_constructible<T> = default;
        constexpr optional(optional&& that) noexcept(std::is_nothrow_move_constructible_v<T>)
            requires impl::move_constructible<T>
        {
            if (that.has_value()) {
                std::construct_at(get(), MOV(*that));
                set_engaged();
            } else {
                set_disengaged();
            }
        }
        // 4
        template<typename U>
        explicit(not std::is_convertible_v<U const&, T>)
        constexpr optional(optional<U> const& that)
            requires std::is_constructible_v<T, U const&>
                 and (not std::is_constructible_v<T, optional<U>&       >)
                 and (not std::is_constructible_v<T, optional<U> const& >)
                 and (not std::is_constructible_v<T, optional<U>&&      >)
                 and (not std::is_constructible_v<T, optional<U> const&&>)
                 and (not std::is_convertible_v<optional<U>&,        T>)
                 and (not std::is_convertible_v<optional<U> const&,  T>)
                 and (not std::is_convertible_v<optional<U>&&,       T>)
                 and (not std::is_convertible_v<optional<U> const&&, T>)
        {
            if (that.has_value()) {
                std::construct_at(get(), *that);
                set_engaged();
            } else {
                set_disengaged();
            }
        }
        // 5
        template<typename U>
        explicit(not std::is_convertible_v<U&&, T>)
        constexpr optional(optional<U>&& that)
            requires std::is_constructible_v<T, U&&>
                 and (not std::is_constructible_v<T, optional<U>&       >)
                 and (not std::is_constructible_v<T, optional<U> const& >)
                 and (not std::is_constructible_v<T, optional<U>&&      >)
                 and (not std::is_constructible_v<T, optional<U> const&&>)
                 and (not std::is_convertible_v<optional<U>&,        T>)
                 and (not std::is_convertible_v<optional<U> const&,  T>)
                 and (not std::is_convertible_v<optional<U>&&,       T>)
                 and (not std::is_convertible_v<optional<U> const&&, T>)
        {
            if (that.has_value()) {
                std::construct_at(get(), MOV(*that));
                set_engaged();
            } else {
                set_disengaged();
            }
        }
        // 6
        template<typename... Args>
        constexpr explicit optional(std::in_place_t, Args&&... args)
            requires std::is_constructible_v<T, Args...>
        {
            std::construct_at(get(), FWD(args)...);
            set_engaged();
        }
        // 7
        template<typename U, typename... Args>
        constexpr explicit optional(std::in_place_t, std::initializer_list<U> il,  Args&&... args)
            requires std::is_constructible_v<T, std::initializer_list<U>&, Args...>
        {
            std::construct_at(get(), il, FWD(args)...);
            set_engaged();
        }
        // 8
        template<typename U = T>
        explicit(not std::is_convertible_v<U&&, T>)
        constexpr optional(U&& value)
            requires std::is_constructible_v<T, U&&>
                 and (not std::is_same_v<std::remove_cvref_t<U>, std::in_place_t>)
                 and (not std::is_same_v<std::remove_cvref_t<U>, optional>)
        {
            std::construct_at(get(), FWD(value));
            set_engaged();
        }

        // operator=
        // 1
        constexpr auto operator=(nullopt_t) noexcept -> optional&
        {
            reset();
            return *this;
        }
        // 2
        constexpr auto operator=(optional const&) noexcept -> optional&
            requires impl::trivially_copy_assignable<T> = default;
        constexpr auto operator=(optional const& that) noexcept -> optional&
            requires impl::copy_assignable<T>
        {
            if (this == &that) [[unlikely]]
                return *this;

            switch (has_value() | that.has_value() << 1) {
            case 0:
            default:
                // Do nothing
                break;
            case 1:
                reset();
                break;
            case 2:
                emplace(*that);
                break;
            case 3:
                *get() = *that;
                break;
            }
            return *this;
        }
        // 3
        constexpr auto operator=(optional&&) noexcept -> optional&
            requires impl::trivially_move_assignable<T> = default;
        constexpr auto operator=(optional&& that)
            noexcept(std::is_nothrow_move_assignable_v<T> and std::is_nothrow_move_constructible_v<T>) -> optional&
            requires impl::move_assignable<T>
        {
            if (this == &that) [[unlikely]]
                return *this;

            switch (has_value() | that.has_value() << 1) {
            case 0:
            default:
                // Do nothing
                break;
            case 1:
                reset();
                break;
            case 2:
                emplace(MOV(*that));
                break;
            case 3:
                *get() = MOV(*that);
                break;
            }
            return *this;
        }
        // 4
        template<typename U = T>
        constexpr auto operator=(U&& value) -> optional&
            requires std::is_constructible_v<T, U> and std::is_assignable_v<T&, U>
                 and (not std::is_same_v<std::remove_cvref_t<U>, optional>)
                 and (not std::is_scalar_v<T> or not std::is_same_v<std::decay_t<U>, T>)
        {
            if (engaged()) {
                *get() = FWD(value);
            } else {
                std::construct_at(get(), FWD(value));
                set_engaged();
            }
            return *this;
        }
        // 5
        template<typename U>
        constexpr auto operator=(optional<U> const& that) -> optional&
            requires std::is_constructible_v<T, U const&> and std::is_assignable_v<T&, U const&>
                 and (not std::is_constructible_v<T, optional<U>&       >)
                 and (not std::is_constructible_v<T, optional<U> const& >)
                 and (not std::is_constructible_v<T, optional<U>&&      >)
                 and (not std::is_constructible_v<T, optional<U> const&&>)
                 and (not std::is_convertible_v<optional<U>&,        T>)
                 and (not std::is_convertible_v<optional<U> const&,  T>)
                 and (not std::is_convertible_v<optional<U>&&,       T>)
                 and (not std::is_convertible_v<optional<U> const&&, T>)
                 and (not std::is_assignable_v<T&, optional<U>&       >)
                 and (not std::is_assignable_v<T&, optional<U> const& >)
                 and (not std::is_assignable_v<T&, optional<U>&&      >)
                 and (not std::is_assignable_v<T&, optional<U> const&&>)
        {
            switch (has_value() | that.has_value() << 1) {
            case 0:
            default:
                // Do nothing
                break;
            case 1:
                reset();
                break;
            case 2:
                emplace(*that);
                break;
            case 3:
                *get() = *that;
                break;
            }
            return *this;
        }
        // 6
        template<typename U>
        constexpr auto operator=(optional<U>&& that) -> optional&
            requires std::is_constructible_v<T, U> and std::is_assignable_v<T&, U>
                 and (not std::is_constructible_v<T, optional<U>&       >)
                 and (not std::is_constructible_v<T, optional<U> const& >)
                 and (not std::is_constructible_v<T, optional<U>&&      >)
                 and (not std::is_constructible_v<T, optional<U> const&&>)
                 and (not std::is_convertible_v<optional<U>&,        T>)
                 and (not std::is_convertible_v<optional<U> const&,  T>)
                 and (not std::is_convertible_v<optional<U>&&,       T>)
                 and (not std::is_convertible_v<optional<U> const&&, T>)
                 and (not std::is_assignable_v<T&, optional<U>&       >)
                 and (not std::is_assignable_v<T&, optional<U> const& >)
                 and (not std::is_assignable_v<T&, optional<U>&&      >)
                 and (not std::is_assignable_v<T&, optional<U> const&&>)
        {
            switch (has_value() | that.has_value() << 1) {
            case 0:
            default:
                // Do nothing
                break;
            case 1:
                reset();
                break;
            case 2:
                emplace(MOV(*that));
                break;
            case 3:
                *get() = MOV(*that);
                break;
            }
            return *this;
        }

        // operator->
        // operator*
        constexpr auto operator->() const  noexcept -> T const*  { return get(); }
        constexpr auto operator->()        noexcept -> T*        { return get(); }
        constexpr auto operator*() const&  noexcept -> T const&  { return *get(); }
        constexpr auto operator*() &       noexcept -> T&        { return *get(); }
        constexpr auto operator*() const&& noexcept -> T const&& { return MOV(*get()); }
        constexpr auto operator*() &&      noexcept -> T&&       { return MOV(*get()); }

        // operator bool
        // has_value
        constexpr explicit operator bool() const noexcept { return engaged(); }
        constexpr bool has_value() const noexcept { return engaged(); }

        // value
        constexpr auto value() & -> T&              { if (not has_value()) [[unlikely]] throw bad_optional_access(); return *get(); }
        constexpr auto value() const& -> T const&   { if (not has_value()) [[unlikely]] throw bad_optional_access(); return *get(); }
        constexpr auto value() && -> T&&            { if (not has_value()) [[unlikely]] throw bad_optional_access(); return MOV(*get()); }
        constexpr auto value() const&& -> T const&& { if (not has_value()) [[unlikely]] throw bad_optional_access(); return MOV(*get()); }

        // value_or
        template<typename U>
        constexpr auto value_or(U&& that) const& -> T { return has_value() ? *get()      : FWD(that); }
        template<typename U>
        constexpr auto value_or(U&& that) &&     -> T { return has_value() ? MOV(*get()) : FWD(that); }

        // and_then
        template<typename F>
        constexpr auto and_then(F&& f) &
        {
            return has_value() ? std::invoke(FWD(f), *get()) : std::remove_cvref_t<std::invoke_result_t<F, T&>>();
        }
        template<typename F>
        constexpr auto and_then(F&& f) const&
        {
            return has_value() ? std::invoke(FWD(f), *get()) : std::remove_cvref_t<std::invoke_result_t<F, T const&>>();
        }
        template<typename F>
        constexpr auto and_then(F&& f) &&
        {
            return has_value() ? std::invoke(FWD(f), MOV(*get())) : std::remove_cvref_t<std::invoke_result_t<F, T&&>>();
        }
        template<typename F>
        constexpr auto and_then(F&& f) const&&
        {
            return has_value() ? std::invoke(FWD(f), MOV(*get())) : std::remove_cvref_t<std::invoke_result_t<F, T const&&>>();
        }

        // transform
        template<typename F>
        constexpr auto transform(F&& f) &
        {
            using U = std::remove_cv_t<std::invoke_result_t<F, T&>>;
            return has_value() ? optional<U>(std::in_place, std::invoke(FWD(f), *get())) : nullopt;
        }
        template<typename F>
        constexpr auto transform(F&& f) const&
        {
            using U = std::remove_cv_t<std::invoke_result_t<F, T const&>>;
            return has_value() ? optional<U>(std::in_place, std::invoke(FWD(f), *get())) : nullopt;
        }
        template<typename F>
        constexpr auto transform(F&& f) &&
        {
            using U = std::remove_cv_t<std::invoke_result_t<F, T>>;
            return has_value() ? optional<U>(std::in_place, std::invoke(FWD(f), MOV(*get()))) : nullopt;
        }
        template<typename F>
        constexpr auto transform(F&& f) const&&
        {
            using U = std::remove_cv_t<std::invoke_result_t<F, T const>>;
            return has_value() ? optional<U>(std::in_place, std::invoke(FWD(f), MOV(*get()))) : nullopt;
        }

        // or_else
        template<typename F>
        constexpr auto or_else(F&& f) const& -> optional
            requires std::copy_constructible<T> and std::invocable<F>
        {
            return has_value() ? *this : FWD(f)();
        }
        template<typename F>
        constexpr auto or_else(F&& f) && -> optional
            requires std::move_constructible<T> and std::invocable<F>
        {
            return has_value() ? MOV(*this) : FWD(f)();
        }

        // swap
        constexpr void swap(optional& that)
            noexcept(std::is_nothrow_move_constructible_v<T> and std::is_nothrow_swappable_v<T>)
        {
            switch (has_value() | that.has_value() << 1) {
            case 0:
            default:
                // Do nothing;
                break;
            case 1:
                that = MOV(*get());
                reset();
                break;
            case 2:
                *this = MOV(*that);
                that.reset();
                break;
            case 3:
                using std::swap;
                swap(*get(), *that);
                break;
            }
        }

        // reset
        constexpr void reset() noexcept
        {
            if (engaged()) {
                std::destroy_at(get());
                set_disengaged();
            }
        }

        // emplace
        // 1
        template<typename... Args>
        constexpr auto emplace(Args&&... args) -> T&
        {
            reset();
            std::construct_at(get(), FWD(args)...);
            set_engaged();
            return *get();
        }
        // 2
        template<typename U, typename... Args>
        constexpr auto emplace(std::initializer_list<U> il, Args&&... args) -> T&
        {
            reset();
            std::construct_at(get(), il, FWD(args)...);
            set_engaged();
            return *get();
        }

    private:
        constexpr auto get()       noexcept { return reinterpret_cast<T*      >(&buffer_); }
        constexpr auto get() const noexcept { return reinterpret_cast<T const*>(&buffer_); }

        constexpr auto engaged() const noexcept -> bool
        {
            if constexpr (smallopt_)
                return Traits::index(get()) == std::size_t(-1);
            else
                return (bool)buffer_[sizeof(T)];
        }

        constexpr void set_engaged() noexcept
        {
            if constexpr (not smallopt_)
                buffer_[sizeof(T)] = (std::byte)true;
        }

        constexpr void set_disengaged() noexcept
        {
            if constexpr (smallopt_)
                Traits::set_spare_representation(get(), 0);
            else
                buffer_[sizeof(T)] = (std::byte)false;
        }
    };

    // swap
    template<typename T>
    inline constexpr void swap(optional<T>& a, optional<T>& b) noexcept(noexcept(a.swap(b)))
        requires std::is_move_constructible_v<T> and std::is_swappable_v<T>
    {
        a.swap(b);
    }

    // make_optional
    // 1
    template<typename T>
    inline constexpr auto make_optional(T&& value) -> optional<std::decay_t<T>>
    {
        return optional<std::decay_t<T>>(FWD(value));
    }
    // 2
    template<typename T, typename... Args>
    inline constexpr auto make_optional(Args&&... args) -> optional<T>
    {
        return optional<T>(std::in_place, FWD(args)...);
    }
    // 3
    template<typename T, typename U, typename... Args>
    inline constexpr auto make_optional(std::initializer_list<U> il, Args&&... args) -> optional<T>
    {
        return optional<T>(std::in_place, il, FWD(args)...);
    }

    // comparison
    // compare two optional objects
    template<typename T, typename U>
    inline constexpr bool operator==(optional<T> const& a, optional<U> const& b)
    {
        return a.has_value() != b.has_value() ? false
             : a.has_value() ? (*a == *b)
             : true;
    }
    template<typename T, typename U>
    inline constexpr bool operator!=(optional<T> const& a, optional<U> const& b)
    {
        return a.has_value() != b.has_value() ? true
             : a.has_value() ? (*a != *b)
             : false;
    }
    template<typename T, typename U>
    inline constexpr bool operator< (optional<T> const& a, optional<U> const& b)
    {
        return not b.has_value() ? false
             : not a.has_value() ? true
             : (*a < *b);
    }
    template<typename T, typename U>
    inline constexpr bool operator<=(optional<T> const& a, optional<U> const& b)
    {
        return not a.has_value() ? true
             : not b.has_value() ? false
             : (*a <= *b);
    }
    template<typename T, typename U>
    inline constexpr bool operator> (optional<T> const& a, optional<U> const& b)
    {
        return not a.has_value() ? false
             : not b.has_value() ? true
             : (*a > *b);
    }
    template<typename T, typename U>
    inline constexpr bool operator>=(optional<T> const& a, optional<U> const& b)
    {
        return not b.has_value() ? true
             : not a.has_value() ? false
             : (*a >= *b);
    }
    template<typename T, std::three_way_comparable_with<T> U>
    inline constexpr auto operator<=>(optional<T> const& a, optional<U> const& b) -> std::compare_three_way_result_t<T, U>
    {
        return a.has_value() and b.has_value ? (*a <=> *b)
             : a.has_value() <=> b.has_value();
    }
    // compare an optional object with a nullopt
    template<typename T>
    inline constexpr bool operator==(optional<T> const& o, nullopt_t) noexcept
    {
        return not o.has_value();
    }
    template<typename T>
    inline constexpr auto operator<=>(optional<T> const& o, nullopt_t) noexcept -> std::strong_ordering
    {
        return o.has_value() <=> false;
    }
    // compare an optional object with a value
    template<typename T, typename U>
    inline constexpr bool operator==(optional<T> const& o, U const& val)
    {
        return o.has_value() ? *o == val : false;
    }
    template<typename T, typename U>
    inline constexpr bool operator==(T const& val, optional<U> const& o)
    {
        return o.has_value() ? val == *o : false;
    }
    template<typename T, typename U>
    inline constexpr bool operator!=(optional<T> const& o, U const& val)
    {
        return o.has_value() ? *o != val : true;
    }
    template<typename T, typename U>
    inline constexpr bool operator!=(T const& val, optional<U> const& o)
    {
        return o.has_value() ? val != *o : true;
    }
    template<typename T, typename U>
    inline constexpr bool operator< (optional<T> const& o, U const& val)
    {
        return o.has_value() ? *o < val : true;
    }
    template<typename T, typename U>
    inline constexpr bool operator< (T const& val, optional<U> const& o)
    {
        return o.has_value() ? val < *o : false;
    }
    template<typename T, typename U>
    inline constexpr bool operator<=(optional<T> const& o, U const& val)
    {
        return o.has_value() ? *o <= val : true;
    }
    template<typename T, typename U>
    inline constexpr bool operator<=(T const& val, optional<U> const& o)
    {
        return o.has_value() ? val <= *o : false;
    }
    template<typename T, typename U>
    inline constexpr bool operator> (optional<T> const& o, U const& val)
    {
        return o.has_value() ? *o > val : false;
    }
    template<typename T, typename U>
    inline constexpr bool operator> (T const& val, optional<U> const& o)
    {
        return o.has_value() ? val > *o : true;
    }
    template<typename T, typename U>
    inline constexpr bool operator>=(optional<T> const& o, U const& val)
    {
        return o.has_value() ? *o >= val : false;
    }
    template<typename T, typename U>
    inline constexpr bool operator>=(T const& val, optional<U> const& o)
    {
        return o.has_value() ? val >= *o : true;
    }

    template<typename T, std::three_way_comparable_with<T> U>
    inline constexpr auto operator<=>(optional<T> const& o, U const& val) -> std::compare_three_way_result_t<T, U>
    {
        return o.has_value() ? *o <=> val : std::strong_ordering::less;
    }

    template<>
    struct tombstone_traits<bool> {
        static constexpr auto spare_representations = std::size_t(254);
        static constexpr auto index(bool const* p)
        {
            auto ch = *(unsigned char*)p;
            return std::size_t((ch >= 2) ? (ch - 2) : -1);
        }
        static constexpr void set_spare_representation(bool* p, std::size_t i)
        {
            *(unsigned char*)p = i + 2;
        }
    };

    template<typename T>
    struct tombstone_traits<optional<T>> {

        static constexpr auto spare_representations =
            (tombstone_traits<T>::spare_representations != 0)
            ? (tombstone_traits<T>::spare_representations - 1)
            : tombstone_traits<bool>::spare_representations;

        static constexpr auto index(optional<T> const* p)
        {
            if constexpr (tombstone_traits<T>::spare_representations != 0) {
                auto i = tombstone_traits<T>::index(&(**p));
                return (i == std::size_t(-1) or i == 0) ? std::size_t(-1) : (i - 1);
            } else {
                return tombstone_traits<bool>::index(&(**p));
            }
        }

        static constexpr void set_spare_representation(optional<T>* p, std::size_t i)
        {
            if constexpr (tombstone_traits<T>::spare_representations != 0)
                tombstone_traits<T>::set_spare_representation(&(**p), i + 1);
            else
                tombstone_traits<bool>::set_spare_representation(&(**p), i);
        }
    };
}

#undef MOV
#undef FWD
