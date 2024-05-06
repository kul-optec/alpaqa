#include <alpaqa/config/config.hpp>
#include <alpaqa/problem/problem-with-counters.hpp>
#include <alpaqa/problem/type-erased-problem.hpp>
#include <alpaqa/util/not-implemented.hpp>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace alpaqa {

template <Config Conf     = DefaultConfig,
          class Allocator = std::allocator<std::byte>>
class TestTypeErasedProblem
    : public util::TypeErased<ProblemVTable<Conf>, Allocator> {
  public:
    USING_ALPAQA_CONFIG(Conf);
    using VTable         = ProblemVTable<Conf>;
    using allocator_type = Allocator;
    using TypeErased     = util::TypeErased<VTable, allocator_type>;
    using TypeErased::TypeErased;

  public:
    using TypeErased::self;
    using TypeErased::vtable;

  public:
    template <class T, class... Args>
    static TestTypeErasedProblem make(Args &&...args) {
        return TypeErased::template make<TestTypeErasedProblem, T>(
            std::forward<Args>(args)...);
    }
};

static_assert(util::derived_from_TypeErased<TestTypeErasedProblem<>>);
static_assert(util::derived_from_TypeErased<TypeErasedProblem<>>);

} // namespace alpaqa

struct TestReqProblem {
    USING_ALPAQA_CONFIG(alpaqa::DefaultConfig);
    TestReqProblem()          = default;
    virtual ~TestReqProblem() = default;
    TestReqProblem(const TestReqProblem &) { throw std::logic_error("copy"); }
    TestReqProblem(TestReqProblem &&) { throw std::logic_error("move"); }

    // clang-format off
    MOCK_METHOD(void, eval_projecting_difference_constraints, (crvec g, rvec e), (const));
    MOCK_METHOD(void, eval_projection_multipliers, (rvec y, real_t M), (const));
    MOCK_METHOD(real_t, eval_proximal_gradient_step, (real_t γ, crvec x, crvec grad_ψ, rvec x̂, rvec p), (const));
    MOCK_METHOD(real_t, eval_objective, (crvec x), (const));
    MOCK_METHOD(void, eval_objective_gradient, (crvec x, rvec grad_fx), (const));
    MOCK_METHOD(void, eval_constraints, (crvec x, rvec gx), (const));
    MOCK_METHOD(void, eval_constraints_gradient_product, (crvec x, crvec y, rvec grad_gxy), (const));
    MOCK_METHOD(void, check, (), (const));
    // clang-format on

    length_t get_num_variables() const { return 0; }
    length_t get_num_constraints() const { return 0; }
};

TEST(TypeErasedProblem, RequiredProblem) {
    USING_ALPAQA_CONFIG(alpaqa::DefaultConfig);
    auto te_prob = alpaqa::TestTypeErasedProblem<>::make<TestReqProblem>();
    vec x;

    EXPECT_CALL(te_prob.as<TestReqProblem>(), eval_projecting_difference_constraints);
    te_prob.vtable.eval_projecting_difference_constraints(te_prob.self, x, x);
    testing::Mock::VerifyAndClearExpectations(&te_prob.as<TestReqProblem>());

    EXPECT_CALL(te_prob.as<TestReqProblem>(), eval_projection_multipliers);
    te_prob.vtable.eval_projection_multipliers(te_prob.self, x, 0);
    testing::Mock::VerifyAndClearExpectations(&te_prob.as<TestReqProblem>());

    EXPECT_CALL(te_prob.as<TestReqProblem>(), eval_proximal_gradient_step);
    te_prob.vtable.eval_proximal_gradient_step(te_prob.self, 0, x, x, x, x);
    testing::Mock::VerifyAndClearExpectations(&te_prob.as<TestReqProblem>());

    EXPECT_CALL(te_prob.as<TestReqProblem>(), eval_objective);
    te_prob.vtable.eval_objective(te_prob.self, x);
    testing::Mock::VerifyAndClearExpectations(&te_prob.as<TestReqProblem>());

    EXPECT_CALL(te_prob.as<TestReqProblem>(), eval_objective_gradient);
    te_prob.vtable.eval_objective_gradient(te_prob.self, x, x);
    testing::Mock::VerifyAndClearExpectations(&te_prob.as<TestReqProblem>());

    EXPECT_CALL(te_prob.as<TestReqProblem>(), eval_constraints);
    te_prob.vtable.eval_constraints(te_prob.self, x, x);
    testing::Mock::VerifyAndClearExpectations(&te_prob.as<TestReqProblem>());

    EXPECT_CALL(te_prob.as<TestReqProblem>(), eval_constraints_gradient_product);
    te_prob.vtable.eval_constraints_gradient_product(te_prob.self, x, x, x);
    testing::Mock::VerifyAndClearExpectations(&te_prob.as<TestReqProblem>());

    // No defaults for second-order functions
    EXPECT_EQ(te_prob.vtable.eval_grad_gi, te_prob.vtable.default_eval_grad_gi);
    EXPECT_EQ(te_prob.vtable.eval_lagrangian_hessian_product,
              te_prob.vtable.default_eval_lagrangian_hessian_product);
    EXPECT_EQ(te_prob.vtable.eval_lagrangian_hessian, te_prob.vtable.default_eval_lagrangian_hessian);

    // Defaults for combined evaluations
    EXPECT_EQ(te_prob.vtable.eval_objective_and_gradient,
              te_prob.vtable.default_eval_objective_and_gradient);
    EXPECT_EQ(te_prob.vtable.eval_objective_and_constraints, te_prob.vtable.default_eval_objective_and_constraints);
    EXPECT_EQ(te_prob.vtable.eval_objective_gradient_and_constraints_gradient_product,
              te_prob.vtable.default_eval_objective_gradient_and_constraints_gradient_product);

    // Defaults for Lagrangians
    EXPECT_EQ(te_prob.vtable.eval_lagrangian_gradient, te_prob.vtable.default_eval_lagrangian_gradient);
    EXPECT_EQ(te_prob.vtable.eval_augmented_lagrangian, te_prob.vtable.default_eval_augmented_lagrangian);
    EXPECT_EQ(te_prob.vtable.eval_augmented_lagrangian_gradient, te_prob.vtable.default_eval_augmented_lagrangian_gradient);
    EXPECT_EQ(te_prob.vtable.eval_augmented_lagrangian_and_gradient,
              te_prob.vtable.default_eval_augmented_lagrangian_and_gradient);
}

