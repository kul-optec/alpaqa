#include "dict/kwargs-to-struct.hpp"

void register_misc(py::module_ &m) {
    register_dataclass<guanaqo::DynamicLoadFlags>(
        m, "DynamicLoadFlags", "C++ documentation: :cpp:class:`guanaqo::DynamicLoadFlags`");
}
