#include <guanaqo-version.h>
#include <guanaqo/demangled-typename.hpp>
#include <iostream>

int main() {
    std::cout << "guanaqo " << GUANAQO_VERSION << " (" << guanaqo_commit_hash << ")" << std::endl;
    struct {
    } s;
    std::cout << guanaqo::demangled_typename(typeid(s)) << std::endl;
}
