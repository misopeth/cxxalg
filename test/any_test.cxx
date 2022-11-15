#include <catch2/catch_test_macros.hpp>

#include <cxxalg/any.hxx>

#include <string>
#include <string_view>
#include <vector>
#include <utility>

using namespace cxxalg;
using namespace std::string_literals;
using namespace std::string_view_literals;

TEST_CASE("any::any()", "[any]") {
    constexpr auto a = any();
    REQUIRE_FALSE(a.has_value());
    REQUIRE(a.type() == typeid(void));
}

TEST_CASE("any::any(T&&)", "[any]") {
    auto a = any(5);
    REQUIRE(a.has_value());
    REQUIRE(a.type() == typeid(int));
    auto b = any("string content that requires allocation"s);
    REQUIRE(b.has_value());
    REQUIRE(b.type() == typeid(std::string));
}

TEST_CASE("any::any(any const&)", "[any]") {
    auto a = any();
    auto b = any(a);
    REQUIRE_FALSE(b.has_value());
    REQUIRE(b.type() == typeid(void));
    a = 5;
    auto c = any(a);
    REQUIRE(a.has_value());
    REQUIRE(c.has_value());
    REQUIRE(a.type() == typeid(int));
    REQUIRE(c.type() == typeid(int));
    a = "string content that requires allocation"s;
    auto d = any(a);
    REQUIRE(a.has_value());
    REQUIRE(d.has_value());
    REQUIRE(a.type() == typeid(std::string));
    REQUIRE(d.type() == typeid(std::string));
    REQUIRE(any_cast<std::string>(d) == "string content that requires allocation"sv);
}

TEST_CASE("any::any(any&&)", "[any]") {
    auto a = any();
    auto b = any(std::move(a));
    REQUIRE_FALSE(a.has_value());
    REQUIRE_FALSE(b.has_value());
    a = 5;
    auto c = any(std::move(a));
    REQUIRE_FALSE(a.has_value());
    REQUIRE(c.has_value());
    a = "string content that requires allocation"sv;
    auto d = any(std::move(a));
    REQUIRE_FALSE(a.has_value());
    REQUIRE(d.has_value());
    REQUIRE(any_cast<std::string_view>(d) == "string content that requires allocation"sv);
}

TEST_CASE("any::any(std::in_place_type_t<T>, Args&&...)", "[any]") {
    auto a = any(std::in_place_type<int>);
    auto b = any(std::in_place_type<std::string>);
    auto c = any(std::in_place_type<int>, 5);
    REQUIRE(any_cast<int>(c) == 5);
    auto d = any(std::in_place_type<std::string>, 40, '*');
    REQUIRE(any_cast<std::string>(d) == std::string(40, '*'));
}

TEST_CASE("any::any(std::in_place_type_t<T>, std::initializer_list<U>, Args&&...)", "[any]") {
    auto a = any(std::in_place_type<std::vector<int>>, {0, 1, 2, 3});
    REQUIRE(any_cast<std::vector<int>>(a) == (std::vector{0, 1, 2, 3}));
}

TEST_CASE("any::operator=(any const&)", "[any]") {
    auto a = any();
    auto b = a;
    REQUIRE_FALSE(b.has_value());
    a = 5;
    b = a;
    REQUIRE(any_cast<int>(a) == 5);
    REQUIRE(any_cast<int>(b) == 5);
}

TEST_CASE("any::operator=(any&&)", "[any]") {
    auto a = any();
    auto b = std::move(a);
    REQUIRE(not b.has_value());
    a = 5;
    b = std::move(a);
    REQUIRE_FALSE(a.has_value());
    REQUIRE(any_cast<int>(b) == 5);
}

TEST_CASE("any::operator=(T&&)", "[any]") {
    auto a = any();
    a = 5;
    REQUIRE(any_cast<int>(a) == 5);
    a = "ciao";
    REQUIRE(any_cast<char const*>(a) == "ciao"sv);
}

TEST_CASE("any::emplace<T>(Args&&...)", "[any]") {
    auto a = any();
    ++a.emplace<int>(5);
    REQUIRE(any_cast<int>(a) == 6);
    a.emplace<std::string>("ciao") += " mare";
    REQUIRE(any_cast<std::string>(a) == "ciao mare");
}

TEST_CASE("any::emplace<T>(std::initializer_list<U>, Args&&...)", "[any]") {
    auto a = any();
    a.emplace<std::vector<int>>({0, 1, 2}).push_back(3);
    REQUIRE(any_cast<std::vector<int>>(a) == (std::vector{0, 1, 2, 3}));
}

TEST_CASE("any::reset()", "[any]") {
    auto a = any();
    a.reset();
    a = 5;
    a.reset();
    REQUIRE_FALSE(a.has_value());
}

TEST_CASE("any::swap(any&)", "[any]") {
    any a(5), b("ciao"s);
    a.swap(b);
    REQUIRE(any_cast<std::string>(a) == "ciao");
    REQUIRE(any_cast<int>(b) == 5);
    a.reset();
    b.swap(a);
    REQUIRE(any_cast<int>(a) == 5);
    REQUIRE_FALSE(b.has_value());
}

TEST_CASE("any::has_value()", "[any]") {
    auto const a = any();
    REQUIRE_FALSE(a.has_value());
    auto const b = any(5);
    REQUIRE(b.has_value());
}

TEST_CASE("any::type()", "[any]") {
    auto const a = any(5);
    REQUIRE(a.type() == typeid(int));
}

TEST_CASE("swap(any&, any&)", "[any]") {
    any a(5), b("ciao"s);
    swap(a, b);
    REQUIRE(any_cast<std::string>(a) == "ciao");
    REQUIRE(any_cast<int>(b) == 5);
    a.reset();
    swap(b, a);
    REQUIRE(any_cast<int>(a) == 5);
    REQUIRE_FALSE(b.has_value());
}

TEST_CASE("any_cast<T>(any const&)", "[any]") {
    auto const a = any(5);
    REQUIRE(any_cast<int>(a) == 5);
    REQUIRE_THROWS_AS(any_cast<short>(a), bad_any_cast);
}

TEST_CASE("any_cast<T>(any&)", "[any]") {
    auto a = any(5);
    REQUIRE(any_cast<int>(a) == 5);
}

TEST_CASE("any_cast<T>(any&&)", "[any]") {
    auto a = any(5);
    REQUIRE(any_cast<int>(std::move(a)) == 5);
    REQUIRE(a.has_value());
    a = "ciao"s;
    REQUIRE(any_cast<std::string>(std::move(a)) == "ciao");
    REQUIRE(a.has_value());
    REQUIRE(any_cast<std::string>(a).empty());
}

TEST_CASE("any_cast<T>(any const*)", "[any]") {
    auto const a = any(5);
    REQUIRE(*any_cast<int>(&a) == 5);
    REQUIRE(any_cast<short>(&a) == nullptr);
}

TEST_CASE("any_cast<T>(any*)", "[any]") {
    auto a = any(5);
    *any_cast<int>(&a) += 3;
    REQUIRE(*any_cast<int>(&a) == 8);
}

TEST_CASE("make_any<T>(Args&&...)", "[any]") {
    auto a = make_any<int>();
    REQUIRE(any_cast<int>(a) == 0);
    a = make_any<std::vector<int>>(2, 10);
    REQUIRE((any_cast<std::vector<int>>(a) == std::vector{10, 10}));
}

TEST_CASE("make_any<T>(std::initializer_list<U>, Args&&...)", "[any]") {
    auto a = make_any<std::vector<int>>({2, 10});
    REQUIRE(any_cast<std::vector<int>>(a) == (std::vector{2, 10}));
}
