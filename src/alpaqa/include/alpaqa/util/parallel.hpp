#pragma once

#include <algorithm>
#include <cstddef>

#ifdef _OPENMP
#include <omp.h>
#endif

namespace alpaqa::util {

/**
 * @brief Struct representing the input parameters for work distribution.
 * @see @ref distribute_work
 */
struct WorkDistributionInput {
    ptrdiff_t total_work;  ///< Total amount of work to be distributed.
    ptrdiff_t chunk_size;  ///< Size of each work chunk.
    ptrdiff_t num_threads; ///< Number of threads for parallelization.
};

/**
 * @brief Struct representing the result of work distribution.
 * @see @ref distribute_work
 */
struct WorkDistributionResult {
    /// Amount of work assigned to each thread.
    ptrdiff_t work_per_thread;
    /// Amount of work for the last thread which may have a smaller chunk.
    ptrdiff_t work_for_last_thread;
    /// Index of the last thread that has work assigned.
    ptrdiff_t last_thread_index;
};

/**
 * @brief Distributes work accross threads based on the given input parameters.
 * 
 * The function calculates how to distribute the total work among multiple
 * threads, ensuring each thread gets a balanced workload as much as possible.
 * 
 * @param   input
 *          The input parameters for work distribution.
 * @return  Result of the work distribution.
 */
inline WorkDistributionResult
distribute_work(const WorkDistributionInput &input) {
    WorkDistributionResult result;
    // Calculate the number of chunks based on the total work and chunk size
    ptrdiff_t num_chunks =
        (input.total_work + input.chunk_size - 1) / input.chunk_size;
    // Calculate how many chunks each thread should process, rounding up
    ptrdiff_t chunks_per_thread =
        (num_chunks + input.num_threads - 1) / input.num_threads;
    // Calculate the total work assigned to each thread
    result.work_per_thread = chunks_per_thread * input.chunk_size;
    // Calculate the index of the last thread that has work assigned
    result.last_thread_index =
        result.work_per_thread ? (input.total_work - 1) / result.work_per_thread
                               : 0;
    // Calculate the work assigned to the last thread, which may have a smaller chunk
    result.work_for_last_thread = std::min(
        result.work_per_thread,
        input.total_work - result.last_thread_index * result.work_per_thread);
    return result;
}

/**
 * @brief Parallelizes the execution of the given callable.
 * 
 * @param   total_work
 *          The total amount of work to be distributed.
 * @param   chunk_size
 *          The minimum unit of work per thread (except the last thread, which
 *          may contain fewer items than the others). This is useful to prevent
 *          false sharing and to ensure alignment.
 * @param   callable
 *          The callable function to be executed in parallel. Accepts the start
 *          index and the size of its work as arguments.
 */
template <typename Callable>
void in_parallel(ptrdiff_t total_work, ptrdiff_t chunk_size,
                 Callable &&callable) {
#ifdef _OPENMP
#pragma omp parallel
    {
        ptrdiff_t num_threads = omp_get_num_threads();
        auto distrib          = distribute_work({.total_work  = total_work,
                                                 .chunk_size  = chunk_size,
                                                 .num_threads = num_threads});
        auto thread_id        = omp_get_thread_num();
        auto start_index      = thread_id * distrib.work_per_thread;
        auto length           = (thread_id < distrib.last_thread_index)
                                    ? distrib.work_per_thread
                                    : distrib.work_for_last_thread;
        if (thread_id <= distrib.last_thread_index && length > 0)
            callable(start_index, length);
    }
#else
    static_cast<void>(chunk_size);
    callable(0, total_work);
#endif
}

} // namespace alpaqa::util