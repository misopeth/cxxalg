#include <catch2/catch_test_macros.hpp>

#include <cxxalg/optional.hxx>

#include <concepts>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

using namespace cxxalg;

using namespace std::string_literals;
using namespace std::string_view_literals;

TEST_CASE("optional::optional()", "[optional]") {
    constexpr auto o = optional<int>();
    REQUIRE_FALSE(o);
}

TEST_CASE("optional::optional(nullopt)", "[optional]") {
    constexpr auto o = optional<int>(nullopt);
    REQUIRE_FALSE(o);
}

TEST_CASE("optional::optional(optional const&)", "[optional]") {
    constexpr auto o = optional<int>();
    constexpr auto p = o;
    REQUIRE(o == p);
    auto q = optional<int>(5);
    auto r = q;
    REQUIRE(q);
    REQUIRE(r);
    REQUIRE(*q == 5);
    REQUIRE(*r == 5);
    auto s = optional<std::string>("stringa che richiede allocazione");
    auto t = s;
    REQUIRE(s);
    REQUIRE(t);
    REQUIRE(*s == "stringa che richiede allocazione");
    REQUIRE(*t == *s);
}

TEST_CASE("optional::optional(optional&&)", "[optional]") {
    constexpr auto o = optional<int>();
    constexpr auto p = std::move(o);
    REQUIRE(o == p);
    auto q = optional<int>(5);
    auto r = std::move(q);
    REQUIRE(q);
    REQUIRE(r);
    REQUIRE(*q == 5);
    REQUIRE(*r == 5);
    auto s = optional<std::string>("stringa che richiede allocazione");
    auto t = std::move(s);
    REQUIRE(s);
    REQUIRE(t);
    REQUIRE(*s == "");
    REQUIRE(*t == "stringa che richiede allocazione");
}

TEST_CASE("optional::optional(optional<U> const&)", "[optional]") {
    constexpr auto o = optional<int>();
    constexpr auto p = optional<long>(o);
    REQUIRE(o == p);
    auto q = optional<int>(5);
    auto r = optional<long>(q);
    REQUIRE(q);
    REQUIRE(r);
    REQUIRE(*q == 5);
    REQUIRE(*r == 5);
    auto s = optional<std::string>("stringa che richiede allocazione");
    auto t = optional<std::string_view>(s);
    REQUIRE(s);
    REQUIRE(t);
    REQUIRE(*s == "stringa che richiede allocazione");
    REQUIRE(*t == *s);
}

TEST_CASE("optional::optional(optional<U>&&)", "[optional]") {
    auto o = optional<int>();
    auto p = optional<long>(std::move(o));
    REQUIRE_FALSE(o);
    REQUIRE_FALSE(p);
    auto q = optional<int>(5);
    auto r = optional<long>(std::move(q));
    REQUIRE(q);
    REQUIRE(r);
    REQUIRE(*q == 5);
    REQUIRE(*r == 5);
    auto s = optional<std::string>("stringa che richiede allocazione");
    auto t = optional<std::string>(std::move(s));
    REQUIRE(s);
    REQUIRE(t);
    REQUIRE(*s == "");
    REQUIRE(*t == "stringa che richiede allocazione");
}

TEST_CASE("optional::optional(std::in_place, Args&&)", "[optional]") {
    auto o = optional<int>(std::in_place);
    auto p = optional<int>(std::in_place, 0);
    REQUIRE(o == p);
    REQUIRE(*o == 0);
    auto q = optional<std::string>(std::in_place, "ciao");
    auto r = optional<std::string>(std::in_place, 2, '*');
    REQUIRE(*q == "ciao");
    REQUIRE(*r == "**");
}

TEST_CASE("optional::optional(std::in_place, std::initializer_list<U>, Args&&)", "[optional]") {
    auto o = optional<std::vector<int>>(std::in_place, 2, 5);
    auto p = optional<std::vector<int>>(std::in_place, {2, 5});
    REQUIRE(*o == (std::vector{5, 5}));
    REQUIRE(*p == (std::vector{2, 5}));
}

TEST_CASE("optional::operator=(nullopt)", "[optional]") {
    auto o = optional<int>(5);
    o = nullopt;
    REQUIRE_FALSE(o);
}

