#include <alpaqa/newton-tr-pantr-alm.hpp>
#include <alpaqa/panoc-alm.hpp>
#include <alpaqa/panoc-anderson-alm.hpp>
#include <alpaqa/problem/functional-problem.hpp>
#include <alpaqa/structured-panoc-alm.hpp>

#include <test-util/eigen-matchers.hpp>

TEST(ALM, singleshooting1D) {
    USING_ALPAQA_CONFIG(alpaqa::EigenConfigd);
    using namespace alpaqa;

    Box<config_t> C{1};
    C.lowerbound << -1;
    C.upperbound << 1;
    Box<config_t> D{0};

    using Diag = Eigen::DiagonalMatrix<real_t, Eigen::Dynamic, Eigen::Dynamic>;

    auto A = mat::Constant(1, 1, 0.5);
    auto B = mat::Constant(1, 1, 1);

    auto Q = Diag(1);
    Q.diagonal() << 10;
    auto R = Diag(1);
    R.diagonal() << 1;

    auto x0 = vec(1);
    x0 << 1;

    auto f = [&](crvec u) {
        auto x1 = A * x0 + B * u;
        return x1.dot(Q * x1) + u.dot(R * u);
    };
    auto grad_f = [&](crvec u, rvec grad_f) {
        grad_f = 2 * B * Q * (A * x0 + B * u) + 2 * R * u;
    };
    auto g      = [&](crvec u, rvec g_u) { (void)u, void(g_u); };
    auto grad_g = [&](crvec u, crvec v, rvec grad_u_v) {
        (void)u, (void)v, grad_u_v(0) = 0;
    };

    FunctionalProblem<config_t> op{C, D};
    op.f           = f;
    op.grad_f      = grad_f;
    op.g           = g;
    op.grad_g_prod = grad_g;

    using Direction   = alpaqa::LBFGSDirection<config_t>;
    using PANOCSolver = alpaqa::PANOCSolver<Direction>;
    using ALMSolver   = alpaqa::ALMSolver<PANOCSolver>;

    ALMSolver::Params almparam;
    almparam.tolerance                      = 1e-4;
    almparam.dual_tolerance                 = 1e-4;
    almparam.penalty_update_factor          = 100;
    almparam.initial_penalty                = 20;
    almparam.initial_tolerance              = 1;
    almparam.rel_penalty_increase_threshold = 0.25;
    almparam.tolerance_update_factor        = 1e-1;
    almparam.max_multiplier                 = 1e9;
    almparam.max_penalty                    = 1e9;
    almparam.max_iter                       = 10;

    PANOCSolver::Params panocparam;
    panocparam.Lipschitz.ε = 1e-6;
    panocparam.Lipschitz.δ = 1e-12;
    panocparam.max_iter    = 100;

    Direction::AcceleratorParams lbfgsparam;
    lbfgsparam.memory = 10;

    ALMSolver solver{almparam, {panocparam, lbfgsparam}};

    vec x(1);
    x << 1;
    vec y(0);
    solver(op, x, y);

    EXPECT_NEAR(x(0), -0.454545, 1e-4);
    std::cout << "u = " << x.transpose() << std::endl;
    std::cout << "x = " << (A * x0 + B * x).transpose() << std::endl;
}