struct TestOptProblem : TestReqProblem {
    USING_ALPAQA_CONFIG(alpaqa::DefaultConfig);
    TestOptProblem() = default;
    static TestReqProblem fail(const auto &msg) { throw std::logic_error(msg); }
    TestOptProblem(const TestOptProblem &) : TestReqProblem{fail("copy")} {}
    TestOptProblem(TestOptProblem &&) : TestReqProblem{fail("move")} {}

    // clang-format off
    MOCK_METHOD(void, eval_grad_gi, (crvec x, index_t i, rvec grad_gi), (const));
    MOCK_METHOD(void, eval_lagrangian_hessian_product, (crvec x, crvec y, real_t scale, crvec v, rvec Hv), (const));
    MOCK_METHOD(void, eval_lagrangian_hessian, (crvec x, crvec y, real_t scale, rvec H_values), (const));
    MOCK_METHOD(real_t, eval_objective_and_gradient, (crvec x, rvec grad_fx), (const));
    MOCK_METHOD(real_t, eval_objective_and_constraints, (crvec x, rvec g), (const));
    MOCK_METHOD(void, eval_objective_gradient_and_constraints_gradient_product, (crvec x, crvec y, rvec grad_f, rvec grad_gxy), (const));
    MOCK_METHOD(void, eval_lagrangian_gradient, (crvec x, crvec y, rvec grad_L, rvec work_n), (const));
    MOCK_METHOD(real_t, eval_augmented_lagrangian, (crvec x, crvec y, crvec Σ, rvec ŷ), (const));
    MOCK_METHOD(void, eval_augmented_lagrangian_gradient, (crvec x, crvec y, crvec Σ, rvec grad_ψ, rvec work_n, rvec work_m), (const));
    MOCK_METHOD(real_t, eval_augmented_lagrangian_and_gradient, (crvec x, crvec y, crvec Σ, rvec grad_ψ, rvec work_n, rvec work_m), (const));
    // clang-format on
};

