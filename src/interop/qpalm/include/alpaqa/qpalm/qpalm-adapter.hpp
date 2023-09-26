#include <alpaqa/config/config.hpp>
#include <alpaqa/problem/type-erased-problem.hpp>
#include <alpaqa/qpalm-adapter-export.h>
#include <qpalm.hpp>

namespace alpaqa {

struct QPALM_ADAPTER_EXPORT OwningQPALMData : QPALMData {
    USING_ALPAQA_CONFIG(EigenConfigd);
    struct Storage {
        qpalm::ladel_sparse_matrix_ptr Q, A;
        vec q;
        Box<config_t> b;
    };
    std::unique_ptr<Storage> sto = std::make_unique<Storage>();
};

QPALM_ADAPTER_EXPORT OwningQPALMData
build_qpalm_problem(const TypeErasedProblem<EigenConfigd> &problem);

} // namespace alpaqa