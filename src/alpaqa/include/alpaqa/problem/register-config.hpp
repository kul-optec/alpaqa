#pragma once

#include <alpaqa/config/config.hpp>
#include <alpaqa/problem/type-erased-problem.hpp>

#define DECLARE_ALPAQA_CONFIG(Conf)                                                                \
    template <>                                                                                    \
    struct alpaqa::is_config<Conf> : std::true_type {};
#if _WIN32
#define DEFINE_ALPAQA_CONFIG(Conf)                                                                 \
    template struct __declspec(dllexport) alpaqa::ProblemVTable<Conf>;                             \
    template <>                                                                                    \
    const typename Conf::vec alpaqa::null_vec<Conf>{};
#else
#define DEFINE_ALPAQA_CONFIG(Conf)                                                                 \
    template struct [[gnu::visibility("default")]] alpaqa::ProblemVTable<Conf>;                    \
    template <>                                                                                    \
    const typename Conf::vec alpaqa::null_vec<Conf>{};
#endif
#define DECLARE_AND_DEFINE_ALPAQA_CONFIG(Conf)                                                     \
    DECLARE_ALPAQA_CONFIG(Conf)                                                                    \
    DEFINE_ALPAQA_CONFIG(Conf)