TEST(TypeErasedProblem, OptionalProblem) {
    USING_ALPAQA_CONFIG(alpaqa::DefaultConfig);
    auto te_prob = alpaqa::TestTypeErasedProblem<>::make<TestOptProblem>();
    vec x;
    indexvec i;

    ASSERT_NE(te_prob.vtable.eval_projecting_difference_constraints, nullptr);
    EXPECT_CALL(te_prob.as<TestOptProblem>(), eval_projecting_difference_constraints);
    te_prob.vtable.eval_projecting_difference_constraints(te_prob.self, x, x);
    testing::Mock::VerifyAndClearExpectations(&te_prob.as<TestOptProblem>());

    ASSERT_NE(te_prob.vtable.eval_projection_multipliers, nullptr);
    EXPECT_CALL(te_prob.as<TestOptProblem>(), eval_projection_multipliers);
    te_prob.vtable.eval_projection_multipliers(te_prob.self, x, 0);
    testing::Mock::VerifyAndClearExpectations(&te_prob.as<TestOptProblem>());

    ASSERT_NE(te_prob.vtable.eval_proximal_gradient_step, nullptr);
    EXPECT_CALL(te_prob.as<TestOptProblem>(), eval_proximal_gradient_step);
    te_prob.vtable.eval_proximal_gradient_step(te_prob.self, 0, x, x, x, x);
    testing::Mock::VerifyAndClearExpectations(&te_prob.as<TestOptProblem>());

    ASSERT_NE(te_prob.vtable.eval_objective, nullptr);
    EXPECT_CALL(te_prob.as<TestOptProblem>(), eval_objective);
    te_prob.vtable.eval_objective(te_prob.self, x);
    testing::Mock::VerifyAndClearExpectations(&te_prob.as<TestOptProblem>());

    ASSERT_NE(te_prob.vtable.eval_objective_gradient, nullptr);
    EXPECT_CALL(te_prob.as<TestOptProblem>(), eval_objective_gradient);
    te_prob.vtable.eval_objective_gradient(te_prob.self, x, x);
    testing::Mock::VerifyAndClearExpectations(&te_prob.as<TestOptProblem>());

    ASSERT_NE(te_prob.vtable.eval_constraints, nullptr);
    EXPECT_CALL(te_prob.as<TestOptProblem>(), eval_constraints);
    te_prob.vtable.eval_constraints(te_prob.self, x, x);
    testing::Mock::VerifyAndClearExpectations(&te_prob.as<TestOptProblem>());

    ASSERT_NE(te_prob.vtable.eval_constraints_gradient_product, nullptr);
    EXPECT_CALL(te_prob.as<TestOptProblem>(), eval_constraints_gradient_product);
    te_prob.vtable.eval_constraints_gradient_product(te_prob.self, x, x, x);
    testing::Mock::VerifyAndClearExpectations(&te_prob.as<TestOptProblem>());

    ASSERT_NE(te_prob.vtable.eval_grad_gi, nullptr);
    EXPECT_CALL(te_prob.as<TestOptProblem>(), eval_grad_gi);
    te_prob.vtable.eval_grad_gi(te_prob.self, x, 0, x, te_prob.vtable);
    testing::Mock::VerifyAndClearExpectations(&te_prob.as<TestOptProblem>());

    ASSERT_NE(te_prob.vtable.eval_lagrangian_hessian_product, nullptr);
    EXPECT_CALL(te_prob.as<TestOptProblem>(), eval_lagrangian_hessian_product);
    te_prob.vtable.eval_lagrangian_hessian_product(te_prob.self, x, x, 1, x, x,
                                    te_prob.vtable);
    testing::Mock::VerifyAndClearExpectations(&te_prob.as<TestOptProblem>());

    ASSERT_NE(te_prob.vtable.eval_lagrangian_hessian, nullptr);
    EXPECT_CALL(te_prob.as<TestOptProblem>(), eval_lagrangian_hessian);
    te_prob.vtable.eval_lagrangian_hessian(te_prob.self, x, x, 1, x, te_prob.vtable);
    testing::Mock::VerifyAndClearExpectations(&te_prob.as<TestOptProblem>());

    ASSERT_NE(te_prob.vtable.eval_objective_and_gradient, nullptr);
    EXPECT_CALL(te_prob.as<TestOptProblem>(), eval_objective_and_gradient);
    te_prob.vtable.eval_objective_and_gradient(te_prob.self, x, x, te_prob.vtable);
    testing::Mock::VerifyAndClearExpectations(&te_prob.as<TestOptProblem>());

    ASSERT_NE(te_prob.vtable.eval_objective_and_constraints, nullptr);
    EXPECT_CALL(te_prob.as<TestOptProblem>(), eval_objective_and_constraints);
    te_prob.vtable.eval_objective_and_constraints(te_prob.self, x, x, te_prob.vtable);
    testing::Mock::VerifyAndClearExpectations(&te_prob.as<TestOptProblem>());

    ASSERT_NE(te_prob.vtable.eval_objective_gradient_and_constraints_gradient_product, nullptr);
    EXPECT_CALL(te_prob.as<TestOptProblem>(), eval_objective_gradient_and_constraints_gradient_product);
    te_prob.vtable.eval_objective_gradient_and_constraints_gradient_product(te_prob.self, x, x, x, x,
                                           te_prob.vtable);
    testing::Mock::VerifyAndClearExpectations(&te_prob.as<TestOptProblem>());

    ASSERT_NE(te_prob.vtable.eval_lagrangian_gradient, nullptr);
    EXPECT_CALL(te_prob.as<TestOptProblem>(), eval_lagrangian_gradient);
    te_prob.vtable.eval_lagrangian_gradient(te_prob.self, x, x, x, x, te_prob.vtable);
    testing::Mock::VerifyAndClearExpectations(&te_prob.as<TestOptProblem>());

    ASSERT_NE(te_prob.vtable.eval_augmented_lagrangian, nullptr);
    EXPECT_CALL(te_prob.as<TestOptProblem>(), eval_augmented_lagrangian);
    te_prob.vtable.eval_augmented_lagrangian(te_prob.self, x, x, x, x, te_prob.vtable);
    testing::Mock::VerifyAndClearExpectations(&te_prob.as<TestOptProblem>());

    ASSERT_NE(te_prob.vtable.eval_augmented_lagrangian_gradient, nullptr);
    EXPECT_CALL(te_prob.as<TestOptProblem>(), eval_augmented_lagrangian_gradient);
    te_prob.vtable.eval_augmented_lagrangian_gradient(te_prob.self, x, x, x, x, x, x, te_prob.vtable);
    testing::Mock::VerifyAndClearExpectations(&te_prob.as<TestOptProblem>());

    ASSERT_NE(te_prob.vtable.eval_augmented_lagrangian_and_gradient, nullptr);
    EXPECT_CALL(te_prob.as<TestOptProblem>(), eval_augmented_lagrangian_and_gradient);
    te_prob.vtable.eval_augmented_lagrangian_and_gradient(te_prob.self, x, x, x, x, x, x,
                                 te_prob.vtable);
    testing::Mock::VerifyAndClearExpectations(&te_prob.as<TestOptProblem>());
}

