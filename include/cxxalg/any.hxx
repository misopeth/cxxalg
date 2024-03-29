#pragma once

#include "impl/common.hxx"

#include <exception>
#include <initializer_list>
#include <type_traits>
#include <utility>

#include <cstddef>

namespace cxxalg {
    struct bad_any_cast: std::bad_cast {
        using bad_cast::bad_cast;
        auto what() const noexcept -> char const* override { return "bad_any_cast"; }
    };

    namespace impl {
        template<typename T>
        struct far_special_members {
            static constexpr auto get(void      * ptr) noexcept { return static_cast<T      *      *>(ptr); }
            static constexpr auto get(void const* ptr) noexcept { return static_cast<T const* const*>(ptr); }

            template<typename... Args>
            static constexpr auto construct(void* dst, Args&&... args)
                noexcept(std::is_nothrow_constructible_v<T, Args...>)
                requires std::is_constructible_v<T, Args...>
            {
                return *get(dst) = new T(std::forward<Args>(args)...);
            }

            template<typename U, typename... Args>
            static constexpr auto construct(void* dst, std::initializer_list<U> il, Args&&... args)
                noexcept(std::is_nothrow_constructible_v<T, std::initializer_list<U>&, Args...>)
                requires std::is_constructible_v<T, std::initializer_list<U>&, Args...>
            {
                return *get(dst) = new T(il, std::forward<Args>(args)...);
            }

            static constexpr void destroy(void* ptr) noexcept
            {
                delete *get(ptr);
            }

            static constexpr void copy_construct(void* dst, void const* src)
                noexcept(std::is_nothrow_copy_constructible_v<T>)
            {
                *get(dst) = new T(**get(src));
            }

            static constexpr void move_construct(void* dst, void* src)
                noexcept(std::is_nothrow_move_constructible_v<T>)
            {
                *get(dst) = *get(src);
                *get(src) = nullptr;
            }

            static constexpr void copy_assign(void* dst, void const* src)
                noexcept(std::is_nothrow_copy_assignable_v<T>)
            {
                **get(dst) = **get(src);
            }

            static constexpr void move_assign(void* dst, void* src)
                noexcept(std::is_nothrow_move_assignable_v<T>)
            {
                **get(dst) = std::move(**get(src));
            }

            static constexpr void swap(void* a, void* b)
                noexcept(std::is_nothrow_swappable_v<T>)
            {
                std::swap(*get(a), *get(b));
            }
        };

        inline constexpr auto any_sbo_size = 2 * sizeof(void*);
        inline constexpr auto any_sbo_align = alignof(std::max_align_t);

        template<typename T>
        inline constexpr bool any_uses_sbo = (sizeof(T) <= any_sbo_size) and (alignof(T) <= any_sbo_align);

        template<typename T>
        using any_members = std::conditional_t<any_uses_sbo<T>, special_members<T>, far_special_members<T>>;

        struct any_meta {
            std::type_info const& type;
            void (*destroy)(void*);
            void (*copy_construct)(void*, void const*);
            void (*move_construct)(void*, void*);
            void (*copy_assign)(void*, void const*);
            void (*move_assign)(void*, void*);
            void (*swap)(void*, void*);
        };
    }

    class any {
        alignas(impl::any_sbo_align) std::byte storage_[impl::any_sbo_size];
        impl::any_meta const* meta_ = nullptr;

    public:
        // (destructor)
        constexpr ~any()
        {
            if (meta_)
                meta_->destroy(storage_);
        }

        // (constructor)
        // 1
        constexpr any() noexcept: storage_{} { }
        // 2
        any(any const& that)
        {
            if (that.meta_) {
                that.meta_->copy_construct(storage_, that.storage_);
                meta_ = that.meta_;
            }
        }
        // 3
        any(any&& that)
        {
            if (that.meta_) {
                that.meta_->move_construct(storage_, that.storage_);
                meta_ = that.meta_;
            }
            that.reset();
        }
        // 4
        template<typename T, typename D = std::decay_t<T>>
        any(T&& value)
            requires std::is_copy_constructible_v<D>
                 and (not impl::is_in_place_type_v<D>)
                 and (not std::is_same_v<D, any>)
        {
            impl::any_members<D>::construct(storage_, std::forward<T>(value));
            meta_ = get_meta_for<D>();
        }
        // 5
        template<typename T, typename... Args, typename D = std::decay_t<T>>
        explicit any(std::in_place_type_t<T>, Args&&... args)
            requires std::is_constructible_v<D, Args...> and std::is_copy_constructible_v<D>
        {
            impl::any_members<D>::construct(storage_, std::forward<Args>(args)...);
            meta_ = get_meta_for<D>();
        }
        // 6
        template<typename T, typename U, typename... Args, typename D = std::decay_t<T>>
        explicit any(std::in_place_type_t<T>, std::initializer_list<U> il, Args&&... args)
            requires std::is_constructible_v<D, std::initializer_list<U>&, Args...>
                 and std::is_copy_constructible_v<D>
        {
            impl::any_members<D>::construct(storage_, il, std::forward<Args>(args)...);
            meta_ = get_meta_for<D>();
        }

