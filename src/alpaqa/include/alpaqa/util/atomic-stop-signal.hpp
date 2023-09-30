#pragma once

#include <atomic>

namespace alpaqa {

class AtomicStopSignal {
  public:
    AtomicStopSignal() = default;
    AtomicStopSignal(const AtomicStopSignal &) : AtomicStopSignal() {}
    AtomicStopSignal &operator=(const AtomicStopSignal &) = delete;
    AtomicStopSignal(AtomicStopSignal &&) noexcept : AtomicStopSignal() {}
    AtomicStopSignal &operator=(AtomicStopSignal &&) noexcept { return *this; }

    void stop() { stop_flag.store(true, std::memory_order_seq_cst); }
    [[nodiscard]] bool stop_requested() const {
        return stop_flag.load(std::memory_order_relaxed);
    }

  private:
    std::atomic<bool> stop_flag{false};
};

} // namespace alpaqa