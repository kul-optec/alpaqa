#include <hyhound-version.h>
#include <hyhound/householder-updowndate.hpp>

#include <guanaqo/print.hpp>
#include <iostream>
#include <vector>

int main() {
    using namespace hyhound;
    std::cout << "# hyhound " << HYHOUND_VERSION << " (" << hyhound_commit_hash << ")" << std::endl;
    index_t n = 3, m = 2;
    std::vector<real_t> L_sto(n * n), A_sto(n * m);
    MatrixView<real_t> L{{.data = L_sto.data(), .rows = n, .cols = n}};
    MatrixView<real_t> A{{.data = A_sto.data(), .rows = n, .cols = m}};
    L.set_constant(4, guanaqo::Triangular::Lower);
    A.set_constant(1);
    A(1, 1) = -1;
    std::vector<real_t> S{2, -1};
    std::cout << "import numpy as np\n";
    guanaqo::print_python(std::cout << "L = np.array(\n", L, ")\n");
    guanaqo::print_python(std::cout << "A = np.array(\n", A, ")\n");
    guanaqo::print_python(std::cout << "S = np.array(", std::span{S}, ")\n");
    update_cholesky(L, A, DiagonalUpDowndate{S});
    guanaqo::print_python(std::cout << "L̃ = np.array(\n", L, ")\n");
    std::cout << "print(L @ L.T + A @ (S[:, None] * A.T) - L̃ @ L̃.T)\n";
}
