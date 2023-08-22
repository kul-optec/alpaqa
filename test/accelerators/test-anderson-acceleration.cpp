#include <alpaqa/config/config.hpp>

#include <gtest/gtest.h>
#include <test-util/eigen-matchers.hpp>

#include <alpaqa/accelerators/internal/anderson-helpers.hpp>
#include <alpaqa/accelerators/internal/limited-memory-qr.hpp>
#include <Eigen/LU>
#include <Eigen/QR>
#include <iomanip>

USING_ALPAQA_CONFIG(alpaqa::DefaultConfig);

mat rotate_add(crmat m, crvec v) {
    mat result(m.rows(), m.cols());
    result.block(0, 0, m.rows(), m.cols() - 1) =
        m.block(0, 1, m.rows(), m.cols() - 1);
    result.rightCols(1) = v;
    return result;
}

TEST(Anderson, minimize) {
    length_t n = 4;
    length_t m = 3;
    length_t K = 7;

    mat R(n, K);
    mat ΔR(n, K - 1);
    mat X(n, K);
    mat G(n, K);
    alpaqa::LimitedMemoryQR qr(4, 3);

    X << 1, 2, 3, 4, 5, 6, 7, //
        2, 4, 3, 7, 1, 2, 4,  //
        1, 3, 2, 1, 2, 1, 3,  //
        5, 2, 1, 3, 3, 4, -1;

    G << 7, 6, 5, 4, 3, 2, 1,  //
        4, 4, 1, 2, 7, 1, -2,  //
        0, 6, 2, 2, -9, -4, 6, //
        0, 8, 1, 2, -1, 1, -1;

    // rₖ = gₖ - xₖ
    for (length_t i = 0; i < K; ++i)
        R.col(i) = G.col(i) - X.col(i);
    // Δrₖ = rₖ₊₁ - rₖ
    for (length_t i = 0; i < K - 1; ++i)
        ΔR.col(i) = R.col(i + 1) - R.col(i);
    // QR factorization of ΔR
    qr.add_column(ΔR.col(0));
    qr.add_column(ΔR.col(1));

    // First iteration ---------------------------------------------------------
    length_t k = 3;

    vec γ_LS(m);
    vec xₖ_aa(n);
    mat G0 = G.block(0, 0, n, m);
    // Call function under test
    alpaqa::minimize_update_anderson(qr, G0, R.col(k), R.col(k - 1), G.col(k),
                                     0, γ_LS, xₖ_aa);

    // Compute reference solution
    // Gref = (g₀ | g₁ | g₂ | g₃)
    mat Gref  = G.block(0, k - m, n, m + 1);
    mat ΔRref = ΔR.block(0, k - m, n, m);
    // γ = argmin ‖ ΔR γ - r₃ ‖²
    vec γ_exp = ΔRref.colPivHouseholderQr().solve(R.col(k));
    // α₀ = γ₀
    // αₙ = γₙ - γₙ₋₁
    // αₘ = 1  - γₘ₋₁
    vec α(m + 1);
    α(0) = γ_exp(0);
    for (length_t i = 1; i < m; ++i)
        α(i) = γ_exp(i) - γ_exp(i - 1);
    α(m) = 1 - γ_exp(m - 1);
    // x = ∑ₙ₌₀ αₙ gₙ
    vec x_exp = Gref * α;

    constexpr real_t ε = 5e-14;
    std::cout << std::setprecision(16) << std::scientific;
    std::cout << γ_LS.transpose() << std::endl;
    std::cout << γ_exp.transpose() << std::endl;
    EXPECT_THAT(γ_LS, EigenAlmostEqual(γ_exp, ε));

    std::cout << xₖ_aa.transpose() << std::endl;
    std::cout << x_exp.transpose() << std::endl;
    EXPECT_THAT(xₖ_aa, EigenAlmostEqual(x_exp, ε));

    // Oldest column of G should have been overwritten by g₃
    mat G1(4, 3);
    G1.col(1) = G.col(1);
    G1.col(2) = G.col(2);
    G1.col(0) = G.col(3);
    EXPECT_THAT(G0, EigenAlmostEqual(G1, ε));

    mat QᵀQ = qr.get_Q().transpose() * qr.get_Q();
    EXPECT_THAT(qr.get_Q() * qr.get_R(), EigenAlmostEqual(ΔRref, ε));
    EXPECT_THAT(QᵀQ, EigenAlmostEqual(mat::Identity(m, m), ε));
    std::cout << "\nR:\n" << qr.get_R() << std::endl;
    std::cout << "\nQᵀQ:\n" << QᵀQ << std::endl;
    std::cout << std::endl;

    // Next iteration
    // -------------------------------------------------------------------------

    ++k;

    // Call function under test
    alpaqa::minimize_update_anderson(qr, G1, R.col(k), R.col(k - 1), G.col(k),
                                     0, γ_LS, xₖ_aa);

    // Compute reference solution
    // Gref = (g₁ | g₂ | g₃ | g₄)
    Gref  = G.block(0, k - m, n, m + 1);
    ΔRref = ΔR.block(0, k - m, n, m);
    // γ = argmin ‖ ΔR γ - r₃ ‖²
    γ_exp = ΔRref.colPivHouseholderQr().solve(R.col(k));
    // α₀ = γ₀
    // αₙ = γₙ - γₙ₋₁
    // αₘ = 1  - γₘ₋₁
    α(0) = γ_exp(0);
    for (length_t i = 1; i < m; ++i)
        α(i) = γ_exp(i) - γ_exp(i - 1);
    α(m) = 1 - γ_exp(m - 1);
    // x = ∑ₙ₌₀ αₙ gₙ
    x_exp = Gref * α;

    std::cout << γ_LS.transpose() << std::endl;
    std::cout << γ_exp.transpose() << std::endl;
    EXPECT_THAT(γ_LS, EigenAlmostEqual(γ_exp, ε));

    std::cout << xₖ_aa.transpose() << std::endl;
    std::cout << x_exp.transpose() << std::endl;
    EXPECT_THAT(xₖ_aa, EigenAlmostEqual(x_exp, ε));

    // Oldest column of G should have been overwritten by g₄
    mat G2(4, 3);
    G2.col(2) = G.col(2);
    G2.col(0) = G.col(3);
    G2.col(1) = G.col(4);
    EXPECT_THAT(G1, EigenAlmostEqual(G2, ε));

    QᵀQ = qr.get_Q().transpose() * qr.get_Q();
    EXPECT_THAT(qr.get_Q() * qr.get_R(), EigenAlmostEqual(ΔRref, ε));
    EXPECT_THAT(QᵀQ, EigenAlmostEqual(mat::Identity(m, m), ε));
    std::cout << "\nR:\n" << qr.get_R() << std::endl;
    std::cout << "\nQᵀQ:\n" << QᵀQ << std::endl;
    std::cout << std::endl;

    // Next iteration
    // -------------------------------------------------------------------------

    ++k;

    // Call function under test
    alpaqa::minimize_update_anderson(qr, G2, R.col(k), R.col(k - 1), G.col(k),
                                     0, γ_LS, xₖ_aa);

    // Compute reference solution
    // Gref = (g₂ | g₃ | g₄ | g₅)
    Gref  = G.block(0, k - m, n, m + 1);
    ΔRref = ΔR.block(0, k - m, n, m);
    // γ = argmin ‖ ΔR γ - r₃ ‖²
    γ_exp = ΔRref.colPivHouseholderQr().solve(R.col(k));
    // α₀ = γ₀
    // αₙ = γₙ - γₙ₋₁
    // αₘ = 1  - γₘ₋₁
    α(0) = γ_exp(0);
    for (length_t i = 1; i < m; ++i)
        α(i) = γ_exp(i) - γ_exp(i - 1);
    α(m) = 1 - γ_exp(m - 1);
    // x = ∑ₙ₌₀ αₙ gₙ
    x_exp = Gref * α;

    std::cout << γ_LS.transpose() << std::endl;
    std::cout << γ_exp.transpose() << std::endl;
    EXPECT_THAT(γ_LS, EigenAlmostEqual(γ_exp, ε));

    std::cout << xₖ_aa.transpose() << std::endl;
    std::cout << x_exp.transpose() << std::endl;
    EXPECT_THAT(xₖ_aa, EigenAlmostEqual(x_exp, ε));

    // Oldest column of G should have been overwritten by g₅
    mat G3(4, 3);
    G3.col(0) = G.col(3);
    G3.col(1) = G.col(4);
    G3.col(2) = G.col(5);
    EXPECT_THAT(G2, EigenAlmostEqual(G3, ε));

    QᵀQ = qr.get_Q().transpose() * qr.get_Q();
    EXPECT_THAT(qr.get_Q() * qr.get_R(), EigenAlmostEqual(ΔRref, ε));
    EXPECT_THAT(QᵀQ, EigenAlmostEqual(mat::Identity(m, m), ε));
    std::cout << "\nR:\n" << qr.get_R() << std::endl;
    std::cout << "\nQᵀQ:\n" << QᵀQ << std::endl;
    std::cout << std::endl;

    // Next iteration
    // -------------------------------------------------------------------------

    ++k;

    // Call function under test
    alpaqa::minimize_update_anderson(qr, G3, R.col(k), R.col(k - 1), G.col(k),
                                     0, γ_LS, xₖ_aa);

    // Compute reference solution
    // Gref = (g₃ | g₄ | g₅ | g₆)
    Gref  = G.block(0, k - m, n, m + 1);
    ΔRref = ΔR.block(0, k - m, n, m);
    // γ = argmin ‖ ΔR γ - r₃ ‖²
    γ_exp = ΔRref.colPivHouseholderQr().solve(R.col(k));
    // α₀ = γ₀
    // αₙ = γₙ - γₙ₋₁
    // αₘ = 1  - γₘ₋₁
    α(0) = γ_exp(0);
    for (length_t i = 1; i < m; ++i)
        α(i) = γ_exp(i) - γ_exp(i - 1);
    α(m) = 1 - γ_exp(m - 1);
    // x = ∑ₙ₌₀ αₙ gₙ
    x_exp = Gref * α;

    std::cout << γ_LS.transpose() << std::endl;
    std::cout << γ_exp.transpose() << std::endl;
    EXPECT_THAT(γ_LS, EigenAlmostEqual(γ_exp, ε));

    std::cout << xₖ_aa.transpose() << std::endl;
    std::cout << x_exp.transpose() << std::endl;
    EXPECT_THAT(xₖ_aa, EigenAlmostEqual(x_exp, ε));

    // Oldest column of G should have been overwritten by g₆
    mat G4(4, 3);
    G4.col(1) = G.col(4);
    G4.col(2) = G.col(5);
    G4.col(0) = G.col(6);
    EXPECT_THAT(G3, EigenAlmostEqual(G4, ε));

    QᵀQ = qr.get_Q().transpose() * qr.get_Q();
    EXPECT_THAT(qr.get_Q() * qr.get_R(), EigenAlmostEqual(ΔRref, ε));
    EXPECT_THAT(QᵀQ, EigenAlmostEqual(mat::Identity(m, m), ε));
    std::cout << "\nR:\n" << qr.get_R() << std::endl;
    std::cout << "\nQᵀQ:\n" << QᵀQ << std::endl;
    std::cout << std::endl;
}

