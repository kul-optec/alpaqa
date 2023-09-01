#pragma once

#include <alpaqa/config/config.hpp>
#include <alpaqa/problem/box.hpp>
#include <alpaqa/proximal/prox.hpp>
#include <cassert>

namespace alpaqa {

template <Config Conf>
typename Conf::real_t
alpaqa_tag_invoke(tag_t<alpaqa::prox>, Box<Conf> &self, typename Conf::crmat in,
                  typename Conf::rmat out,
                  [[maybe_unused]] typename Conf::real_t γ) {
    assert(in.cols() == 1);
    assert(out.cols() == 1);
    assert(in.size() == out.size());
    assert(in.size() == self.lowerbound.size());
    assert(in.size() == self.upperbound.size());
    assert(!(self.lowerbound > self.upperbound).any());
    out = in.cwiseMax(self.lowerbound).cwiseMin(self.upperbound);
    return 0;
}

} // namespace alpaqa
