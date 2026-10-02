#include <alpaqa/example-util.hpp>

#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#endif

#if LIB_PICO_STDIO_USB
#include <pico/stdio_usb.h>
#include <pico/time.h>
#endif

namespace alpaqa {
void init_stdout() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
#if LIB_PICO_STDIO_USB
    stdio_init_all();
    while (!stdio_usb_connected())
        sleep_ms(100);
#endif
}
} // namespace alpaqa