TEST(TypeErasedProblem, OptionalProblemPtr) {
    USING_ALPAQA_CONFIG(alpaqa::DefaultConfig);
    TestOptProblem prob;
    alpaqa::TestTypeErasedProblem<> te_prob{&prob};
    vec x;
    indexvec i;

    ASSERT_NE(te_prob.vtable.eval_projecting_difference_constraints, nullptr);
    EXPECT_CALL(prob, eval_projecting_difference_constraints);
    te_prob.vtable.eval_projecting_difference_constraints(te_prob.self, x, x);
    testing::Mock::VerifyAndClearExpectations(&prob);

    ASSERT_NE(te_prob.vtable.eval_projection_multipliers, nullptr);
    EXPECT_CALL(prob, eval_projection_multipliers);
    te_prob.vtable.eval_projection_multipliers(te_prob.self, x, 0);
    testing::Mock::VerifyAndClearExpectations(&prob);

    ASSERT_NE(te_prob.vtable.eval_proximal_gradient_step, nullptr);
    EXPECT_CALL(prob, eval_proximal_gradient_step);
    te_prob.vtable.eval_proximal_gradient_step(te_prob.self, 0, x, x, x, x);
    testing::Mock::VerifyAndClearExpectations(&prob);

    ASSERT_NE(te_prob.vtable.eval_objective, nullptr);
    EXPECT_CALL(prob, eval_objective);
    te_prob.vtable.eval_objective(te_prob.self, x);
    testing::Mock::VerifyAndClearExpectations(&prob);

    ASSERT_NE(te_prob.vtable.eval_objective_gradient, nullptr);
    EXPECT_CALL(prob, eval_objective_gradient);
    te_prob.vtable.eval_objective_gradient(te_prob.self, x, x);
    testing::Mock::VerifyAndClearExpectations(&prob);

    ASSERT_NE(te_prob.vtable.eval_constraints, nullptr);
    EXPECT_CALL(prob, eval_constraints);
    te_prob.vtable.eval_constraints(te_prob.self, x, x);
    testing::Mock::VerifyAndClearExpectations(&prob);

    ASSERT_NE(te_prob.vtable.eval_constraints_gradient_product, nullptr);
    EXPECT_CALL(prob, eval_constraints_gradient_product);
    te_prob.vtable.eval_constraints_gradient_product(te_prob.self, x, x, x);
    testing::Mock::VerifyAndClearExpectations(&prob);

    ASSERT_NE(te_prob.vtable.eval_grad_gi, nullptr);
    EXPECT_CALL(prob, eval_grad_gi);
    te_prob.vtable.eval_grad_gi(te_prob.self, x, 0, x, te_prob.vtable);
    testing::Mock::VerifyAndClearExpectations(&prob);

    ASSERT_NE(te_prob.vtable.eval_lagrangian_hessian_product, nullptr);
    EXPECT_CALL(prob, eval_lagrangian_hessian_product);
    te_prob.vtable.eval_lagrangian_hessian_product(te_prob.self, x, x, 1, x, x,
                                    te_prob.vtable);
    testing::Mock::VerifyAndClearExpectations(&prob);

    ASSERT_NE(te_prob.vtable.eval_lagrangian_hessian, nullptr);
    EXPECT_CALL(prob, eval_lagrangian_hessian);
    te_prob.vtable.eval_lagrangian_hessian(te_prob.self, x, x, 1, x, te_prob.vtable);
    testing::Mock::VerifyAndClearExpectations(&prob);

    ASSERT_NE(te_prob.vtable.eval_objective_and_gradient, nullptr);
    EXPECT_CALL(prob, eval_objective_and_gradient);
    te_prob.vtable.eval_objective_and_gradient(te_prob.self, x, x, te_prob.vtable);
    testing::Mock::VerifyAndClearExpectations(&prob);

    ASSERT_NE(te_prob.vtable.eval_objective_and_constraints, nullptr);
    EXPECT_CALL(prob, eval_objective_and_constraints);
    te_prob.vtable.eval_objective_and_constraints(te_prob.self, x, x, te_prob.vtable);
    testing::Mock::VerifyAndClearExpectations(&prob);

    ASSERT_NE(te_prob.vtable.eval_objective_gradient_and_constraints_gradient_product, nullptr);
    EXPECT_CALL(prob, eval_objective_gradient_and_constraints_gradient_product);
    te_prob.vtable.eval_objective_gradient_and_constraints_gradient_product(te_prob.self, x, x, x, x,
                                           te_prob.vtable);
    testing::Mock::VerifyAndClearExpectations(&prob);

    ASSERT_NE(te_prob.vtable.eval_lagrangian_gradient, nullptr);
    EXPECT_CALL(prob, eval_lagrangian_gradient);
    te_prob.vtable.eval_lagrangian_gradient(te_prob.self, x, x, x, x, te_prob.vtable);
    testing::Mock::VerifyAndClearExpectations(&prob);

    ASSERT_NE(te_prob.vtable.eval_augmented_lagrangian, nullptr);
    EXPECT_CALL(prob, eval_augmented_lagrangian);
    te_prob.vtable.eval_augmented_lagrangian(te_prob.self, x, x, x, x, te_prob.vtable);
    testing::Mock::VerifyAndClearExpectations(&prob);

    ASSERT_NE(te_prob.vtable.eval_augmented_lagrangian_gradient, nullptr);
    EXPECT_CALL(prob, eval_augmented_lagrangian_gradient);
    te_prob.vtable.eval_augmented_lagrangian_gradient(te_prob.self, x, x, x, x, x, x, te_prob.vtable);
    testing::Mock::VerifyAndClearExpectations(&prob);

    ASSERT_NE(te_prob.vtable.eval_augmented_lagrangian_and_gradient, nullptr);
    EXPECT_CALL(prob, eval_augmented_lagrangian_and_gradient);
    te_prob.vtable.eval_augmented_lagrangian_and_gradient(te_prob.self, x, x, x, x, x, x,
                                 te_prob.vtable);
    testing::Mock::VerifyAndClearExpectations(&prob);
}

