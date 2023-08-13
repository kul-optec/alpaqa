#pragma once

#ifdef ALPAQA_WITH_QUAD_PRECISION

#include <alpaqa/export.h>
#include <iosfwd>

namespace alpaqa {
ALPAQA_EXPORT
std::ostream &operator<<(std::ostream &os, __float128 f);
} // namespace alpaqa
using alpaqa::operator<<;

#endif