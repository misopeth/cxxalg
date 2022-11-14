#pragma once

#include <cxxalg/any.hxx>

#include <string>
#include <vector>
#include <utility>

#include <cassert>

inline void any_test()
{
    using namespace cxxalg;

    /* any::any() */ {
        constexpr auto a = any();
        assert(not a.has_value());
        assert(a.type() == typeid(void));
    }
    /* any::any(T&&) */ {
        auto a = any(5);
        assert(a.has_value());
        assert(a.type() == typeid(int));
        auto b = any(std::string("string content that requires allocation"));
        assert(b.has_value());
        assert(b.type() == typeid(std::string));
    }
    /* any::any(any const&) */ {
        auto a = any();
        auto b = any(a);
        assert(not b.has_value());
        assert(b.type() == typeid(void));
        a = 5;
        auto c = any(a);
        assert(a.has_value() and c.has_value());
        assert(a.type() == typeid(int) and c.type() == typeid(int));
        a = std::string("string content that requires allocation");
        auto d = any(a);
        assert(a.has_value() and d.has_value());
        assert(a.type() == typeid(std::string) and d.type() == typeid(std::string));
        assert(any_cast<std::string>(d) == std::string("string content that requires allocation"));
    }
    /* any::any(any&&) */ {
        auto a = any();
        auto b = any(std::move(a));
        assert(not a.has_value() and not b.has_value());
        a = 5;
        auto c = any(std::move(a));
        assert(not a.has_value() and c.has_value());
        a = std::string("string content that requires allocation");
        auto d = any(std::move(a));
        assert(not a.has_value() and d.has_value());
        assert(any_cast<std::string>(d) == std::string("string content that requires allocation"));
    }
    /* any::any(std::in_place_type_t<T>, Args&&...) */ {
        auto a = any(std::in_place_type<int>);
        auto b = any(std::in_place_type<std::string>);
        auto c = any(std::in_place_type<int>, 0);
        auto d = any(std::in_place_type<std::string>, 40, '*');
        assert(any_cast<std::string>(d) == std::string(40, '*'));
    }
    /* any::any(std::in_place_type_t<T>, std::initializer_list<U>, Args&&...) */ {
        auto a = any(std::in_place_type<std::vector<int>>, {0, 1, 2, 3});
        assert((any_cast<std::vector<int>>(a) == std::vector{0, 1, 2, 3}));
    }

    /* any::operator=(any const&) */ {
        auto a = any();
        auto b = a;
        assert(not b.has_value());
        a = 5;
        b = a;
        assert(any_cast<int>(a) == 5 and any_cast<int>(b) == 5);
    }
    /* any::operator=(any&&) */ {
        auto a = any();
        auto b = std::move(a);
        assert(not b.has_value());
        a = 5;
        b = std::move(a);
        assert(not a.has_value() and any_cast<int>(b) == 5);
    }
    /* any::operator=(T&&) */ {
        auto a = any();
        a = 5;
        assert(any_cast<int>(a) == 5);
        a = "ciao";
        assert(any_cast<char const*>(a) == std::string("ciao"));
    }

    /* any::emplace<T>(Args&&...) */ {
        auto a = any();
        ++a.emplace<int>(5);
        assert(any_cast<int>(a) == 6);
        a.emplace<std::string>("ciao") += " mare";
        assert(any_cast<std::string>(a) == "ciao mare");
    }
    /* any::emplace<T>(std::initializer_list<U>, Args&&...) */ {
        auto a = any();
        a.emplace<std::vector<int>>({0, 1, 2}).push_back(3);
        assert((any_cast<std::vector<int>>(a) == std::vector{0, 1, 2, 3}));
    }

    /* any::reset() */ {
        auto a = any();
        a.reset();
        a = 5;
        a.reset();
        assert(not a.has_value());
    }

    /* any::swap(any&) */ {
        any a(5), b(std::string("ciao"));
        a.swap(b);
        assert(any_cast<std::string>(a) == "ciao" and any_cast<int>(b) == 5);
        a.reset();
        b.swap(a);
        assert(any_cast<int>(a) == 5 and not b.has_value());
    }

    /* any::has_value() */ {
        auto const a = any();
        assert(not a.has_value());
        auto const b = any(5);
        assert(b.has_value());
    }

    /* any::type() */ {
        auto const a = any(5);
        assert(a.type() == typeid(int));
    }

    /* swap(any&, any&) */ {
        any a(5), b(std::string("ciao"));
        swap(a, b);
        assert(any_cast<std::string>(a) == "ciao" and any_cast<int>(b) == 5);
        a.reset();
        swap(b, a);
        assert(any_cast<int>(a) == 5 and not b.has_value());
    }

    /* any_cast<T>(any const&) */ {
        auto const a = any(5);
        assert(any_cast<int>(a) == 5);
        try {
            any_cast<short>(a);
            assert(false);
        } catch (bad_any_cast const&) {
            assert(true);
        }
    }
    /* any_cast<T>(any&) */ {
        auto a = any(5);
        assert(any_cast<int>(a) == 5);
    }
    /* any_cast<T>(any&&) */ {
        auto a = any(5);
        assert(any_cast<int>(std::move(a)) == 5);
        assert(a.has_value());
        a = std::string("ciao");
        assert(any_cast<std::string>(std::move(a)) == "ciao");
        assert(a.has_value() and any_cast<std::string>(a).empty());
    }
    /* any_cast<T>(any const*) */ {
        auto const a = any(5);
        assert(*any_cast<int>(&a) == 5);
        assert(any_cast<short>(&a) == nullptr);
    }
    /* any_cast<T>(any*) */ {
        auto a = any(5);
        *any_cast<int>(&a) += 3;
        assert(*any_cast<int>(&a) == 8);
    }

    /* make_any<T>(Args&&...) */ {
        auto a = make_any<int>();
        assert(any_cast<int>(a) == 0);
        a = make_any<std::vector<int>>(2, 10);
        assert((any_cast<std::vector<int>>(a) == std::vector{10, 10}));
    }
    /* make_any<T>(std::initializer_list<U>, Args&&...) */ {
        auto a = make_any<std::vector<int>>({2, 10});
        assert((any_cast<std::vector<int>>(a) == std::vector{2, 10}));
    }
}