TEST(TypeErasedProblem, CountedOptionalProblem) {
    USING_ALPAQA_CONFIG(alpaqa::DefaultConfig);
    using Problem = alpaqa::ProblemWithCounters<TestOptProblem &>;
    TestOptProblem prob;
    auto te_prob = alpaqa::TestTypeErasedProblem<>::make<Problem>(prob);
    auto &evals  = *te_prob.as<Problem>().evaluations;
    vec x(1);
    indexvec i;

    EXPECT_EQ(evals.proj_diff_g, 0);
    ASSERT_NE(te_prob.vtable.eval_projecting_difference_constraints, nullptr);
    EXPECT_CALL(prob, eval_projecting_difference_constraints);
    te_prob.vtable.eval_projecting_difference_constraints(te_prob.self, x, x);
    testing::Mock::VerifyAndClearExpectations(&prob);
    EXPECT_EQ(evals.proj_diff_g, 1);

    EXPECT_EQ(evals.proj_multipliers, 0);
    ASSERT_NE(te_prob.vtable.eval_projection_multipliers, nullptr);
    EXPECT_CALL(prob, eval_projection_multipliers);
    te_prob.vtable.eval_projection_multipliers(te_prob.self, x, 0);
    testing::Mock::VerifyAndClearExpectations(&prob);
    EXPECT_EQ(evals.proj_multipliers, 1);

    EXPECT_EQ(evals.prox_grad_step, 0);
    ASSERT_NE(te_prob.vtable.eval_proximal_gradient_step, nullptr);
    EXPECT_CALL(prob, eval_proximal_gradient_step);
    te_prob.vtable.eval_proximal_gradient_step(te_prob.self, 0, x, x, x, x);
    testing::Mock::VerifyAndClearExpectations(&prob);
    EXPECT_EQ(evals.prox_grad_step, 1);

    EXPECT_EQ(evals.f, 0);
    ASSERT_NE(te_prob.vtable.eval_objective, nullptr);
    EXPECT_CALL(prob, eval_objective);
    te_prob.vtable.eval_objective(te_prob.self, x);
    testing::Mock::VerifyAndClearExpectations(&prob);
    EXPECT_EQ(evals.f, 1);

    EXPECT_EQ(evals.grad_f, 0);
    ASSERT_NE(te_prob.vtable.eval_objective_gradient, nullptr);
    EXPECT_CALL(prob, eval_objective_gradient);
    te_prob.vtable.eval_objective_gradient(te_prob.self, x, x);
    testing::Mock::VerifyAndClearExpectations(&prob);
    EXPECT_EQ(evals.grad_f, 1);

    EXPECT_EQ(evals.g, 0);
    ASSERT_NE(te_prob.vtable.eval_constraints, nullptr);
    EXPECT_CALL(prob, eval_constraints);
    te_prob.vtable.eval_constraints(te_prob.self, x, x);
    testing::Mock::VerifyAndClearExpectations(&prob);
    EXPECT_EQ(evals.g, 1);

    EXPECT_EQ(evals.grad_g_prod, 0);
    ASSERT_NE(te_prob.vtable.eval_constraints_gradient_product, nullptr);
    EXPECT_CALL(prob, eval_constraints_gradient_product);
    te_prob.vtable.eval_constraints_gradient_product(te_prob.self, x, x, x);
    testing::Mock::VerifyAndClearExpectations(&prob);
    EXPECT_EQ(evals.grad_g_prod, 1);

    EXPECT_EQ(evals.grad_gi, 0);
    ASSERT_NE(te_prob.vtable.eval_grad_gi, nullptr);
    EXPECT_CALL(prob, eval_grad_gi);
    te_prob.vtable.eval_grad_gi(te_prob.self, x, 0, x, te_prob.vtable);
    testing::Mock::VerifyAndClearExpectations(&prob);
    EXPECT_EQ(evals.grad_gi, 1);

    EXPECT_EQ(evals.hess_L_prod, 0);
    ASSERT_NE(te_prob.vtable.eval_lagrangian_hessian_product, nullptr);
    EXPECT_CALL(prob, eval_lagrangian_hessian_product);
    te_prob.vtable.eval_lagrangian_hessian_product(te_prob.self, x, x, 1, x, x,
                                    te_prob.vtable);
    testing::Mock::VerifyAndClearExpectations(&prob);
    EXPECT_EQ(evals.hess_L_prod, 1);

    EXPECT_EQ(evals.hess_L, 0);
    ASSERT_NE(te_prob.vtable.eval_lagrangian_hessian, nullptr);
    EXPECT_CALL(prob, eval_lagrangian_hessian);
    te_prob.vtable.eval_lagrangian_hessian(te_prob.self, x, x, 1, x, te_prob.vtable);
    testing::Mock::VerifyAndClearExpectations(&prob);
    EXPECT_EQ(evals.hess_L, 1);

    EXPECT_EQ(evals.f_grad_f, 0);
    ASSERT_NE(te_prob.vtable.eval_objective_and_gradient, nullptr);
    EXPECT_CALL(prob, eval_objective_and_gradient);
    te_prob.vtable.eval_objective_and_gradient(te_prob.self, x, x, te_prob.vtable);
    testing::Mock::VerifyAndClearExpectations(&prob);
    EXPECT_EQ(evals.f_grad_f, 1);

    EXPECT_EQ(evals.f_g, 0);
    ASSERT_NE(te_prob.vtable.eval_objective_and_constraints, nullptr);
    EXPECT_CALL(prob, eval_objective_and_constraints);
    te_prob.vtable.eval_objective_and_constraints(te_prob.self, x, x, te_prob.vtable);
    testing::Mock::VerifyAndClearExpectations(&prob);
    EXPECT_EQ(evals.f_g, 1);

    EXPECT_EQ(evals.grad_f_grad_g_prod, 0);
    ASSERT_NE(te_prob.vtable.eval_objective_gradient_and_constraints_gradient_product, nullptr);
    EXPECT_CALL(prob, eval_objective_gradient_and_constraints_gradient_product);
    te_prob.vtable.eval_objective_gradient_and_constraints_gradient_product(te_prob.self, x, x, x, x,
                                           te_prob.vtable);
    testing::Mock::VerifyAndClearExpectations(&prob);
    EXPECT_EQ(evals.grad_f_grad_g_prod, 1);

    EXPECT_EQ(evals.grad_L, 0);
    ASSERT_NE(te_prob.vtable.eval_lagrangian_gradient, nullptr);
    EXPECT_CALL(prob, eval_lagrangian_gradient);
    te_prob.vtable.eval_lagrangian_gradient(te_prob.self, x, x, x, x, te_prob.vtable);
    testing::Mock::VerifyAndClearExpectations(&prob);
    EXPECT_EQ(evals.grad_L, 1);

    EXPECT_EQ(evals.ψ, 0);
    ASSERT_NE(te_prob.vtable.eval_augmented_lagrangian, nullptr);
    EXPECT_CALL(prob, eval_augmented_lagrangian);
    te_prob.vtable.eval_augmented_lagrangian(te_prob.self, x, x, x, x, te_prob.vtable);
    testing::Mock::VerifyAndClearExpectations(&prob);
    EXPECT_EQ(evals.ψ, 1);

    EXPECT_EQ(evals.grad_ψ, 0);
    ASSERT_NE(te_prob.vtable.eval_augmented_lagrangian_gradient, nullptr);
    EXPECT_CALL(prob, eval_augmented_lagrangian_gradient);
    te_prob.vtable.eval_augmented_lagrangian_gradient(te_prob.self, x, x, x, x, x, x, te_prob.vtable);
    testing::Mock::VerifyAndClearExpectations(&prob);
    EXPECT_EQ(evals.grad_ψ, 1);

    EXPECT_EQ(evals.ψ_grad_ψ, 0);
    ASSERT_NE(te_prob.vtable.eval_augmented_lagrangian_and_gradient, nullptr);
    EXPECT_CALL(prob, eval_augmented_lagrangian_and_gradient);
    te_prob.vtable.eval_augmented_lagrangian_and_gradient(te_prob.self, x, x, x, x, x, x,
                                 te_prob.vtable);
    testing::Mock::VerifyAndClearExpectations(&prob);
    EXPECT_EQ(evals.ψ_grad_ψ, 1);
}

