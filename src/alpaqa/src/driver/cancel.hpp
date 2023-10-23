#pragma once

#include <atomic>
#include <csignal>
#include <memory>
#include <stdexcept>

namespace alpaqa {

namespace detail {
inline std::atomic<void *> solver_to_stop;
}

/**
 * Attach SIGINT and SIGTERM handlers to stop the given solver.
 * @param   solver
 *          The solver that should be stopped by the handler.
 * @return  A RAII object that detaches the handler when destroyed.
 */
template <class Solver>
[[nodiscard]] auto attach_cancellation(Solver &solver) {
    using detail::solver_to_stop;
    using solver_to_stop_t = decltype(solver_to_stop);
    if constexpr (requires { solver.stop(); }) {
        auto *old = solver_to_stop.exchange(&solver, std::memory_order_release);
        if (old) {
            throw std::logic_error(
                "alpaqa-driver:attach_cancellation can only be used once");
        }
        auto handler = +[](int) {
            if (auto *s = solver_to_stop.load(std::memory_order::acquire))
                reinterpret_cast<Solver *>(s)->stop();
        };
#ifdef _WIN32
        signal(SIGINT, handler);
        signal(SIGTERM, handler);
#else
        struct sigaction action;
        action.sa_handler = handler;
        sigemptyset(&action.sa_mask);
        action.sa_flags = 0;
        sigaction(SIGINT, &action, nullptr);
        sigaction(SIGTERM, &action, nullptr);
#endif
        auto detach_solver = +[](solver_to_stop_t *p) {
#ifdef _WIN32
            signal(SIGINT, SIG_DFL);
            signal(SIGTERM, SIG_DFL);
#else
            struct sigaction action;
            action.sa_handler = SIG_DFL;
            sigemptyset(&action.sa_mask);
            action.sa_flags = 0;
            sigaction(SIGINT, &action, nullptr);
            sigaction(SIGTERM, &action, nullptr);
#endif
            p->store(nullptr, std::memory_order_relaxed);
            // Don't reorder this store with subsequent destruction of solver
            std::atomic_signal_fence(std::memory_order_release);
        };
        return std::unique_ptr<solver_to_stop_t, decltype(detach_solver)>{
            &solver_to_stop, detach_solver};
    } else {
        struct [[maybe_unused]] empty {};
        return empty{};
    }
}

} // namespace alpaqa