        // operator=
        // 1
        auto operator=(any const& that) -> any&
        {
            any(that).swap(*this);
            return *this;
        }
        // 2
        auto operator=(any&& that) -> any&
        {
            any(std::move(that)).swap(*this);
            return *this;
        }
        // 3
        template<typename T, typename D = std::decay_t<T>>
        auto operator=(T&& value) -> any&
            requires (not std::is_same_v<D, any>) and std::is_copy_constructible_v<D>
        {
            any(std::forward<T>(value)).swap(*this);
            return *this;
        }

        // emplace
        // 1
        template<typename T, typename... Args, typename D = std::decay_t<T>>
        auto emplace(Args&&... args) -> D&
            requires std::is_constructible_v<D, Args...> and std::is_copy_constructible_v<D>
        {
            any(std::in_place_type<D>, std::forward<Args>(args)...).swap(*this);
            return *get_as<D>();
        }
        // 2
        template<typename T, typename U, typename... Args, typename D = std::decay_t<T>>
        auto emplace(std::initializer_list<U> il, Args&&... args) -> D&
            requires std::is_constructible_v<D, std::initializer_list<U>&, Args...>
                 and std::is_copy_constructible_v<D>
        {
            any(std::in_place_type<D>, il, std::forward<Args>(args)...).swap(*this);
            return *get_as<D>();
        }

        // reset
        void reset() noexcept
        {
            if (meta_) {
                meta_->destroy(storage_);
                meta_ = nullptr;
            }
        }

        // swap
        void swap(any& that) noexcept
        {
            switch (has_value() | that.has_value() << 1) {
            case 0:
            default:
                // Do nothing;
                break;
            case 1:
                new(&that) any(std::move(*this));
                break;
            case 2:
                new(this) any(std::move(that));
                break;
            case 3:
                if (type() == that.type()) {
                    meta_->swap(storage_, that.storage_);
                } else {
                    auto tmp = any(std::move(*this));
                    *this = std::move(that);
                    that = std::move(tmp);
                }
                break;
            }
        }

        // has_value
        bool has_value() const noexcept
        {
            return meta_;
        }

        // type
        auto type() const noexcept -> std::type_info const&
        {
            return meta_ ? meta_->type : typeid(void);
        }

    private:
        template<typename T>
        auto get_as() noexcept
        {
            if constexpr (impl::any_uses_sbo<T>)
                return impl::any_members<T>::get(storage_);
            else
                return *impl::any_members<T>::get(storage_);
        }
        template<typename T>
        auto get_as() const noexcept
        {
            if constexpr (impl::any_uses_sbo<T>)
                return impl::any_members<T>::get(storage_);
            else
                return *impl::any_members<T>::get(storage_);
        }

        template<typename T>
        static auto get_meta_for() noexcept
        {
            using members = impl::any_members<T>;
            static auto const meta = impl::any_meta{
                .type = typeid(T),
                .destroy = members::destroy,
                .copy_construct = members::copy_construct,
                .move_construct = members::move_construct,
                .copy_assign = members::copy_assign,
                .move_assign = members::move_assign,
                .swap = members::swap,
            };
            return &meta;
        }

        template<typename T>
        friend auto any_cast(any const&) -> T
            requires std::is_constructible_v<T, std::remove_cvref_t<T> const&>;
        template<typename T>
        friend auto any_cast(any&) -> T
            requires std::is_constructible_v<T, std::remove_cvref_t<T>&>;
        template<typename T>
        friend auto any_cast(any&&) -> T
            requires std::is_constructible_v<T, std::remove_cvref_t<T>>;
        template<typename T>
        friend auto any_cast(any const*) -> T const*;
        template<typename T>
        friend auto any_cast(any*) -> T*;
    };

    // swap
    inline void swap(any& a, any& b) noexcept
    {
        a.swap(b);
    }

    // any_cast
    // 1
    template<typename T>
    inline auto any_cast(any const& a) -> T
        requires std::is_constructible_v<T, std::remove_cvref_t<T> const&>
    {
        if (a.type() != typeid(T))
            throw bad_any_cast();

        return *a.get_as<T>();
    }
    // 2
    template<typename T>
    inline auto any_cast(any& a) -> T
        requires std::is_constructible_v<T, std::remove_cvref_t<T>&>
    {
        if (a.type() != typeid(T))
            throw bad_any_cast();

        return *a.get_as<T>();
    }
    // 3
    template<typename T>
    inline auto any_cast(any&& a) -> T
        requires std::is_constructible_v<T, std::remove_cvref_t<T>>
    {
        if (a.type() != typeid(T))
            throw bad_any_cast();

        return std::move(*a.get_as<T>());
    }
    // 4
    template<typename T>
    inline auto any_cast(any const* a) -> T const*
    {
        return (a and a->type() == typeid(T)) ? a->get_as<T>() : nullptr;
    }
    // 5
    template<typename T>
    inline auto any_cast(any* a) -> T*
    {
        return (a and a->type() == typeid(T)) ? a->get_as<T>() : nullptr;
    }

    // make_any
    // 1
    template<typename T, typename... Args >
    inline auto make_any(Args&&... args) -> any
    {
        return any(std::in_place_type<T>, std::forward<Args>(args)...);
    }
    // 2
    template<typename T, typename U, typename... Args >
    inline auto make_any(std::initializer_list<U> il, Args&&... args) -> any
    {
        return any(std::in_place_type<T>, il, std::forward<Args>(args)...);
    }
}