struct TestOptProblemNoHess : TestOptProblem {
    USING_ALPAQA_CONFIG(alpaqa::DefaultConfig);
    TestOptProblemNoHess()                             = default;
    TestOptProblemNoHess(const TestOptProblemNoHess &) = default;
    TestOptProblemNoHess(TestOptProblemNoHess &&)      = default;

    bool provides_eval_grad_gi() { return true; }
    bool provides_eval_lagrangian_hessian_product() { return false; }
    bool provides_eval_lagrangian_hessian() { return false; }
};

TEST(TypeErasedProblem, providesNoHess) {
    USING_ALPAQA_CONFIG(alpaqa::DefaultConfig);
    auto te_prob =
        alpaqa::TestTypeErasedProblem<>::make<TestOptProblemNoHess>();

    EXPECT_NE(te_prob.vtable.eval_grad_gi, te_prob.vtable.default_eval_grad_gi);
    EXPECT_EQ(te_prob.vtable.eval_lagrangian_hessian_product,
              te_prob.vtable.default_eval_lagrangian_hessian_product);
    EXPECT_EQ(te_prob.vtable.eval_lagrangian_hessian, te_prob.vtable.default_eval_lagrangian_hessian);
}

struct TestOptProblemNoPsi : TestOptProblem {
    USING_ALPAQA_CONFIG(alpaqa::DefaultConfig);
    TestOptProblemNoPsi()                            = default;
    virtual ~TestOptProblemNoPsi()                   = default;
    TestOptProblemNoPsi(const TestOptProblemNoPsi &) = default;
    TestOptProblemNoPsi(TestOptProblemNoPsi &&)      = default;

