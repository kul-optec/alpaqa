#include <alpaqa/example-util.hpp>

#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#endif

namespace alpaqa {
void init_stdout() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
}
} // namespace alpaqa
