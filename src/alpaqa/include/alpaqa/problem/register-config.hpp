#pragma once

#include <alpaqa/config/config.hpp>
#include <alpaqa/problem/type-erased-problem.hpp>

#if _WIN32

#define DECLARE_ALPAQA_CONFIG(Conf)                                                                \
    template <>                                                                                    \
    struct alpaqa::is_config<Conf> : std::true_type {};
#define DEFINE_ALPAQA_CONFIG(Conf)                                                                 \
    template struct __declspec(dllexport) alpaqa::ProblemVTable<Conf>;                             \
    template <>                                                                                    \
    const typename Conf::vec alpaqa::null_vec<Conf>{};

#else // _WIN32

#define DECLARE_ALPAQA_CONFIG(Conf)                                                                \
    template <>                                                                                    \
    struct alpaqa::is_config<Conf> : std::true_type {};                                            \
    extern template struct __attribute__((visibility("default"))) alpaqa::ProblemVTable<Conf>;
#define DEFINE_ALPAQA_CONFIG(Conf)                                                                 \
    template struct alpaqa::ProblemVTable<Conf>;                                                   \
    template <>                                                                                    \
    const typename Conf::vec alpaqa::null_vec<Conf>{};

#endif // _WIN32

#define DECLARE_AND_DEFINE_ALPAQA_CONFIG(Conf)                                                     \
    DECLARE_ALPAQA_CONFIG(Conf)                                                                    \
    DEFINE_ALPAQA_CONFIG(Conf)