TEST(ALM, multipleshooting1D) {
    USING_ALPAQA_CONFIG(alpaqa::EigenConfigd);
    using namespace alpaqa;

    Box<config_t> C{2};
    C.lowerbound << -1, -inf<config_t>;
    C.upperbound << 1, inf<config_t>;
    Box<config_t> D{1};
    D.lowerbound << 0;
    D.upperbound << 0;

    using Diag = Eigen::DiagonalMatrix<real_t, Eigen::Dynamic, Eigen::Dynamic>;

    auto A = mat::Constant(1, 1, 0.5);
    auto B = mat::Constant(1, 1, 1);

    auto Q = Diag(1);
    Q.diagonal() << 10;
    auto R = Diag(1);
    R.diagonal() << 1;

    auto x0 = vec(1);
    x0 << 1;

    auto f = [&](crvec ux) {
        auto u = ux.topRows(1);
        auto x = ux.bottomRows(1);
        return x.dot(Q * x) + u.dot(R * u);
    };
    auto grad_f = [&](crvec ux, rvec grad_f) {
        auto u               = ux.topRows(1);
        auto x               = ux.bottomRows(1);
        grad_f.topRows(1)    = 2 * R * u;
        grad_f.bottomRows(1) = 2 * Q * x;
    };
    auto g = [&](crvec ux, rvec g_u) {
        auto u         = ux.topRows(1);
        auto x         = ux.bottomRows(1);
        g_u.topRows(1) = A * x0 + B * u - x;
    };
    auto grad_g = [&](crvec ux, crvec v, rvec grad_u_v) {
        (void)ux;
        grad_u_v.topRows(1)    = B * v;
        grad_u_v.bottomRows(1) = -mat::Identity(1, 1) * v;
    };

    FunctionalProblem<config_t> op{C, D};
    op.f           = f;
    op.grad_f      = grad_f;
    op.g           = g;
    op.grad_g_prod = grad_g;

    using Direction   = alpaqa::LBFGSDirection<config_t>;
    using PANOCSolver = alpaqa::PANOCSolver<Direction>;
    using ALMSolver   = alpaqa::ALMSolver<PANOCSolver>;

    ALMSolver::Params almparam;
    almparam.tolerance                      = 1e-4;
    almparam.dual_tolerance                 = 1e-4;
    almparam.penalty_update_factor          = 5;
    almparam.initial_penalty                = 1;
    almparam.initial_tolerance              = 1e-4;
    almparam.rel_penalty_increase_threshold = 0.25;
    almparam.tolerance_update_factor        = 1e-1;
    almparam.max_multiplier                 = 1e9;
    almparam.max_penalty                    = 1e9;
    almparam.max_iter                       = 10;

    PANOCSolver::Params panocparam;
    panocparam.Lipschitz.ε = 1e-6;
    panocparam.Lipschitz.δ = 1e-12;
    panocparam.max_iter    = 100;

    Direction::AcceleratorParams lbfgsparam;
    lbfgsparam.memory = 10;

    ALMSolver solver{almparam, {panocparam, lbfgsparam}};

    vec x(2);
    x << 0.5, 0.5;
    vec y(1);
    y << 1;

    auto begin = std::chrono::high_resolution_clock::now();
    auto stats = solver(op, x, y);
    auto end   = std::chrono::high_resolution_clock::now();

    std::cout << "u = " << x.topRows(1).transpose() << std::endl;
    std::cout << "x = " << x.bottomRows(1).transpose() << std::endl;
    std::cout << "Inner: " << stats.inner.iterations
              << ", Outer: " << stats.outer_iterations << std::endl;

    auto duration = std::chrono::duration<double>(end - begin);
    std::cout << duration.count() * 1e3 << "ms" << std::endl;

    EXPECT_NEAR(x(0), -0.454545, 1e-4);
}

auto build_ms_problem() {
    using namespace alpaqa;
    USING_ALPAQA_CONFIG(DefaultConfig);
    length_t nx = 8, nu = 8;
    length_t n = nu + nx;
    length_t m = nx;

    Box<config_t> C{n};
    C.lowerbound.topRows(nu).fill(-1);
    C.lowerbound.bottomRows(nx).fill(-inf<config_t>);
    C.upperbound.topRows(nu).fill(1);
    C.upperbound.bottomRows(nx).fill(inf<config_t>);
    Box<config_t> D{m};
    D.lowerbound.fill(0);
    D.upperbound.fill(0);

#include "matrices/test-alm.mat.ipp"

    using Diag = Eigen::DiagonalMatrix<real_t, Eigen::Dynamic, Eigen::Dynamic>;

    auto Q = Diag(nx);
    Q.diagonal().fill(10);
    auto R = Diag(nu);
    R.diagonal().fill(1);

    auto x0 = vec(nx);
    x0.fill(1);

    mat G(nx + nu, nx);
    G << B.transpose(), -mat::Identity(nx, nx);

    FunctionalProblem<config_t> op{C, D};
    op.f = [=](crvec ux) {
        auto u = ux.topRows(nu);
        auto x = ux.bottomRows(nx);
        return x.dot(Q * x) + u.dot(R * u);
    };
    op.grad_f = [=](crvec ux, rvec grad_f) {
        auto u                = ux.topRows(nu);
        auto x                = ux.bottomRows(nx);
        grad_f.topRows(nu)    = 2 * (R * u);
        grad_f.bottomRows(nx) = 2 * (Q * x);
    };
    op.g = [=](crvec ux, rvec g_u) {
        auto u         = ux.topRows(nu);
        auto x         = ux.bottomRows(nx);
        g_u.topRows(m) = A * x0 + B * u - x;
    };
    op.grad_g_prod = [=](crvec ux, crvec v, rvec grad_u_v) {
        (void)ux;
        grad_u_v.topRows(nu)    = B.transpose() * v;
        grad_u_v.bottomRows(nx) = -v;
    };
    op.hess_ψ_prod = [=](crvec, crvec, crvec Σ, real_t scale, crvec ux,
                         rvec Hv) {
        auto u            = ux.topRows(nu);
        auto x            = ux.bottomRows(nx);
        Hv.topRows(nu)    = 2 * scale * R * u;
        Hv.bottomRows(nx) = 2 * scale * Q * x;
        Hv += G * Σ.asDiagonal() * (G.transpose() * ux);
    };
    return std::tuple{std::move(op), nx, nu};
}

