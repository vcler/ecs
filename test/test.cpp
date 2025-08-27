#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "colony.hpp" // Adjust to your actual header location
#include <string>
#include <vector>
#include <type_traits>

// Example type for complex construction
struct MyStruct {
    int id;
    std::string name;

    MyStruct(int i, std::string n) : id(i), name(std::move(n)) {}
    bool operator==(const MyStruct &other) const {
        return id == other.id && name == other.name;
    }
};

TEST_CASE("Basic push_back and access") {
    colony<double> c;
    auto idx1 = c.push_back(10);
    auto idx2 = c.push_back(20);

    CHECK(c.size() == 2);
    CHECK(c.at(idx1) == 10);
    CHECK(c.at(idx2) == 20);
}

TEST_CASE("emplace_back and complex types") {
    colony<MyStruct> c;
    auto idx = c.emplace_back(1, "Alice");

    CHECK(c.size() == 1);
    CHECK(c.at(idx) == MyStruct(1, "Alice"));
}

TEST_CASE("erase by index") {
    colony<double> c;
    auto i1 = c.push_back(1);
    auto i2 = c.push_back(2);
    auto i3 = c.push_back(3);

    c.erase(i2);

    CHECK(c.size() == 2);
    CHECK_THROWS_AS(c.at(i2), std::out_of_range);
    CHECK(c.at(i1) == 1);
    CHECK(c.at(i3) == 3);
}

TEST_CASE("erase by iterator") {
    colony<double> c;
    std::vector<colony<double>::size_type> ids;
    for (int i = 0; i < 5; ++i)
        ids.push_back(c.push_back(i));

    auto it = c.begin();
    ++it; // Pointing to 1
    auto new_it = c.erase(it);

    CHECK(c.size() == 4);
    CHECK((*new_it) == 2);
}

TEST_CASE("iterator traversal") {
    colony<double> c;
    std::vector<int> values = {5, 10, 15};
    for (int v : values) c.push_back(v);

    std::vector<int> result;
    for (auto it = c.begin(); it != c.end(); ++it) {
        result.push_back(*it);
    }

    CHECK(result == values);
}

TEST_CASE("iterator validity after insertions") {
    colony<double> c;
    c.push_back(1);
    c.push_back(2);
    auto it = c.begin(); // Should point to 1

    c.push_back(3); // Insert after capturing iterator

    CHECK(*it == 1); // Still valid
}

TEST_CASE("clear and reuse") {
    colony<std::string> c;
    c.push_back("first");
    c.push_back("second");
    c.clear();

    CHECK(c.size() == 0);

    auto idx = c.push_back("new");
    CHECK(c.size() == 1);
    CHECK(c.at(idx) == "new");
}

TEST_CASE("capacity and next") {
    colony<double> c;
    auto i1 = c.push_back(100);
    auto i2 = c.push_back(200);
    auto i3 = c.push_back(300);

    c.erase(i2);

    CHECK(c.next(i1) == i3);
    CHECK_THROWS_AS(c.at(i2), std::out_of_range);
}

TEST_CASE("const correctness") {
    colony<double> c;
    c.push_back(1);
    c.push_back(2);

    const auto &cc = c;
    auto it = cc.begin();
    CHECK(std::is_same_v<decltype(it), colony<double>::const_iterator>);
}

