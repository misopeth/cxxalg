#pragma once

#include "impl/common.hxx"

#include <compare>
#include <exception>
#include <functional>
#include <initializer_list>
#include <memory>
#include <type_traits>
#include <utility>

#include <cstddef>

namespace cxxalg {
    struct bad_optional_access: std::exception {
        using std::exception::exception;
        auto what() const noexcept -> char const* override { return "bad_optional_access"; }
    };

    template<typename T>
    struct tombstone_traits {
        static constexpr auto spare_representations = std::size_t(0);
        static constexpr auto index(T const*) -> std::size_t = delete;
        static constexpr void set_spare_representation(T*, std::size_t) = delete;
    };

    namespace impl { enum class nullopt_init { }; }
    struct nullopt_t { explicit constexpr nullopt_t(impl::nullopt_init) { } };
    inline constexpr auto nullopt = nullopt_t(impl::nullopt_init{});

    namespace impl {
        template<typename T, typename Traits, bool SmallOpt>
        class optional_base {
            alignas(T) std::byte storage_[sizeof(T)] = {};
            bool engaged_ = false;

        public:
            constexpr auto get()       noexcept { return reinterpret_cast<T*      >(&storage_); }
            constexpr auto get() const noexcept { return reinterpret_cast<T const*>(&storage_); }

            constexpr auto engaged() const noexcept -> bool
            {
                return engaged_;
            }

            constexpr void set_engaged() noexcept
            {
                engaged_ = true;
            }

            constexpr void set_disengaged() noexcept
            {
                engaged_ = false;
            }
        };

        template<typename T, typename Traits>
        class optional_base<T, Traits, true> {
            alignas(T) std::byte storage_[sizeof(T)] = {};

        public:
            constexpr auto get()       noexcept { return reinterpret_cast<T*      >(&storage_); }
            constexpr auto get() const noexcept { return reinterpret_cast<T const*>(&storage_); }

            constexpr auto engaged() const noexcept -> bool
            {
                return Traits::index(get()) == std::size_t(-1);
            }

            constexpr void set_engaged() noexcept { }

            constexpr void set_disengaged() noexcept
            {
                Traits::set_spare_representation(get(), 0);
            }
        };
    }

    template<typename T, typename Traits = tombstone_traits<T>>
    class optional: impl::optional_base<T, Traits, Traits::spare_representations != 0> {
    public:
        using value_type = T;
        using traits_type = Traits;

        // (destructor)
        constexpr ~optional() requires impl::trivially_destructible<T> = default;
        constexpr ~optional()
        {
            if (this->engaged())
                std::destroy_at(this->get());
        }

        // (constructor)
        // 1
        constexpr optional() noexcept { this->set_disengaged(); }
        constexpr optional(nullopt_t) noexcept { this->set_disengaged(); }
        // 2
        constexpr optional(optional const&) requires impl::trivially_copy_constructible<T> = default;
        constexpr optional(optional const& that) requires impl::copy_constructible<T>
        {
            if (that.has_value()) {
                std::construct_at(this->get(), *that);
                this->set_engaged();
            } else {
                this->set_disengaged();
            }
        }
        // 3
        constexpr optional(optional&&) requires impl::trivially_move_constructible<T> = default;
        constexpr optional(optional&& that) noexcept(std::is_nothrow_move_constructible_v<T>)
            requires impl::move_constructible<T>
        {
            if (that.has_value()) {
                std::construct_at(this->get(), std::move(*that));
                this->set_engaged();
            } else {
                this->set_disengaged();
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
                std::construct_at(this->get(), *that);
                this->set_engaged();
            } else {
                this->set_disengaged();
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
                std::construct_at(this->get(), std::move(*that));
                this->set_engaged();
            } else {
                this->set_disengaged();
            }
        }
        // 6
        template<typename... Args>
        constexpr explicit optional(std::in_place_t, Args&&... args)
            requires std::is_constructible_v<T, Args...>
        {
            std::construct_at(this->get(), std::forward<Args>(args)...);
            this->set_engaged();
        }
        // 7
        template<typename U, typename... Args>
        constexpr explicit optional(std::in_place_t, std::initializer_list<U> il,  Args&&... args)
            requires std::is_constructible_v<T, std::initializer_list<U>&, Args...>
        {
            std::construct_at(this->get(), il, std::forward<Args>(args)...);
            this->set_engaged();
        }
        // 8
        template<typename U = T>
        explicit(not std::is_convertible_v<U&&, T>)
        constexpr optional(U&& value)
            requires std::is_constructible_v<T, U&&>
                 and (not std::is_same_v<std::remove_cvref_t<U>, std::in_place_t>)
                 and (not std::is_same_v<std::remove_cvref_t<U>, optional>)
        {
            std::construct_at(this->get(), std::forward<U>(value));
            this->set_engaged();
        }

