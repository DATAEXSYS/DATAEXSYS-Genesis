// test/unit/adhoc/net/ControlPlane/routecache_test.cpp

#include "adhoc/net/ControlPlane/RouteCache.h"
#include <iostream>
#include <cassert>
#include <optional>
#include <vector>
#include <cstdint>

// Helper function to print current route table
void printRouteCache(const RouteCache &rc, const std::string &title) {
    std::cout << "=== " << title << " ===\n";

    bool empty = true;

    for (uint8_t dest = 0; dest <= 254; ++dest) {
        auto neighbor = rc.findNeighbor(dest);
        if (neighbor.has_value()) {
            std::cout << "Dest: " << +dest << " -> Next hop: " << +neighbor.value() << "\n";
            empty = false;
        }
    }

    if (empty) {
        std::cout << "(RouteCache is empty)\n";
    }

    std::cout << "===========================\n";
}

int main() {
    std::cout << "Starting RouteCache tests...\n";

    RouteCache rc;

    // Test 1: Add neighbors
    rc.addNeighbor(1, 10);
    rc.addNeighbor(2, 20);
    rc.addNeighbor(3, 30);
    printRouteCache(rc, "After adding neighbors");

    // Test 2: Check hasNeighbor
    assert(rc.hasNeighbor(1) == true);
    assert(rc.hasNeighbor(2) == true);
    assert(rc.hasNeighbor(3) == true);
    assert(rc.hasNeighbor(99) == false);

    std::cout << "HasNeighbor tests passed.\n";

    // Test 3: Find neighbor
    auto n1 = rc.findNeighbor(1);
    auto n2 = rc.findNeighbor(2);
    auto n3 = rc.findNeighbor(3);
    auto n99 = rc.findNeighbor(99);

    assert(n1.has_value() && n1.value() == 10);
    assert(n2.has_value() && n2.value() == 20);
    assert(n3.has_value() && n3.value() == 30);
    assert(!n99.has_value());

    std::cout << "FindNeighbor tests passed.\n";

    // Test 4: Delete neighbor
    rc.deleteNeighbor(2);
    printRouteCache(rc, "After deleting neighbor 2");

    assert(rc.hasNeighbor(2) == false);

    // Test 5: Overwrite neighbor
    rc.addNeighbor(1, 42); // overwrite 10 -> 42
    printRouteCache(rc, "After overwriting neighbor 1");

    auto n1_updated = rc.findNeighbor(1);
    assert(n1_updated.has_value() && n1_updated.value() == 42);

    std::cout << "All RouteCache tests passed successfully!\n";
    return 0;
}
