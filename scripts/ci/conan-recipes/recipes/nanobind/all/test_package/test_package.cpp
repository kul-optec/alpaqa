#include <nanobind/nanobind.h>

static int add(int i, int j) { return i + j; }

static const char *hello() { return "Hello from the C++ world!"; }

NB_MODULE(test_package, m) {
    m.doc() = "nanobind example plugin"; // optional module docstring

    m.def("add", &add, "A function which adds two numbers");
    m.def("msg", &hello, "A function returning a message");
}
