#include "gtest/gtest.h"

#include "grid_synth/grid_solvers.hpp"

using namespace staq;
using namespace grid_synth;

TEST(OneDGridSolverTest, OneSolution) {
    Interval<real_t> A(0.0, 0.1);
    Interval<real_t> B(0.0, 0.1);
    EXPECT_TRUE(1 == oneD_grid_solver<real_t>(A, B).size());
}

TEST(OneDGridSolverTest, EdgeSolutions) {
    Interval<real_t> A(0.0, 1 + SQRT2);
    Interval<real_t> B(-SQRT2, 1.0);

    EXPECT_TRUE(4 == oneD_grid_solver<real_t>(A, B).size());
}

TEST(OneDGridSolverTest, IntervalConfirmation) {
    Interval<real_t> A(-10, 10);
    Interval<real_t> B(30, 40);

    zsqrt2_vec_t solns = oneD_grid_solver<real_t>(A, B);

    for (auto soln : solns) {
        EXPECT_TRUE(A.contains(soln.decimal()));
        EXPECT_TRUE(B.contains(soln.dot().decimal()));
    }

    A = Interval<real_t>(100, 200);
    B = Interval<real_t>(150, 250);

    solns = oneD_grid_solver<real_t>(A, B);

    for (auto soln : solns) {
        EXPECT_TRUE(A.contains(soln.decimal()));
        EXPECT_TRUE(B.contains(soln.dot().decimal()));
    }

    A = Interval<real_t>(-100, -50);
    B = Interval<real_t>(31.1, 54.2);

    solns = oneD_grid_solver<real_t>(A, B);

    for (auto soln : solns) {
        EXPECT_TRUE(A.contains(soln.decimal()));
        EXPECT_TRUE(B.contains(soln.dot().decimal()));
    }

    A = Interval<real_t>(1.245, 123.213);
    B = Interval<real_t>(-1231.123, -123.13123);

    solns = oneD_grid_solver(A, B);

    for (auto soln : solns) {
        EXPECT_TRUE(A.contains(soln.decimal()));
        EXPECT_TRUE(B.contains(soln.dot().decimal()));
    }
}

TEST(OneDScaledGridSolver, SameSolutionsAsOneDGridSolver) {
    Interval<real_t> A(0.0, 10);
    Interval<real_t> B(30.0, 40.0);

    zsqrt2_vec_t unscaled_solns = oneD_grid_solver(A, B);
    zsqrt2_vec_t scaled_solns = oneD_grid_solver(A, B);

    EXPECT_TRUE(unscaled_solns.size() == scaled_solns.size());

    for (auto unscaled_soln : unscaled_solns) {
        EXPECT_TRUE(std::find(scaled_solns.begin(), scaled_solns.end(),
                              unscaled_soln) != scaled_solns.end());
    }
}

TEST(TwoDScaledGridSolver, SolutionConfirmation) {
    UprightRectangle<real_t> A(0.0, 1 + SQRT2, 0.0, 1 + SQRT2);
    UprightRectangle<real_t> B(-SQRT2, 1, -SQRT2, 1);

    zomega_vec_t solns = twoD_grid_solver(A, B);

    for (auto soln : solns) {
        EXPECT_TRUE(A.contains(soln.decimal()));
        EXPECT_TRUE(B.contains(soln.dot().decimal()));
    }

    A = UprightRectangle<real_t>(-10, 10, -10, 10);
    B = UprightRectangle<real_t>(0, 10, 0, 12);

    solns = twoD_grid_solver(A, B);

    for (auto soln : solns) {
        EXPECT_TRUE(A.contains(soln.decimal()));
        EXPECT_TRUE(B.contains(soln.dot().decimal()));
    }

    A = UprightRectangle<real_t>(-10, 10, -10, 10);
    B = UprightRectangle<real_t>(-93.4, -20, -20, 10.3);

    solns = twoD_grid_solver(A, B);

    for (auto soln : solns) {
        EXPECT_TRUE(A.contains(soln.decimal()));
        EXPECT_TRUE(B.contains(soln.dot().decimal()));
    }

    A = UprightRectangle<real_t>(0.234, 13.2, 50, 60);
    B = UprightRectangle<real_t>(-10.5, 22.3, -10.23, 40.5);

    solns = twoD_grid_solver(A, B);

    for (auto soln : solns) {
        EXPECT_TRUE(A.contains(soln.decimal()));
        EXPECT_TRUE(B.contains(soln.dot().decimal()));
    }

    A = UprightRectangle<real_t>(-100.13, -10.34, -502.3, -460.23);
    B = UprightRectangle<real_t>(-134.5, -120.34, -13.45, -8.123);

    solns = twoD_grid_solver(A, B);

    for (auto soln : solns) {
        EXPECT_TRUE(A.contains(soln.decimal()));
        EXPECT_TRUE(B.contains(soln.dot().decimal()));
    }
}
