#pragma once

#include <alpaqa/export.h>

#ifndef DOXYGEN

#ifdef _WIN32
#define ALPAQA_EXPORT_EXTERN_TEMPLATE(strcls, name, ...)                       \
    extern template strcls name<__VA_ARGS__>
#define ALPAQA_EXPORT_TEMPLATE(strcls, name, ...)                              \
    template strcls ALPAQA_EXPORT name<__VA_ARGS__>
#define ALPAQA_EXPORT_STATIC /* For static functions in an exported class */
#define ALPAQA_EXPORT_EXCEPTION
#else
#define ALPAQA_EXPORT_EXTERN_TEMPLATE(strcls, name, ...)                       \
extern template strcls ALPAQA_EXPORT name<__VA_ARGS__>
#define ALPAQA_EXPORT_TEMPLATE(strcls, name, ...)                              \
template strcls name<__VA_ARGS__>
#define ALPAQA_EXPORT_STATIC ALPAQA_EXPORT
#define ALPAQA_EXPORT_EXCEPTION ALPAQA_EXPORT
#endif

#else // DOXYGEN

#define ALPAQA_EXPORT_EXTERN_TEMPLATE(...)
#define ALPAQA_EXPORT_TEMPLATE(...)

#endif // DOXYGEN
