#pragma once

#include <alpaqa/config/config.hpp>
#include <alpaqa/util/tag-invoke.hpp>

namespace alpaqa {

struct prox_fn {
    template <class T>
        requires requires {
            typename T::config_t;
            requires is_config_v<typename T::config_t>;
            requires alpaqa::tag_invocable<
                prox_fn, T &, typename T::config_t::crmat,
                typename T::config_t::rmat, typename T::config_t::real_t>;
            requires std::is_same_v<
                tag_invoke_result_t<prox_fn, T &, typename T::config_t::crmat,
                                    typename T::config_t::rmat,
                                    typename T::config_t::real_t>,
                typename T::config_t::real_t>;
        }
    auto operator()(T &proximable_func, T::config_t::crmat in,
                    T::config_t::rmat out, T::config_t::real_t γ = 1) const
        noexcept(alpaqa::is_nothrow_tag_invocable_v<
                 prox_fn, T &, typename T::config_t::crmat,
                 typename T::config_t::rmat, typename T::config_t::real_t>)
            -> T::config_t::real_t {
        return alpaqa::alpaqa_tag_invoke(*this, proximable_func, std::move(in),
                                         std::move(out), γ);
    }
} inline constexpr prox;

} // namespace alpaqa