TEST_CASE("optional::operator=(optional const&)", "[optional]") {
    auto o = optional<int>();
    auto p = optional<int>();
    p = o;
    REQUIRE(o == p);
    auto q = optional<int>(5);
    auto r = optional<int>();
    r = q;
    REQUIRE(q);
    REQUIRE(r);
    REQUIRE(*q == 5);
    REQUIRE(*r == 5);
    auto s = optional<std::string>("stringa che richiede allocazione");
    auto t = optional<std::string>();
    t = s;
    REQUIRE(s);
    REQUIRE(t);
    REQUIRE(*s == "stringa che richiede allocazione");
    REQUIRE(*t == *s);
}

TEST_CASE("optional::operator=(optional&&)", "[optional]") {
    auto o = optional<int>();
    auto p = optional<int>();
    p = std::move(o);
    REQUIRE(o == p);
    auto q = optional<int>(5);
    auto r = optional<int>();
    r = std::move(q);
    REQUIRE(q);
    REQUIRE(r);
    REQUIRE(*q == 5);
    REQUIRE(*r == 5);
    auto s = optional<std::string>("stringa che richiede allocazione");
    auto t = optional<std::string>();
    t = std::move(s);
    REQUIRE(s);
    REQUIRE(t);
    REQUIRE(*s == "");
    REQUIRE(*t == "stringa che richiede allocazione");
}

TEST_CASE("optional::operator=(U&&)", "[optional]") {
    auto o = optional<int>();
    o = 5;
    o = 7L;
    REQUIRE(*o == 7);
    auto s = optional<std::string>();
    s = "stringa che richiede allocazione";
    REQUIRE(*s == "stringa che richiede allocazione");
}

TEST_CASE("optional::operator=(optional<U> const&)", "[optional]") {
    auto o = optional<int>(5);
    auto p = optional<long>();
    p = o;
    REQUIRE(*p == 5);
    auto s = optional<std::string>("stringa che richiede allocazione");
    auto t = optional<std::string_view>();
    t = s;
    REQUIRE(s);
    REQUIRE(t);
    REQUIRE(*s == "stringa che richiede allocazione");
    REQUIRE(*t == *s);
}

TEST_CASE("optional::operator=(optional<U>&&)", "[optional]") {
    auto o = optional<int>(5);
    auto p = optional<long>();
    p = std::move(o);
    REQUIRE(*o == 5);
    REQUIRE(*p == 5);
    auto s = optional<std::string_view>("stringa che richiede allocazione");
    auto t = optional<std::string>();
    auto u = optional<std::string>();
    t = std::move(s);
    u = std::move(t);
    REQUIRE(s);
    REQUIRE(t);
    REQUIRE(u);
    REQUIRE(*s == *u);
    REQUIRE(*t == "");
    REQUIRE(*u == "stringa che richiede allocazione");
}

TEST_CASE("optional::operator->, optional::operator*", "[optional]") {
    auto o = optional<int>(5);
    REQUIRE(std::is_same_v<int&, decltype((*o))>);
    REQUIRE(std::is_same_v<int&&, decltype((*std::move(o)))>);
    REQUIRE(*o == 5);
    auto p = optional<std::string>("ciao");
    REQUIRE(p->size() == 4);
    p = nullopt;
    REQUIRE_THROWS_AS(p.value().size(), bad_optional_access);
}

TEST_CASE("std::optional::operator bool, std::optional::has_value", "[optional]") {
    // Done
}

TEST_CASE("optional::value", "[optional]") {
    auto o = optional<int>(5);
    REQUIRE(std::is_same_v<int&, decltype((o.value()))>);
    REQUIRE(std::is_same_v<int&&, decltype((std::move(o).value()))>);
    REQUIRE(o.value() == 5);
    auto p = optional<std::string>("ciao");
    REQUIRE(p.value().size() == 4);
    p = nullopt;
    REQUIRE_THROWS_AS(p.value(), bad_optional_access);
}

TEST_CASE("optional::value_or", "[optional]") {
    auto o = optional<int>();
    REQUIRE(o.value_or(5) == 5);
    o = 4;
    REQUIRE(o.value_or([] { return 5; }()) == 4);
}

TEST_CASE("optional::and_then", "[optional]") {
    auto o = optional<int>(5);
    auto p = o.and_then([](int x) { return optional<long>(x + 5); });
    REQUIRE(*p == 10);
    o = nullopt;
    p = o.and_then([](int x) { return optional<long>(x + 5); });
    REQUIRE_FALSE(p);
}

