#pragma once

// Data types shared between the DLL and the main program.

#include <stdexcept>
#include <string>

struct InitData {
    std::string message;
};

struct dl_exception : std::runtime_error {
    dl_exception(const std::string &msg) : std::runtime_error(msg) {}
};
