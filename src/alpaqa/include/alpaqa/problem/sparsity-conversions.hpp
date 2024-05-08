#pragma once

#include <alpaqa/config/config.hpp>
#include <alpaqa/problem/sparsity.hpp>
#include <alpaqa/util/span.hpp>
#include <guanaqo/linalg/sparsity-conversions.hpp>
#include <utility>

namespace alpaqa::sparsity {

template <Config Conf, class To>
struct ConvertingEvaluator {
    USING_ALPAQA_CONFIG(Conf);
    ConvertingEvaluator(const Sparsity &sp)
        : converter{sp}, values(get_nnz(sp)), work(converter.work_size()) {}
    SparsityConverter<Sparsity, To> converter;
    vec values, work;

    template <class E>
    rvec eval(E &&evaluator) {
        std::forward<E>(evaluator)(values);
        auto r = converter.template convert_values<real_t>(as_span(values), as_span(work));
        return as_vec(r);
    }
};

template <Config Conf>
struct DenseEvaluator {
    USING_ALPAQA_CONFIG(Conf);
    DenseEvaluator(const Sparsity &sp) : converter{sp} {}
    ConvertingEvaluator<Conf, Dense> converter;

    template <class E>
    rmat eval(E &&evaluator) {
        const auto &sp = converter.converter.get_sparsity();
        auto v         = converter.eval(std::forward<E>(evaluator));
        return mmat{v.data(), num_rows(sp), num_cols(sp)};
    }
};

} // namespace alpaqa::sparsity
