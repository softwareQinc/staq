#include "gtest/gtest.h"
#include "mapping/device.hpp"
#include <set>

using namespace staq;

// Testing devices
using steiner_edges = std::set<std::pair<int, int>>;

bool subset(const steiner_edges& A, const steiner_edges& B) {
    for (auto it = A.begin(); it != A.end(); it++) {
        if (B.find(*it) == B.end())
            return false;
    }

    return true;
}

static mapping::Device test_device("Test device", 9,
                                   {
                                       {0, 1, 0, 0, 0, 1, 0, 0, 0},
                                       {1, 0, 1, 0, 1, 0, 0, 0, 0},
                                       {0, 1, 0, 1, 0, 0, 0, 0, 0},
                                       {0, 0, 1, 0, 1, 0, 0, 0, 1},
                                       {0, 1, 0, 1, 0, 1, 0, 1, 0},
                                       {1, 0, 0, 0, 1, 0, 1, 0, 0},
                                       {0, 0, 0, 0, 0, 1, 0, 1, 0},
                                       {0, 0, 0, 0, 1, 0, 1, 0, 1},
                                       {0, 0, 0, 1, 0, 0, 0, 1, 0},
                                   },
                                   {1, 1, 1, 1, 1, 1, 1, 1, 1},
                                   {
                                       {0, 0.9, 0, 0, 0, 0.1, 0, 0, 0},
                                       {0.9, 0, 0.1, 0, 0.9, 0, 0, 0, 0},
                                       {0, 0.1, 0, 0.1, 0, 0, 0, 0, 0},
                                       {0, 0, 0.1, 0, 0.1, 0, 0, 0, 0.1},
                                       {0, 0.9, 0, 0.1, 0, 0.1, 0, 0.9, 0},
                                       {0.1, 0, 0, 0, 0.1, 0, 0.1, 0, 0},
                                       {0, 0, 0, 0, 0, 0.1, 0, 0.1, 0},
                                       {0, 0, 0, 0, 0.9, 0, 0.9, 0, 0.9},
                                       {0, 0, 0, 0.1, 0, 0, 0, 0.11, 0},
                                   });

/******************************************************************************/
TEST(Device, Couplings) {
    EXPECT_TRUE(test_device.coupled(3, 4));
    EXPECT_FALSE(test_device.coupled(3, 5));
}
/******************************************************************************/

/******************************************************************************/
TEST(Device, Out_Of_Range) {
    EXPECT_NO_THROW(test_device.sq_fidelity(0));
    EXPECT_THROW(test_device.sq_fidelity(9), std::out_of_range);
    EXPECT_THROW(test_device.sq_fidelity(-1), std::out_of_range);
}
/******************************************************************************/

/******************************************************************************/
TEST(Device, Shortest_Path) {
    EXPECT_EQ(test_device.shortest_path(0, 2), mapping::path({0, 1, 2}));
    EXPECT_EQ(test_device.shortest_path(0, 6), mapping::path({0, 1, 4, 7, 6}));
    EXPECT_EQ(test_device.shortest_path(4, 8), mapping::path({4, 7, 8}));
    EXPECT_EQ(test_device.shortest_path(8, 0), mapping::path({8, 7, 4, 1, 0}));
}
/******************************************************************************/

/******************************************************************************/
TEST(Device, Shortest_Path_tokyo) {
    mapping::Device test = mapping::parse_json(PATH "/qpus/ibm_tokyo.json");

    EXPECT_TRUE(test.coupled(8, 7));
    EXPECT_TRUE(test.coupled(7, 6));
    EXPECT_TRUE(test.coupled(6, 5));
    EXPECT_EQ(test.shortest_path(8, 5), mapping::path({8, 7, 6, 5}));
}
/******************************************************************************/

/******************************************************************************/
TEST(Device, Steiner_tree) {
    auto tmp1 = test_device.steiner(std::list<int>({2, 6}), 0);
    auto tmp2 = test_device.steiner(std::list<int>({3, 8}), 1);
    auto tmp3 = test_device.steiner(std::list<int>({2, 7}), 0);
    auto tmp4 =
        test_device.steiner(std::list<int>({1, 2, 3, 4, 5, 6, 7, 8}), 0);

    EXPECT_EQ(steiner_edges(tmp1.begin(), tmp1.end()),
              steiner_edges({{0, 1}, {1, 4}, {4, 7}, {7, 6}, {1, 2}}));
    EXPECT_EQ(steiner_edges(tmp2.begin(), tmp2.end()),
              steiner_edges({{1, 4}, {4, 7}, {7, 8}, {4, 3}}));
    EXPECT_EQ(steiner_edges(tmp3.begin(), tmp3.end()),
              steiner_edges({{0, 1}, {1, 4}, {4, 7}, {1, 2}}));
    EXPECT_TRUE(subset(steiner_edges({{0, 1}, {1, 4}, {4, 7}, {7, 6}, {7, 8}}),
                       steiner_edges(tmp4.begin(), tmp4.end())));
}
/******************************************************************************/
