#pragma once

#include <alpaqa/config/config.hpp>
#include <alpaqa/export.h>

namespace alpaqa::params {

template <Config Conf>
struct ALPAQA_EXPORT vec_from_file {
    USING_ALPAQA_CONFIG(Conf);
    length_t expected_size;
    std::optional<vec> value = std::nullopt;
};

} // namespace alpaqa::params