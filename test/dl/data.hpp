#pragma once

// Data types shared between the DLL and the main program.

#include <stdexcept>
#include <string>

#ifdef __GNUC__
#define EXPORT_RTTI __attribute__((visibility("default")))
#else
#define EXPORT_RTTI
#endif

struct EXPORT_RTTI InitData {
    std::string message;
};

struct EXPORT_RTTI dl_exception : std::runtime_error {
    dl_exception(const std::string &msg) : std::runtime_error(msg) {}
};
