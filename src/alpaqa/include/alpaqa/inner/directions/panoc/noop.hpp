#pragma once

#include <alpaqa/inner/directions/panoc-direction-update.hpp>
#include <alpaqa/problem/type-erased-problem.hpp>

namespace alpaqa {

/// Direction provider that provides no directions (apply always returns false).
/// @ingroup grp_DirectionProviders
template <Config Conf>
struct NoopDirection {
    USING_ALPAQA_CONFIG(Conf);

    using Problem           = TypeErasedProblem<config_t>;
    using AcceleratorParams = void;
    using DirectionParams   = void;

    NoopDirection() = default;

    /// @see @ref PANOCDirection::initialize
    void initialize([[maybe_unused]] const Problem &problem,
                    [[maybe_unused]] crvec y, [[maybe_unused]] crvec Σ,
                    [[maybe_unused]] real_t γ_0, [[maybe_unused]] crvec x_0,
                    [[maybe_unused]] crvec x̂_0, [[maybe_unused]] crvec p_0,
                    [[maybe_unused]] crvec grad_ψx_0) {}

    /// @see @ref PANOCDirection::has_initial_direction
    [[nodiscard]] bool has_initial_direction() const { return false; }

    /// @see @ref PANOCDirection::update
    [[nodiscard]] bool
    update([[maybe_unused]] real_t γₖ, [[maybe_unused]] real_t γₙₑₓₜ,
           [[maybe_unused]] crvec xₖ, [[maybe_unused]] crvec xₙₑₓₜ,
           [[maybe_unused]] crvec pₖ, [[maybe_unused]] crvec pₙₑₓₜ,
           [[maybe_unused]] crvec grad_ψxₖ,
           [[maybe_unused]] crvec grad_ψxₙₑₓₜ) {
        return true;
    }

    /// @see @ref PANOCDirection::apply
    [[nodiscard]] bool
    apply([[maybe_unused]] real_t γₖ, [[maybe_unused]] crvec xₖ,
          [[maybe_unused]] crvec x̂ₖ, [[maybe_unused]] crvec pₖ,
          [[maybe_unused]] crvec grad_ψxₖ, [[maybe_unused]] rvec qₖ) const {
        return false;
    }

    /// @see @ref PANOCDirection::changed_γ
    void changed_γ([[maybe_unused]] real_t γₖ, [[maybe_unused]] real_t old_γₖ) {
    }

    /// @see @ref PANOCDirection::reset
    void reset() {}

    /// @see @ref PANOCDirection::get_name
    [[nodiscard]] std::string get_name() const {
        return "NoopDirection<" + std::string(config_t::get_name()) + '>';
    }
    void get_params() const {}
};

} // namespace alpaqa