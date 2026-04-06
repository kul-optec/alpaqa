#include <algorithm>
#include <cstdlib>
#include <iterator>
#include <ska_sort.hpp>

int main() {
    int a[] {-93, 43, 0,   37,  53,  -61, -43, -60, -92, -69,
             -81, 39, -29, -29, -89, -28, -91, 56,  75,  -81};
    ska_sort(std::begin(a), std::end(a));
    return std::is_sorted(std::begin(a), std::end(a)) ? EXIT_SUCCESS
                                                      : EXIT_FAILURE;
}