TEST_CASE("optional::transform", "[optional]") {
    auto o = optional<int>();
    auto p = o.transform([](int x) { return x + 5; });
    REQUIRE_FALSE(p);
    o = 5;
    p = o.transform([](int x) { return x + 5; });
    REQUIRE(*p == 10);
}

TEST_CASE("optional::or_else", "[optional]") {
    auto o = optional<int>();
    auto p = o.or_else([] { return 5; });
    REQUIRE(*p == 5);
    o = 4;
    p = o.or_else([] { return 5; });
    REQUIRE(*p == 4);
}

TEST_CASE("optional::swap", "[optional]") {
    auto o = optional<int>(5);
    auto p = optional<int>();
    o.swap(p);
    REQUIRE_FALSE(o);
    REQUIRE(*p == 5);
    auto q = optional<std::string>("ciao");
    auto r = optional<std::string>("cacao");
    r.swap(q);
    REQUIRE(*q == "cacao");
    REQUIRE(*r == "ciao");
}

TEST_CASE("optional::reset", "[optional]") {
    auto o = optional<int>(5);
    o.reset();
    REQUIRE_FALSE(o);
    auto p = optional<std::string>("ciao");
    p.reset();
    REQUIRE_FALSE(p);
}

TEST_CASE("optional::emplace", "[optional]") {
    auto o = optional<int>();
    o.emplace(5);
    REQUIRE(*o == 5);
    auto p = optional<std::string>();
    p.emplace("ciao");
    REQUIRE(*p == "ciao");
    p.emplace(2, '*');
    REQUIRE(*p == "**");
    auto r = optional<std::vector<int>>();
    r.emplace({1, 2}).push_back(3);
    REQUIRE(*r == (std::vector<int>{1, 2, 3}));
}

// TODO

TEST_CASE("small optional", "[optional][nonstd]") {
    REQUIRE(sizeof(optional<bool>) == sizeof(bool));
    REQUIRE(sizeof(optional<optional<bool>>) == sizeof(bool));
    REQUIRE(sizeof(optional<optional<std::string>>) == sizeof(optional<std::string>));
}

template<typename T, std::equality_comparable_with<T> auto... Spares>
struct with_spares {
    static constexpr auto spare_representations = sizeof...(Spares);
    static constexpr auto index(T const* p) -> std::size_t
    {
        auto ret = -1, i = 0;
        (((*p == Spares) ? (ret = i, true) : (++i, false)) or ...);
        return ret;
    }
    static constexpr void set_spare_representation(T* p, std::size_t index)
        requires (sizeof...(Spares) != 0)
    {
        auto i = 0;
        (((index == i) ? (std::construct_at(p, Spares), true) : (++i, false)) or ...);
    }
};

TEST_CASE("optional custom traits", "[optional][nonstd]") {
    REQUIRE(sizeof(optional<int, with_spares<int, 0>>) == sizeof(int));
    REQUIRE(sizeof(optional<int, with_spares<int, 0, 1>>) == sizeof(int));
    REQUIRE(sizeof(optional<std::size_t, with_spares<std::size_t, 0>>) == sizeof(std::size_t));
    REQUIRE(sizeof(optional<void*, with_spares<void*, nullptr>>) == sizeof(void*));

    auto o = optional<int, with_spares<int, 0>>();
    REQUIRE_FALSE(o);
    o = 5;
    REQUIRE(o);
    REQUIRE(*o == 5);
    o = 0;
    REQUIRE_FALSE(o);
}

TEST_CASE("optional of optional", "[optional][nonstd]") {
    REQUIRE(sizeof(optional<optional<int>>) == 2 * sizeof(int));
    REQUIRE(sizeof(optional<optional<int, with_spares<int>>>) == 2 * sizeof(int));
    REQUIRE(sizeof(optional<optional<int, with_spares<int, 0>>>) == 2 * sizeof(int));
    REQUIRE(sizeof(optional<optional<int, with_spares<int, 0, 1>>>) == sizeof(int));
    REQUIRE(sizeof(optional<optional<int, with_spares<int, 0, 1, 2>>>) == sizeof(int));

    auto o = optional<optional<int, with_spares<int, 0, 1>>>();
    REQUIRE_FALSE(o);
    o = optional<int, with_spares<int, 0, 1>>();
    REQUIRE(o);
    REQUIRE_FALSE(*o);
    o = 7;
    REQUIRE(o);
    REQUIRE(*o);
    REQUIRE(**o == 7);
    o = 1;
    REQUIRE_FALSE(o);
}