        // operator=
        // 1
        constexpr auto operator=(nullopt_t) noexcept -> optional&
        {
            this->reset();
            return *this;
        }
        // 2
        constexpr auto operator=(optional const&) noexcept -> optional&
            requires impl::trivially_copy_assignable<T> = default;
        constexpr auto operator=(optional const& that) noexcept -> optional&
            requires impl::copy_assignable<T>
        {
            switch (this->has_value() | that.has_value() << 1) {
            case 0:
            default:
                // Do nothing
                break;
            case 1:
                this->reset();
                break;
            case 2:
                this->emplace(*that);
                break;
            case 3:
                *this->get() = *that;
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
            switch (this->has_value() | that.has_value() << 1) {
            case 0:
            default:
                // Do nothing
                break;
            case 1:
                this->reset();
                break;
            case 2:
                this->emplace(std::move(*that));
                break;
            case 3:
                *this->get() = std::move(*that);
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
            if (this->engaged()) {
                *this->get() = std::forward<U>(value);
            } else {
                std::construct_at(this->get(), std::forward<U>(value));
                this->set_engaged();
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
            switch (this->has_value() | that.has_value() << 1) {
            case 0:
            default:
                // Do nothing
                break;
            case 1:
                this->reset();
                break;
            case 2:
                this->emplace(*that);
                break;
            case 3:
                *this->get() = *that;
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
            switch (this->has_value() | that.has_value() << 1) {
            case 0:
            default:
                // Do nothing
                break;
            case 1:
                this->reset();
                break;
            case 2:
                this->emplace(std::move(*that));
                break;
            case 3:
                *this->get() = std::move(*that);
                break;
            }
            return *this;
        }

        // operator->
        // operator*
        constexpr auto operator->() const  noexcept -> T const*  { return this->get(); }
        constexpr auto operator->()        noexcept -> T*        { return this->get(); }
        constexpr auto operator*() const&  noexcept -> T const&  { return *this->get(); }
        constexpr auto operator*() &       noexcept -> T&        { return *this->get(); }
        constexpr auto operator*() const&& noexcept -> T const&& { return std::move(*this->get()); }
        constexpr auto operator*() &&      noexcept -> T&&       { return std::move(*this->get()); }

        // operator bool
        // has_value
        constexpr explicit operator bool() const noexcept { return this->engaged(); }
        constexpr bool has_value() const noexcept { return this->engaged(); }

        // value
        constexpr auto value() & -> T&              { if (not this->has_value()) [[unlikely]] throw bad_optional_access(); return *this->get(); }
        constexpr auto value() const& -> T const&   { if (not this->has_value()) [[unlikely]] throw bad_optional_access(); return *this->get(); }
        constexpr auto value() && -> T&&            { if (not this->has_value()) [[unlikely]] throw bad_optional_access(); return std::move(*this->get()); }
        constexpr auto value() const&& -> T const&& { if (not this->has_value()) [[unlikely]] throw bad_optional_access(); return std::move(*this->get()); }

        // value_or
        template<typename U>
        constexpr auto value_or(U&& that) const& -> T { return this->has_value() ? *this->get()            : std::forward<U>(that); }
        template<typename U>
        constexpr auto value_or(U&& that) &&     -> T { return this->has_value() ? std::move(*this->get()) : std::forward<U>(that); }

        // and_then
        template<typename F>
        constexpr auto and_then(F&& f) &
        {
            return this->has_value() ? std::invoke(std::forward<F>(f), *this->get()) : std::remove_cvref_t<std::invoke_result_t<F, T&>>();
        }
        template<typename F>
        constexpr auto and_then(F&& f) const&
        {
            return this->has_value() ? std::invoke(std::forward<F>(f), *this->get()) : std::remove_cvref_t<std::invoke_result_t<F, T const&>>();
        }
        template<typename F>
        constexpr auto and_then(F&& f) &&
        {
            return this->has_value() ? std::invoke(std::forward<F>(f), std::move(*this->get())) : std::remove_cvref_t<std::invoke_result_t<F, T&&>>();
        }
        template<typename F>
        constexpr auto and_then(F&& f) const&&
        {
            return this->has_value() ? std::invoke(std::forward<F>(f), std::move(*this->get())) : std::remove_cvref_t<std::invoke_result_t<F, T const&&>>();
        }

        // transform
        template<typename F>
        constexpr auto transform(F&& f) &
        {
            using U = std::remove_cv_t<std::invoke_result_t<F, T&>>;
            return this->has_value() ? optional<U>(std::in_place, std::invoke(std::forward<F>(f), *this->get())) : nullopt;
        }
        template<typename F>
        constexpr auto transform(F&& f) const&
        {
            using U = std::remove_cv_t<std::invoke_result_t<F, T const&>>;
            return this->has_value() ? optional<U>(std::in_place, std::invoke(std::forward<F>(f), *this->get())) : nullopt;
        }
        template<typename F>
        constexpr auto transform(F&& f) &&
        {
            using U = std::remove_cv_t<std::invoke_result_t<F, T>>;
            return this->has_value() ? optional<U>(std::in_place, std::invoke(std::forward<F>(f), std::move(*this->get()))) : nullopt;
        }
        template<typename F>
        constexpr auto transform(F&& f) const&&
        {
            using U = std::remove_cv_t<std::invoke_result_t<F, T const>>;
            return this->has_value() ? optional<U>(std::in_place, std::invoke(std::forward<F>(f), std::move(*this->get()))) : nullopt;
        }

        // or_else
        template<typename F>
        constexpr auto or_else(F&& f) const& -> optional
            requires std::copy_constructible<T> and std::invocable<F>
        {
            return this->has_value() ? *this : std::forward<F>(f)();
        }
        template<typename F>
        constexpr auto or_else(F&& f) && -> optional
            requires std::move_constructible<T> and std::invocable<F>
        {
            return this->has_value() ? std::move(*this) : std::forward<F>(f)();
        }

        // swap
        constexpr void swap(optional& that)
            noexcept(std::is_nothrow_move_constructible_v<T> and std::is_nothrow_swappable_v<T>)
        {
            switch (this->has_value() | that.has_value() << 1) {
            case 0:
            default:
                // Do nothing;
                break;
            case 1:
                that = std::move(*this->get());
                this->reset();
                break;
            case 2:
                *this = std::move(*that);
                that.reset();
                break;
            case 3:
                using std::swap;
                swap(*this->get(), *that);
                break;
            }
        }

        // reset
        constexpr void reset() noexcept
        {
            if (this->engaged()) {
                std::destroy_at(this->get());
                this->set_disengaged();
            }
        }

        // emplace
        // 1
        template<typename... Args>
        constexpr auto emplace(Args&&... args) -> T&
        {
            this->reset();
            std::construct_at(this->get(), std::forward<Args>(args)...);
            this->set_engaged();
            return *this->get();
        }
        // 2
        template<typename U, typename... Args>
        constexpr auto emplace(std::initializer_list<U> il, Args&&... args) -> T&
        {
            this->reset();
            std::construct_at(this->get(), il, std::forward<Args>(args)...);
            this->set_engaged();
            return *this->get();
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
        return optional<std::decay_t<T>>(std::forward<T>(value));
    }
    // 2
    template<typename T, typename... Args>
    inline constexpr auto make_optional(Args&&... args) -> optional<T>
    {
        return optional<T>(std::in_place, std::forward<Args>(args)...);
    }
    // 3
    template<typename T, typename U, typename... Args>
    inline constexpr auto make_optional(std::initializer_list<U> il, Args&&... args) -> optional<T>
    {
        return optional<T>(std::in_place, il, std::forward<Args>(args)...);
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

    template<typename T, typename Traits>
        requires (Traits::spare_representations != 0)
    struct tombstone_traits<optional<T, Traits>> {
        static constexpr auto spare_representations = Traits::spare_representations - 1;
        static constexpr auto index(optional<T, Traits> const* p) -> std::size_t
        {
            auto i = Traits::index(&(**p));
            return (i == std::size_t(-1) or i == 0) ? -1 : (i - 1);
        }
        static constexpr void set_spare_representation(optional<T, Traits>* p, std::size_t index)
        {
            Traits::set_spare_representation(&(**p), index + 1);
        }
    };

    template<typename T, typename Traits>
        requires (Traits::spare_representations == 0)
    struct tombstone_traits<optional<T, Traits>>: tombstone_traits<bool> { };
}
