#pragma once

#include <alpaqa/export.h>
#include <filesystem>
#include <memory>
#include <stdexcept>

namespace alpaqa ::util {

/// Failed to load a DLL or SO file, or failed to access a function in it.
struct ALPAQA_EXPORT dynamic_load_error : std::runtime_error {
    using std::runtime_error::runtime_error;
};

/// Load a DLL or SO file.
ALPAQA_EXPORT std::shared_ptr<void>
load_lib(const std::filesystem::path &so_filename);
/// Get a pointer to a function inside of a loaded DLL or SO file.
ALPAQA_EXPORT void *load_func(void *lib_handle, const std::string &name);

} // namespace alpaqa::util
