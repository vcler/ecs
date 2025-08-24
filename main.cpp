#include <iostream>
#include <ostream>
#include <print>

#include "colony.hpp"


struct X {
    X(double value) : value(value) { std::println("X() {}", value); }
    ~X() { std::println("~X() {}", value); }
    double value;
};

std::ostream &operator<<(std::ostream &os, X &x) { return os << x.value; }

int main()
{
    colony<X> col;
    col.push_back(5.0);
    col.push_back(1.0);
    col.push_back(3.0);

    col.erase(1);

    //std::cout << col.at(0) << '\n';

    for (auto &x : col) {
        std::cout << x << '\n';
    }
    
}

