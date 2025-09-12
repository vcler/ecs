#include <iostream>
#include <ostream>
#include <print>

#include "registry.hpp"


struct X {
    X(double value) : valuex(value) { std::println("X() {}", value); }
    ~X() { std::println("~X() {}", valuex); }
    double valuex;
};

struct Y {
    Y(double value) : valuey(value) { std::println("Y() {}", value); }
    ~Y() { std::println("~Y() {}", valuey); }
    double valuey;
};

struct Z {
    Z(double value) : valuez(value) { std::println("Z() {}", value); }
    ~Z() { std::println("~Z() {}", valuez); }
    double valuez;
};


std::ostream &operator<<(std::ostream &os, X &x) { return os << x.valuex; }
std::ostream &operator<<(std::ostream &os, Y &y) { return os << y.valuey; }

int main()
{
    ecs::registry reg;

    reg.create(X{1.0}, Y{2.0}, Z{3.0});
    reg.create(X{1.0}, Y{2.0}, Z{3.0});
    auto ent =reg.create(X{1.0}, Y{1.0});
    reg.emplace(ent, Z{99.0});

    reg.singleton<X>(X{-1.0});
    auto &x2 = reg.singleton<X>();
    std::println("{}", x2.valuex);

    //X &cx = reg.get<X>(ent);
    //std::println("x == {}", cx.valuex);

    /*
    auto range = reg.range<Y, X>();
    auto it = range.begin();
    auto [x, y]  = *it;
    */

    for (auto &[z, y, x] : reg.range<Z, Y, X>()) {
        std::println("X == {}", x.valuex);
        std::println("Y == {}", y.valuey);
        std::println("Z == {}", z.valuez);
        //std::println("Y {}", y.valuey);
    }

    //reg.destroy<X, Y, Z>(ent);



    reg.create(X{4.0}, Y{5.0}, Z{6.0});

    for (auto &[x, y] : reg.range<X, Y>()) {
        std::println("X == {}", x.valuex);
        std::println("Y == {}", y.valuey);
        //std::println("Z == {}", z.valuez);
    }

    for (auto &x : reg.range<X>()) {
        std::println("X == {}", x.valuex);
    }
    /*
    for (auto [y, x, z] : reg.range<Y, X, Z>()) {
        std::println("X == {}", x.valuex);
        std::println("Y == {}", y.valuey);
        std::println("Z == {}", z.valuez);
    }
    */

    /*
    colony<X> col;
    col.push_back(5.0);
    col.push_back(1.0);
    col.push_back(3.0);
    col.erase(1);

    //std::cout << col.at(0) << '\n';

    for (auto &x : col) {
        std::cout << x << '\n';
    }
    */
    return 0;
}

