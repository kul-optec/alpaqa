#include <batmat-version.h>
#include <iostream>

int main() {
    std::cout << BATMAT_VERSION_FULL << " (" << batmat_commit_hash << ")\n";
}