    bool provides_eval_lagrangian_gradient() { return true; }
    bool provides_eval_augmented_lagrangian() { return false; }
    bool provides_eval_augmented_lagrangian_and_gradient() { return false; }
};

TEST(TypeErasedProblem, providesNoPsi) {
    USING_ALPAQA_CONFIG(alpaqa::DefaultConfig);
    auto te_prob = alpaqa::TestTypeErasedProblem<>::make<TestOptProblemNoPsi>();

    EXPECT_NE(te_prob.vtable.eval_lagrangian_gradient, te_prob.vtable.default_eval_lagrangian_gradient);
    EXPECT_EQ(te_prob.vtable.eval_augmented_lagrangian, te_prob.vtable.default_eval_augmented_lagrangian);
    EXPECT_EQ(te_prob.vtable.eval_augmented_lagrangian_and_gradient,
              te_prob.vtable.default_eval_augmented_lagrangian_and_gradient);
}

TEST(TypeErasedProblem, TEOptionalProblem) {
    USING_ALPAQA_CONFIG(alpaqa::DefaultConfig);
    auto te_prob = alpaqa::TypeErasedProblem<>::make<TestOptProblem>();
    vec x;
    indexvec i;

    EXPECT_CALL(te_prob.as<TestOptProblem>(), eval_projecting_difference_constraints);
    te_prob.eval_projecting_difference_constraints(x, x);
    testing::Mock::VerifyAndClearExpectations(&te_prob.as<TestOptProblem>());

    EXPECT_CALL(te_prob.as<TestOptProblem>(), eval_projection_multipliers);
    te_prob.eval_projection_multipliers(x, 0);
    testing::Mock::VerifyAndClearExpectations(&te_prob.as<TestOptProblem>());

    EXPECT_CALL(te_prob.as<TestOptProblem>(), eval_proximal_gradient_step);
    te_prob.eval_proximal_gradient_step(0, x, x, x, x);
    testing::Mock::VerifyAndClearExpectations(&te_prob.as<TestOptProblem>());

    EXPECT_CALL(te_prob.as<TestOptProblem>(), eval_objective);
    (void)te_prob.eval_objective(x);
    testing::Mock::VerifyAndClearExpectations(&te_prob.as<TestOptProblem>());

    EXPECT_CALL(te_prob.as<TestOptProblem>(), eval_objective_gradient);
    te_prob.eval_objective_gradient(x, x);
    testing::Mock::VerifyAndClearExpectations(&te_prob.as<TestOptProblem>());

    EXPECT_CALL(te_prob.as<TestOptProblem>(), eval_constraints);
    te_prob.eval_constraints(x, x);
    testing::Mock::VerifyAndClearExpectations(&te_prob.as<TestOptProblem>());

    EXPECT_CALL(te_prob.as<TestOptProblem>(), eval_constraints_gradient_product);
    te_prob.eval_constraints_gradient_product(x, x, x);
    testing::Mock::VerifyAndClearExpectations(&te_prob.as<TestOptProblem>());

    EXPECT_CALL(te_prob.as<TestOptProblem>(), eval_grad_gi);
    te_prob.eval_grad_gi(x, 0, x);
    testing::Mock::VerifyAndClearExpectations(&te_prob.as<TestOptProblem>());

    EXPECT_CALL(te_prob.as<TestOptProblem>(), eval_lagrangian_hessian_product);
    te_prob.eval_lagrangian_hessian_product(x, x, 1, x, x);
    testing::Mock::VerifyAndClearExpectations(&te_prob.as<TestOptProblem>());

    EXPECT_CALL(te_prob.as<TestOptProblem>(), eval_lagrangian_hessian);
    te_prob.eval_lagrangian_hessian(x, x, 1, x);
    testing::Mock::VerifyAndClearExpectations(&te_prob.as<TestOptProblem>());

    EXPECT_CALL(te_prob.as<TestOptProblem>(), eval_objective_and_gradient);
    (void)te_prob.eval_objective_and_gradient(x, x);
    testing::Mock::VerifyAndClearExpectations(&te_prob.as<TestOptProblem>());

    EXPECT_CALL(te_prob.as<TestOptProblem>(), eval_objective_and_constraints);
    (void)te_prob.eval_objective_and_constraints(x, x);
    testing::Mock::VerifyAndClearExpectations(&te_prob.as<TestOptProblem>());

    EXPECT_CALL(te_prob.as<TestOptProblem>(), eval_objective_gradient_and_constraints_gradient_product);
    te_prob.eval_objective_gradient_and_constraints_gradient_product(x, x, x, x);
    testing::Mock::VerifyAndClearExpectations(&te_prob.as<TestOptProblem>());

    EXPECT_CALL(te_prob.as<TestOptProblem>(), eval_lagrangian_gradient);
    te_prob.eval_lagrangian_gradient(x, x, x, x);
    testing::Mock::VerifyAndClearExpectations(&te_prob.as<TestOptProblem>());

    EXPECT_CALL(te_prob.as<TestOptProblem>(), eval_augmented_lagrangian);
    (void)te_prob.eval_augmented_lagrangian(x, x, x, x);
    testing::Mock::VerifyAndClearExpectations(&te_prob.as<TestOptProblem>());

    EXPECT_CALL(te_prob.as<TestOptProblem>(), eval_augmented_lagrangian_gradient);
    te_prob.eval_augmented_lagrangian_gradient(x, x, x, x, x, x);
    testing::Mock::VerifyAndClearExpectations(&te_prob.as<TestOptProblem>());

    EXPECT_CALL(te_prob.as<TestOptProblem>(), eval_augmented_lagrangian_and_gradient);
    (void)te_prob.eval_augmented_lagrangian_and_gradient(x, x, x, x, x, x);
    testing::Mock::VerifyAndClearExpectations(&te_prob.as<TestOptProblem>());
}

TEST(TypeErasedProblem, TEprovidesNoHess) {
    USING_ALPAQA_CONFIG(alpaqa::DefaultConfig);
    auto te_prob = alpaqa::TypeErasedProblem<>::make<TestOptProblemNoHess>();
    vec x;
    indexvec i;

    EXPECT_TRUE(te_prob.provides_eval_grad_gi());
    EXPECT_FALSE(te_prob.provides_eval_lagrangian_hessian_product());
    EXPECT_FALSE(te_prob.provides_eval_lagrangian_hessian());

    EXPECT_CALL(te_prob.as<TestOptProblemNoHess>(), eval_grad_gi);
    te_prob.eval_grad_gi(x, 0, x);
    testing::Mock::VerifyAndClearExpectations(
        &te_prob.as<TestOptProblemNoHess>());

    EXPECT_THROW(te_prob.eval_lagrangian_hessian_product(x, x, 1, x, x),
                 alpaqa::not_implemented_error);

    EXPECT_THROW(te_prob.eval_lagrangian_hessian(x, x, 1, x),
                 alpaqa::not_implemented_error);
}
