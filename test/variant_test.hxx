#pragma once

#include <cxxalg/variant.hxx>

#include <string>
#include <string_view>

#include <cassert>

using namespace std::string_literals;
using namespace std::string_view_literals;

inline void variant_test()
{
    using namespace cxxalg;

    /* variant::variant() */ {
        auto v = variant<int, char>();
        assert(v.index() == 0 and get<0>(v) == 0);
    }
    /* variant::variant(variant const&) */ {
        auto v = variant<int, char const*>("ciao");
        auto w = v;
        assert(w.index() == 1 and get<1>(w) == "ciao"sv);
        auto x = variant<int, std::string>("ciao");
        auto y = x;
        assert(y.index() == 1 and get<1>(y) == "ciao");
    }
    /* variant::variant(variant&&) */ {
        auto v = variant<int, char>('c');
        auto w = std::move(v);
        assert(v.index() == 1 and get<1>(v) == 'c' and w.index() == 1 and get<1>(w) == 'c');
        auto x = variant<int, std::string>("ciao");
        auto y = std::move(x);
        assert(x.index() == 1 and get<1>(x) == "" and y.index() == 1 and get<1>(y) == "ciao");
        auto z = variant<std::unique_ptr<int>>();
        auto a = std::move(z);
    }
    /* variant::variant(T&&) */ {
        auto v = variant<int, long>(5);
        auto w = variant<char const*, std::string>("ciao");
        auto x = variant<char const*, std::string>("ciao"s);
        assert(v.index() == 0 and w.index() == 0 and x.index() == 1);
    }
    /* variant::variant(std::in_place_type<T>, Args&&...) */ {
        auto v = variant<int, std::string>(std::in_place_type<std::string>, '-', 20);
        assert(v.index() == 1 and get<1>(v) == std::string('-', 20));
    }
    /* variant::variant(std::in_place_type<T>, std::initializer_list<U>, Args&&...) */ {
        auto v = variant<int, std::string>(std::in_place_type<std::string>, {'c', 'i', 'a', 'o'});
        assert(v.index() == 1 and get<1>(v) == "ciao");
    }
    /* variant::variant(std::in_place_index<I>, Args&&...) */ {
        auto v = variant<int, std::string>(std::in_place_index<1>, '-', 20);
        assert(v.index() == 1 and get<1>(v) == std::string('-', 20));
    }
    /* variant::variant(std::in_place_index<I>, std::initializer_list<U>, Args&&...) */ {
        auto v = variant<int, std::string>(std::in_place_index<1>, {'c', 'i', 'a', 'o'});
        assert(v.index() == 1 and get<1>(v) == "ciao");
    }
    /* variant::operator=(variant const&) */ {
        auto v = variant<int, std::string>("ciao");
        auto w = decltype(v)();
        w = v;
        assert(v.index() == 1 and get<1>(v) == "ciao" and w.index() == 1 and get<1>(w) == "ciao");
    }
    /* variant::operator=(variant&&) */ {
        auto v = variant<int, std::string>("ciao");
        auto w = decltype(v)();
        w = std::move(v);
        assert(v.index() == 1 and get<1>(v) == "" and w.index() == 1 and get<1>(w) == "ciao");
    }
    /* variant::operator=(T&&) */ {
        auto v = variant<int, std::string>("ciao");
        v = 1;
        assert(v.index() == 0 and get<0>(v) == 1);
        v = std::string("ciao");
        assert(v.index() == 1 and get<1>(v) == "ciao");
    }
    /* variant::index() */ {
        // Done
    }
    /* variant::valueless_by_exception() */ {
        struct throw_when_copying {
            throw_when_copying() = default;
            throw_when_copying(throw_when_copying const&) { throw 5; }
            auto operator=(throw_when_copying const&) -> throw_when_copying& = default;
        };
        auto v = variant<throw_when_copying, std::string>("ciao");
        auto t = throw_when_copying();
        try {
            v = t;
            assert(false);
        } catch (int) {
            assert(v.valueless_by_exception());
        }
    }
    /* variant::emplace<T>(Args&&...) */ {
        auto v = variant<int, std::string>();
        v.emplace<std::string>('-', 20);
        assert(v.index() == 1 and get<1>(v) == std::string('-', 20));
    }
    /* variant::emplace<T>(std::initializer_list<U>, Args&&...) */ {
        auto v = variant<int, std::string>();
        v.emplace<std::string>({'c', 'i', 'a', 'o'});
        assert(v.index() == 1 and get<1>(v) == "ciao");
    }
    /* variant::emplace<I>(Args&&...) */ {
        auto v = variant<int, std::string>();
        v.emplace<1>('-', 20);
        assert(v.index() == 1 and get<1>(v) == std::string('-', 20));
    }
    /* variant::emplace<I>(std::initializer_list<U>, Args&&...) */ {
        auto v = variant<int, std::string>();
        v.emplace<1>({'c', 'i', 'a', 'o'});
        assert(v.index() == 1 and get<1>(v) == "ciao");
    }
    /* variant::swap(variant&) */ {
        auto v = variant<std::unique_ptr<int>, std::string>("ciao");
        auto w = decltype(v)(std::make_unique<int>(5));
        w.swap(v);
        assert(v.index() == 0 and *get<0>(v) == 5 and w.index() == 1 and get<1>(w) == "ciao");
    }
    /* visit(Visitor&&, Variants&&...) */ {
        auto v = variant<std::unique_ptr<int>, std::string>("ciao");
        auto w = variant<int, double, std::vector<char>>(2);
        struct {
            int operator()(std::unique_ptr<int> const& a, int b) const { return (a ? *a : 0) * b; }
            int operator()(std::unique_ptr<int> const& a, double b) const { return (a ? *a : 0) * b; }
            int operator()(std::unique_ptr<int> const& a, std::vector<char> const& b) const { return (a ? *a : 0) * b.size(); }
            int operator()(std::string const& a, int b) { return a.size() * b; }
            int operator()(std::string const& a, double b) { return a.size() * b; }
            int operator()(std::string const& a, std::vector<char> const& b) { return a.size() * b.size(); }
        } visitor;
        assert(visit(visitor, v, w) == 8);
    }

    // TODO
}