TEST(Anderson, matrix) {
    mat A(2, 2);
    A << 20, -10, -10, 30;

    // Ax - b = x
    // [ 20 -10] [1] - [ 9] = [1]
    // [-10  30] [1]   [19] = [1]

    vec xₖ(2), b(2);
    b << 9, 19;
    xₖ = -b;

    auto g = [&](crvec x) -> vec { return A * x - b; };
    auto r = [&](crvec x) -> vec { return g(x) - x; };

    std::cout << "A: \n" << A << std::endl;
    std::cout << "A⁻¹: \n" << A.inverse() << std::endl;

    alpaqa::LimitedMemoryQR qr(2, 2);
    mat G(qr.n(), qr.m());
    vec rₖ    = r(xₖ);
    vec rₙₑₓₜ = rₖ;
    std::vector<real_t> res;
    unsigned update_count = 0;
    for (length_t i = 0; i < 5; ++i) {
        { // Print BFGS estimate
            std::cout << "\nIter:  " << i << std::endl;
            std::cout << "Updates: " << update_count << std::endl;
            std::cout << "x:    " << xₖ.transpose() << std::endl;
            std::cout << "g(x): " << g(xₖ).transpose() << std::endl;
            std::cout << "r(x): " << rₖ.transpose() << std::endl;
            std::cout << "R = \n" << qr.get_Q() * qr.get_R() << std::endl;
            res.push_back(rₖ.norm());
        }

        vec γ_LS(qr.n()), xₖ_aa(qr.n());
        vec gₖ      = A * xₖ - b;
        auto &rₗₐₛₜ = rₙₑₓₜ;
        if (i == 0) {
            G.col(0) = gₖ;
            xₖ_aa    = gₖ;
        } else {
            alpaqa::minimize_update_anderson(qr, G, rₖ, rₗₐₛₜ, gₖ, 0, γ_LS,
                                             xₖ_aa);
            ++update_count;
        }
        rₙₑₓₜ = A * xₖ_aa - b - xₖ_aa;

        std::swap(rₖ, rₙₑₓₜ);
        xₖ = std::move(xₖ_aa);
    }
    std::cout << "\nfinal" << std::endl;
    std::cout << "x:    " << xₖ.transpose() << std::endl;
    std::cout << "Ax - b - x: " << (A * xₖ - b - xₖ).transpose() << std::endl;
    std::cout << "[";
    for (auto r : res)
        std::cout << r << ", ";
    std::cout << "]" << std::endl;

    EXPECT_NEAR(xₖ(0), 1, 1e-10);
    EXPECT_NEAR(xₖ(1), 1, 1e-10);
}

