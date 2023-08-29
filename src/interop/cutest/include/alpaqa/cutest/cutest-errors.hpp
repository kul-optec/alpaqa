#include <stdexcept>

namespace alpaqa {

struct cutest_function_load_error : std::runtime_error {
    using std::runtime_error::runtime_error;
};

}; // namespace alpaqa