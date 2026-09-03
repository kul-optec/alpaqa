#include <casadi/casadi.hpp>
#include <casadi/config.h>
#include <iostream>

int main() {
    const casadi::SX x = casadi::SX::sym("x");
    std::cout << "Test package for CasADi " << CASADI_VERSION_STRING << '\n';
    std::cout << "Symbolic expression: " << x * x << '\n';
}
