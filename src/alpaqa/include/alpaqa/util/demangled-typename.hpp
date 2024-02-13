#pragma once

#include <alpaqa/export.hpp>

#include <string>

#ifndef ALPAQA_WITH_RTTI
#if !defined(__GNUC__) || defined(__GXX_RTTI)
#define ALPAQA_WITH_RTTI 1
#else
#define ALPAQA_WITH_RTTI 0
#endif
#endif

#if ALPAQA_WITH_RTTI

#include <typeinfo>

/// Get the pretty name of the given type as a string.
ALPAQA_EXPORT std::string demangled_typename(const std::type_info &t);

#else

#define demangled_typename(...) (std::string("unknown type"))

#endif
