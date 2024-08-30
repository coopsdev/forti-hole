#include <iostream>
#include "include/forti_hole.h"

int main() {
    auto start = std::chrono::high_resolution_clock::now();
    std::cout << "Starting forti-hole..." << std::endl;

    FortiHole fortiHole;
    fortiHole.update_threat_feeds();

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "forti-hole finished in " << duration.count() << "ms" << std::endl;

    return 0;
}