template <class>
class PANOC : public testing::Test {};
TYPED_TEST_SUITE_P(PANOC);

TYPED_TEST_P(PANOC, multipleshooting8D) {
    USING_ALPAQA_CONFIG_TEMPLATE(TypeParam::config_t);
    using namespace alpaqa;

    auto [op, nx, nu] = build_ms_problem();

    using Direction   = TypeParam;
    using PANOCSolver = alpaqa::PANOCSolver<Direction>;
    using ALMSolver   = alpaqa::ALMSolver<PANOCSolver>;

    typename ALMSolver::Params almparam;
    almparam.tolerance                      = 1e-4;
    almparam.dual_tolerance                 = 1e-4;
    almparam.penalty_update_factor          = 5;
    almparam.initial_penalty                = 1;
    almparam.initial_tolerance              = 1e-4;
    almparam.rel_penalty_increase_threshold = 0.25;
    almparam.tolerance_update_factor        = 1e-1;
    almparam.max_multiplier                 = 1e9;
    almparam.max_penalty                    = 1e9;
    almparam.max_iter                       = 20;

    typename PANOCSolver::Params panocparam;
    panocparam.Lipschitz.ε = 1e-6;
    panocparam.Lipschitz.δ = 1e-12;
    panocparam.max_iter    = 200;

    typename Direction::AcceleratorParams accelparam;
    accelparam.memory = 10;

    ALMSolver solver{almparam, PANOCSolver{panocparam, Direction{accelparam}}};

    vec x(op.get_n());
    x.fill(5);
    vec y(op.get_m());
    y.fill(1);

    auto begin = std::chrono::high_resolution_clock::now();
    auto stats = solver(op, x, y);
    auto end   = std::chrono::high_resolution_clock::now();

    std::cout << "u = " << x.topRows(nu).transpose() << std::endl;
    std::cout << "x = " << x.bottomRows(nx).transpose() << std::endl;
    std::cout << "y = " << y.transpose() << std::endl;
    std::cout << "f(x) = " << op.eval_f(x) << std::endl;
    auto gx = vec(op.get_m());
    op.eval_g(x, gx);
    std::cout << "g(x) = " << gx.transpose() << std::endl;
    std::cout << "Inner: " << stats.inner.iterations
              << ", Outer: " << stats.outer_iterations << std::endl;

    auto duration = std::chrono::duration<double>(end - begin);
    std::cout << duration.count() * 1e3 << "ms" << std::endl;

    auto u  = x.topRows(nu);
    auto xs = x.bottomRows(nx);
    auto ε  = 6e-4;

    vec u_ref(nu);
    u_ref << -1, -1, -1, -1, -1, -0.878565, -0.719274, -0.999999;
    EXPECT_THAT(u, EigenAlmostEqual(u_ref, ε));

    vec xs_ref(nx);
    xs_ref << 0.312975, -1.14893, -0.03111, 1.69392, -0.11316, 1.60049,
        0.177012, 1.14043;
    EXPECT_THAT(xs, EigenAlmostEqual(xs_ref, ε));

    vec y_ref(op.get_m());
    y_ref << 6.2595, -22.9787, -0.6222, 33.8783, -2.2632, 32.0098, 3.54023,
        22.8086;
    EXPECT_THAT(y, EigenAlmostEqual(y_ref, ε));
}

REGISTER_TYPED_TEST_SUITE_P(PANOC, multipleshooting8D);

