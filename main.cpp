#include <boost/dynamic_bitset.hpp>
#include <print>

#include "colony.hpp"

int main()
{

    colony<double> col;
    col.push_back(5.0);
    col.push_back(1.0);
    col.push_back(3.0);
    std::println("asdf");
    std::println("{}", col.at(0));
}

