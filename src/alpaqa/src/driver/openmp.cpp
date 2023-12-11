#ifdef _OPENMP

#if defined(__GNUC__)
#define ALPAQA_USED [[gnu::used]]
#elif defined(_MSC_VER)
#define ALPAQA_USED __declspec(dllexport)
#else
#define ALPAQA_USED
#endif

namespace alpaqa::detail {
ALPAQA_USED void openmp_dummy() {
#pragma omp parallel
    {
        [[maybe_unused]] const char *volatile msg =
            "Force linking OpenMP, even if not used inside of alpaqa itself";
    }
}
} // namespace alpaqa::detail

#endif