using PANOCDirectionTypes =
    ::testing::Types<alpaqa::LBFGSDirection<alpaqa::DefaultConfig>,
                     alpaqa::StructuredLBFGSDirection<alpaqa::DefaultConfig>,
                     alpaqa::AndersonDirection<alpaqa::DefaultConfig>>;
INSTANTIATE_TYPED_TEST_SUITE_P(ALM, PANOC, PANOCDirectionTypes);

template <class>
class PANTR : public testing::Test {};
TYPED_TEST_SUITE_P(PANTR);

TYPED_TEST_P(PANTR, multipleshooting8D) {
    USING_ALPAQA_CONFIG_TEMPLATE(TypeParam::Direction::config_t);
    using namespace alpaqa;

    auto [op, nx, nu] = build_ms_problem();

    using Direction = typename TypeParam::Direction;
    using Solver    = alpaqa::PANTRSolver<Direction>;
    using ALMSolver = alpaqa::ALMSolver<Solver>;

    typename ALMSolver::Params almparam;
    almparam.tolerance                      = 1e-4;
    almparam.dual_tolerance                 = 1e-4;
    almparam.penalty_update_factor          = 5;
    almparam.initial_penalty                = 1;
    almparam.initial_tolerance              = 1e-4;
    almparam.rel_penalty_increase_threshold = 0.25;
    almparam.tolerance_update_factor        = 1e-1;
    almparam.max_multiplier                 = 1e9;
    almparam.max_penalty                    = 1e9;
    almparam.max_iter                       = 20;

    typename Solver::Params innerparam;
    innerparam.Lipschitz.ε = 1e-6;
    innerparam.Lipschitz.δ = 1e-12;
    innerparam.max_iter    = 200;

    typename Direction::DirectionParams dirparam;
    dirparam.finite_diff = TypeParam::finite_diff;

    ALMSolver solver{almparam, Solver{innerparam, Direction{{}, dirparam}}};

    vec x(op.get_n());
    x.fill(5);
    vec y(op.get_m());
    y.fill(1);

    auto begin = std::chrono::high_resolution_clock::now();
    auto stats = solver(op, x, y);
    auto end   = std::chrono::high_resolution_clock::now();

    std::cout << "u = " << x.topRows(nu).transpose() << std::endl;
    std::cout << "x = " << x.bottomRows(nx).transpose() << std::endl;
    std::cout << "y = " << y.transpose() << std::endl;
    std::cout << "f(x) = " << op.eval_f(x) << std::endl;
    auto gx = vec(op.get_m());
    op.eval_g(x, gx);
    std::cout << "g(x) = " << gx.transpose() << std::endl;
    std::cout << "Inner: " << stats.inner.iterations
              << ", Outer: " << stats.outer_iterations << std::endl;

    auto duration = std::chrono::duration<double>(end - begin);
    std::cout << duration.count() * 1e3 << "ms" << std::endl;

    auto u  = x.topRows(nu);
    auto xs = x.bottomRows(nx);
    auto ε  = 6e-4;

    vec u_ref(nu);
    u_ref << -1, -1, -1, -1, -1, -0.878565, -0.719274, -0.999999;
    EXPECT_THAT(u, EigenAlmostEqual(u_ref, ε));

    vec xs_ref(nx);
    xs_ref << 0.312975, -1.14893, -0.03111, 1.69392, -0.11316, 1.60049,
        0.177012, 1.14043;
    EXPECT_THAT(xs, EigenAlmostEqual(xs_ref, ε));

    vec y_ref(op.get_m());
    y_ref << 6.2595, -22.9787, -0.6222, 33.8783, -2.2632, 32.0098, 3.54023,
        22.8086;
    EXPECT_THAT(y, EigenAlmostEqual(y_ref, ε));
}

REGISTER_TYPED_TEST_SUITE_P(PANTR, multipleshooting8D);
struct NewtonTRHessVec {
    using Direction = alpaqa::NewtonTRDirection<alpaqa::DefaultConfig>;
    static constexpr bool finite_diff = false;
};

struct NewtonTRFiniteDiff {
    using Direction = alpaqa::NewtonTRDirection<alpaqa::DefaultConfig>;
    static constexpr bool finite_diff = true;
};

using PANTRDirectionTypes =
    ::testing::Types<NewtonTRHessVec, NewtonTRFiniteDiff>;
INSTANTIATE_TYPED_TEST_SUITE_P(ALM, PANTR, PANTRDirectionTypes);
