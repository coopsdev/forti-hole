//
// Created by Cooper Larson on 8/30/24.
//

#include "include/forti_hole.h"
#include <gtest/gtest.h>
#include <iostream>

#define ALLOW_RISKY_TESTS_ON_LOCAL_DEVICE true

#if ALLOW_RISKY_TESTS_ON_LOCAL_DEVICE

TEST(TestFortiHole, TestPopulateThreatFeed) {
    try {
        FortiHole fortiHole;
        fortiHole();
    } catch (const std::exception& e) {
        std::cout << e.what() << std::endl;
        FAIL();
    }
}

#endif
