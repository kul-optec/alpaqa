#pragma once

#include <alpaqa/config/config.hpp>
#include <alpaqa/functions/prox.hpp>
#include <alpaqa/problem/box.hpp>
#include <cassert>

namespace alpaqa::sets {

template <Config Conf>
typename Conf::real_t
guanaqo_tag_invoke(tag_t<alpaqa::prox>, Box<Conf> &self,
                   typename Conf::crmat in, typename Conf::rmat out,
                   [[maybe_unused]] typename Conf::real_t γ) {
    assert(in.rows() == out.rows());
    assert(in.cols() == out.cols());
    assert(in.size() == self.lower.size());
    assert(in.size() == self.upper.size());
    assert(!(self.lower.array() > self.upper.array()).any());
    out = in.reshaped()
              .cwiseMax(self.lower)
              .cwiseMin(self.upper)
              .reshaped(in.rows(), in.cols());
    return 0;
}

template <Config Conf>
typename Conf::real_t
guanaqo_tag_invoke(tag_t<alpaqa::prox_step>, Box<Conf> &self,
                   typename Conf::crmat in, typename Conf::crmat fwd_step,
                   typename Conf::rmat out, typename Conf::rmat fb_step,
                   [[maybe_unused]] typename Conf::real_t γ,
                   typename Conf::real_t γ_fwd) {
    assert(in.rows() == fwd_step.rows());
    assert(in.cols() == fwd_step.cols());
    assert(in.rows() == out.rows());
    assert(in.cols() == out.cols());
    assert(in.rows() == fb_step.rows());
    assert(in.cols() == fb_step.cols());
    assert(in.size() == self.lower.size());
    assert(in.size() == self.upper.size());
    assert(!(self.lower.array() > self.upper.array()).any());
    fb_step = (γ_fwd * fwd_step)
                  .reshaped()
                  .cwiseMax(self.lower - in.reshaped())
                  .cwiseMin(self.upper - in.reshaped())
                  .reshaped(in.rows(), in.cols());
    out = in + fb_step;
    return 0;
}

} // namespace alpaqa::sets
