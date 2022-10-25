#pragma once

#include <cxxalg/optional.hxx>

#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include <cassert>

inline void optional_test()
{
    using namespace cxxalg;

    /* optional::optional() */ {
        constexpr auto o = optional<int>();
        static_assert(not o);
    }
    /* optional::optional(nullopt) */ {
        constexpr auto o = optional<int>(nullopt);
        static_assert(not o);
    }
    /* optional::optional(optional const&) */ {
        constexpr auto o = optional<int>();
        constexpr auto p = o;
        static_assert(o == p);
        auto q = optional<int>(5);
        auto r = q;
        assert(q and r and (*q == 5) and (*r == 5));
        auto s = optional<std::string>("stringa che richiede allocazione");
        auto t = s;
        assert(s and t and (*s == "stringa che richiede allocazione") and (*t == *s));
    }
    /* optional::optional(optional&&) */ {
        constexpr auto o = optional<int>();
        constexpr auto p = std::move(o);
        static_assert(o == p);
        auto q = optional<int>(5);
        auto r = std::move(q);
        assert(q and r and (*q == 5) and (*r == 5));
        auto s = optional<std::string>("stringa che richiede allocazione");
        auto t = std::move(s);
        assert(s and t and (*s == "") and (*t == "stringa che richiede allocazione"));
    }
    /* optional::optional(optional<U> const&) */ {
        constexpr auto o = optional<int>();
        constexpr auto p = optional<long>(o);
        static_assert(o == p);
        auto q = optional<int>(5);
        auto r = optional<long>(q);
        assert(q and r and (*q == 5) and (*r == 5));
        auto s = optional<std::string>("stringa che richiede allocazione");
        auto t = optional<std::string_view>(s);
        assert(s and t and (*s == "stringa che richiede allocazione") and (*t == *s));
    }
    /* optional::optional(optional<U>&&) */ {
        auto o = optional<int>();
        auto p = optional<long>(std::move(o));
        assert(not o and not p);
        auto q = optional<int>(5);
        auto r = optional<long>(std::move(q));
        assert(q and r and (*q == 5) and (*r == 5));
        auto s = optional<std::string>("stringa che richiede allocazione");
        auto t = optional<std::string>(std::move(s));
        assert(s and t and (*s == "") and (*t == std::string("stringa che richiede allocazione")));
    }
    /* optional::optional(std::in_place, Args&&) */ {
        auto o = optional<int>(std::in_place);
        auto p = optional<int>(std::in_place, 0);
        assert(o == p and *o == 0);
        auto q = optional<std::string>(std::in_place, "ciao");
        auto r = optional<std::string>(std::in_place, 2, '*');
        assert(*q == "ciao" and *r == "**");
    }
    /* optional::optional(std::in_place, std::initializer_list<U>, Args&&) */ {
        auto o = optional<std::vector<int>>(std::in_place, 2, 5);
        auto p = optional<std::vector<int>>(std::in_place, {2, 5});
        assert((*o == std::vector{5, 5} and *p == std::vector{2, 5}));
    }

    /* optional::operator=(nullopt) */ {
        auto o = optional<int>(5);
        o = nullopt;
        assert(not o);
    }
    /* optional::operator=(optional const&) */ {
        auto o = optional<int>();
        auto p = optional<int>();
        p = o;
        assert(o == p);
        auto q = optional<int>(5);
        auto r = optional<int>();
        r = q;
        assert(q and r and (*q == 5) and (*r == 5));
        auto s = optional<std::string>("stringa che richiede allocazione");
        auto t = optional<std::string>();
        t = s;
        assert(s and t and (*s == "stringa che richiede allocazione") and (*t == *s));
    }
    /* optional::operator=(optional&&) */ {
        auto o = optional<int>();
        auto p = optional<int>();
        p = std::move(o);
        assert(o == p);
        auto q = optional<int>(5);
        auto r = optional<int>();
        r = std::move(q);
        assert(q and r and (*q == 5) and (*r == 5));
        auto s = optional<std::string>("stringa che richiede allocazione");
        auto t = optional<std::string>();
        t = std::move(s);
        assert(s and t and (*s == "") and (*t == "stringa che richiede allocazione"));
    }
    /* optional::operator=(U&&) */ {
        auto o = optional<int>();
        o = 5;
        o = 7L;
        assert(*o == 7);
        auto s = optional<std::string>();
        s = "stringa che richiede allocazione";
        assert(*s == "stringa che richiede allocazione");
    }
    /* optional::operator=(optional<U> const&) */ {
        auto o = optional<int>(5);
        auto p = optional<long>();
        p = o;
        assert(*p == 5);
        auto s = optional<std::string>("stringa che richiede allocazione");
        auto t = optional<std::string_view>();
        t = s;
        assert(s and t and (*s == "stringa che richiede allocazione") and (*t == *s));
    }
    /* optional::operator=(optional<U>&&) */ {
        auto o = optional<int>(5);
        auto p = optional<long>();
        p = std::move(o);
        assert(*o == 5 and *p == 5);
        auto s = optional<std::string_view>("stringa che richiede allocazione");
        auto t = optional<std::string>();
        auto u = optional<std::string>();
        t = std::move(s);
        u = std::move(t);
        assert(s and t and u and *s == *u and *t == "" and *u == "stringa che richiede allocazione");
    }

    /* optional::operator->, optional::operator* */ {
        auto o = optional<int>(5);
        static_assert(std::is_same_v<int&, decltype((*o))>);
        static_assert(std::is_same_v<int&&, decltype((*std::move(o)))>);
        assert(*o == 5);
        auto p = optional<std::string>("ciao");
        assert(p->size() == 4);
        p = nullopt;
        try {
            auto&& _ [[maybe_unused]] = p.value().size();
            assert(false);
        } catch (bad_optional_access const&) {
            assert(true);
        }
    }

    /* std::optional::operator bool, std::optional::has_value */ {
        // Done
    }

    /* optional::value */ {
        auto o = optional<int>(5);
        static_assert(std::is_same_v<int&, decltype((o.value()))>);
        static_assert(std::is_same_v<int&&, decltype((std::move(o).value()))>);
        assert(o.value() == 5);
        auto p = optional<std::string>("ciao");
        assert(p.value().size() == 4);
        p = nullopt;
        try {
            p.value();
            assert(false);
        } catch (bad_optional_access const&) {
            assert(true);
        }
    }

    /* optional::value_or */ {
        auto o = optional<int>();
        assert(o.value_or(5) == 5);
        o = 4;
        assert(o.value_or([] { return 5; }()) == 4);
    }

    /* optional::and_then */ {
        auto o = optional<int>(5);
        auto p = o.and_then([](int x) { return optional<long>(x + 5); });
        assert(*p == 10);
        o = nullopt;
        p = o.and_then([](int x) { return optional<long>(x + 5); });
        assert(not p);
    }

    /* optional::transform */ {
        auto o = optional<int>();
        auto p = o.transform([](int x) { return x + 5; });
        assert(not p);
        o = 5;
        p = o.transform([](int x) { return x + 5; });
        assert(*p == 10);
    }

    /* optional::or_else */ {
        auto o = optional<int>();
        auto p = o.or_else([] { return 5; });
        assert(*p == 5);
        o = 4;
        p = o.or_else([] { return 5; });
        assert(*p == 4);
    }

    /* optional::swap */ {
        auto o = optional<int>(5);
        auto p = optional<int>();
        o.swap(p);
        assert(not o and *p == 5);
        auto q = optional<std::string>("ciao");
        auto r = optional<std::string>("cacao");
        r.swap(q);
        assert(*q == "cacao" and *r == "ciao");
    }

    /* optional::reset */ {
        auto o = optional<int>(5);
        o.reset();
        assert(not o);
        auto p = optional<std::string>("ciao");
        p.reset();
        assert(not p);
    }

    /* optional::emplace */ {
        auto o = optional<int>();
        o.emplace(5);
        assert(*o == 5);
        auto p = optional<std::string>();
        p.emplace("ciao");
        assert(*p == "ciao");
        p.emplace(2, '*');
        assert(*p == "**");
        auto r = optional<std::vector<int>>();
        r.emplace({1, 2}).push_back(3);
        assert((*r == std::vector<int>{1, 2, 3}));
    }

    // TODO
}