#include <alpaqa/accelerators/anderson.hpp>

TEST(Anderson, matrix2) {
    mat A(2, 2);
    A << 20, -10, -10, 30;

    // Ax - b = x
    // [ 20 -10] [1] - [ 9] = [1]
    // [-10  30] [1]   [19] = [1]

    vec xₖ(2), b(2);
    b << 9, 19;
    xₖ = -b;

    auto g = [&](crvec x) -> vec { return A * x - b; };
    auto r = [&](crvec x) -> vec { return g(x) - x; };

    std::cout << "A: \n" << A << std::endl;
    std::cout << "A⁻¹: \n" << A.inverse() << std::endl;

    alpaqa::AndersonAccel aa({2}, 2);
    std::vector<real_t> res;
    unsigned update_count = 0;

    for (length_t i = 0; i < 5; ++i) {
        { // Print BFGS estimate
            std::cout << "\nIter:  " << i << std::endl;
            std::cout << "Updates: " << update_count << std::endl;
            std::cout << "x:    " << xₖ.transpose() << std::endl;
            std::cout << "g(x): " << g(xₖ).transpose() << std::endl;
            std::cout << "r(x): " << r(xₖ).transpose() << std::endl;
            res.push_back(r(xₖ).norm());
        }

        vec xₙₑₓₜ(2);
        if (i == 0) {
            aa.initialize(g(xₖ), r(xₖ));
            xₙₑₓₜ = g(xₖ);
        } else {
            aa.compute(g(xₖ), r(xₖ), xₙₑₓₜ);
            ++update_count;
        }
        xₖ = std::move(xₙₑₓₜ);
    }
    std::cout << "\nfinal" << std::endl;
    std::cout << "x:    " << xₖ.transpose() << std::endl;
    std::cout << "g(x): " << g(xₖ).transpose() << std::endl;
    std::cout << "r(x): " << r(xₖ).transpose() << std::endl;
    std::cout << "[";
    for (auto r : res)
        std::cout << r << ", ";
    std::cout << "]" << std::endl;

    EXPECT_NEAR(xₖ(0), 1, 1e-10);
    EXPECT_NEAR(xₖ(1), 1, 1e-10);
}
