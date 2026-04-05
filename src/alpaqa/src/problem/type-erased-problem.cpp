#include <alpaqa/problem/type-erased-problem.hpp>

namespace alpaqa {

ALPAQA_EXPORT_TEMPLATE(struct, ProblemVTable, EigenConfigd);
ALPAQA_IF_FLOAT(ALPAQA_EXPORT_TEMPLATE(struct, ProblemVTable, EigenConfigf);)
ALPAQA_IF_LONGD(ALPAQA_EXPORT_TEMPLATE(struct, ProblemVTable, EigenConfigl);)
ALPAQA_IF_QUADF(ALPAQA_EXPORT_TEMPLATE(struct, ProblemVTable, EigenConfigq);)

} // namespace alpaqa
