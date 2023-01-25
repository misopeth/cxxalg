#include <catch2/catch_test_macros.hpp>

#include <cxxalg/variant.hxx>

#include <memory>
#include <string>
#include <string_view>
#include <vector>

using namespace cxxalg;

using namespace std::string_literals;
using namespace std::string_view_literals;

TEST_CASE("variant::variant()", "[variant]") {
    auto v = variant<int, char>();
    REQUIRE(v.index() == 0);
    REQUIRE(get<0>(v) == 0);
}

// TODO
TEST_CASE("variant::variant(variant const&)", "[variant]") {
    auto v = variant<int, char const*>("ciao");
    auto w = v;
    REQUIRE(w.index() == 1);
    REQUIRE(get<1>(w) == "ciao"sv);
    auto x = variant<int, std::string>("ciao");
    auto y = x;
    REQUIRE(y.index() == 1);
    REQUIRE(get<1>(y) == "ciao");
}

TEST_CASE("variant::variant(variant&&)", "[variant]") {
    auto v = variant<int, char>('c');
    auto w = std::move(v);
    REQUIRE(v.index() == 1);
    REQUIRE(w.index() == 1);
    REQUIRE(get<1>(v) == 'c');
    REQUIRE(get<1>(w) == 'c');
    auto x = variant<int, std::string>("ciao");
    auto y = std::move(x);
    REQUIRE(x.index() == 1);
    REQUIRE(y.index() == 1);
    REQUIRE(get<1>(x) == "");
    REQUIRE(get<1>(y) == "ciao");
    auto z = variant<std::unique_ptr<int>>();
    auto a = std::move(z);
}

TEST_CASE("variant::variant(T&&)", "[variant]") {
    auto v = variant<int, long>(5);
    auto w = variant<char const*, std::string>("ciao");
    auto x = variant<char const*, std::string>("ciao"s);
    REQUIRE(v.index() == 0);
    REQUIRE(w.index() == 0);
    REQUIRE(x.index() == 1);
}

TEST_CASE("variant::variant(std::in_place_type<T>, Args&&...)", "[variant]") {
    auto v = variant<int, std::string>(std::in_place_type<std::string>, '-', 20);
    REQUIRE(v.index() == 1);
    REQUIRE(get<1>(v) == std::string('-', 20));
}

TEST_CASE("variant::variant(std::in_place_type<T>, std::initializer_list<U>, Args&&...)", "[variant]") {
    auto v = variant<int, std::string>(std::in_place_type<std::string>, {'c', 'i', 'a', 'o'});
    REQUIRE(v.index() == 1);
    REQUIRE(get<1>(v) == "ciao");
}

TEST_CASE("variant::variant(std::in_place_index<I>, Args&&...)", "[variant]") {
    auto v = variant<int, std::string>(std::in_place_index<1>, '-', 20);
    REQUIRE(v.index() == 1);
    REQUIRE(get<1>(v) == std::string('-', 20));
}

TEST_CASE("variant::variant(std::in_place_index<I>, std::initializer_list<U>, Args&&...)", "[variant]") {
    auto v = variant<int, std::string>(std::in_place_index<1>, {'c', 'i', 'a', 'o'});
    REQUIRE(v.index() == 1);
    REQUIRE(get<1>(v) == "ciao");
}

TEST_CASE("variant::operator=(variant const&)", "[variant]") {
    auto v = variant<int, std::string>("ciao");
    auto w = decltype(v)();
    w = v;
    REQUIRE(v.index() == 1);
    REQUIRE(w.index() == 1);
    REQUIRE(get<1>(v) == "ciao");
    REQUIRE(get<1>(w) == "ciao");
}

TEST_CASE("variant::operator=(variant&&)", "[variant]") {
    auto v = variant<int, std::string>("ciao");
    auto w = decltype(v)();
    w = std::move(v);
    REQUIRE(v.index() == 1);
    REQUIRE(w.index() == 1);
    REQUIRE(get<1>(v) == "");
    REQUIRE(get<1>(w) == "ciao");
}

TEST_CASE("variant::operator=(T&&)", "[variant]") {
    auto v = variant<int, std::string>("ciao");
    v = 1;
    REQUIRE(v.index() == 0);
    REQUIRE(get<0>(v) == 1);
    v = std::string("ciao");
    REQUIRE(v.index() == 1);
    REQUIRE(get<1>(v) == "ciao");
}

TEST_CASE("variant::index()", "[variant]") {
    // Done
}

TEST_CASE("variant::valueless_by_exception()", "[variant]") {
    struct throw_when_copying {
        throw_when_copying() = default;
        throw_when_copying(throw_when_copying const&) { throw 5; }
        auto operator=(throw_when_copying const&) -> throw_when_copying& = default;
    };
    auto v = variant<throw_when_copying, std::string>("ciao");
    auto t = throw_when_copying();
    REQUIRE_THROWS(v = t);
    REQUIRE(v.valueless_by_exception());
}

TEST_CASE("variant::emplace<T>(Args&&...)", "[variant]") {
    auto v = variant<int, std::string>();
    v.emplace<std::string>('-', 20);
    REQUIRE(v.index() == 1);
    REQUIRE(get<1>(v) == std::string('-', 20));
}

TEST_CASE("variant::emplace<T>(std::initializer_list<U>, Args&&...)", "[variant]") {
    auto v = variant<int, std::string>();
    v.emplace<std::string>({'c', 'i', 'a', 'o'});
    REQUIRE(v.index() == 1);
    REQUIRE(get<1>(v) == "ciao");
}

TEST_CASE("variant::emplace<I>(Args&&...)", "[variant]") {
    auto v = variant<int, std::string>();
    v.emplace<1>('-', 20);
    REQUIRE(v.index() == 1);
    REQUIRE(get<1>(v) == std::string('-', 20));
}

TEST_CASE("variant::emplace<I>(std::initializer_list<U>, Args&&...)", "[variant]") {
    auto v = variant<int, std::string>();
    v.emplace<1>({'c', 'i', 'a', 'o'});
    REQUIRE(v.index() == 1);
    REQUIRE(get<1>(v) == "ciao");
}

TEST_CASE("variant::swap(variant&)", "[variant]") {
    auto v = variant<std::unique_ptr<int>, std::string>("ciao");
    auto w = decltype(v)(std::make_unique<int>(5));
    w.swap(v);
    REQUIRE(v.index() == 0);
    REQUIRE(w.index() == 1);
    REQUIRE(*get<0>(v) == 5);
    REQUIRE(get<1>(w) == "ciao");
}

TEST_CASE("visit(Visitor&&, Variants&&...)", "[variant]") {
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
    REQUIRE(visit(visitor, v, w) == 8);
}

TEST_CASE("visit(Visitor&&, Variants&&...) 2", "[variant]") {
    auto v = variant<int, char>(5);
    struct {
        int operator()(int&      ) const { return 0; }
        int operator()(int const&) const { return 1; }
        int operator()(int&&     ) const { return 2; }
        int operator()(char      ) const { return 3; }
    } visitor;
    REQUIRE(visit(visitor, v) == 0);
    REQUIRE(visit(visitor, std::as_const(v)) == 1);
    REQUIRE(visit(visitor, std::move(v)) == 2);
}

// TODO
