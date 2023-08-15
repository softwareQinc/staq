#include "gtest/gtest.h"

#include "grid_synth/regions.hpp"
#include "grid_synth/constants.hpp"
#include "grid_synth/gmp_functions.hpp"
#include "grid_synth/rings.hpp"

using namespace staq;
using namespace grid_synth;

TEST(IntervalTest, ContainsElement) {
    EXPECT_TRUE(Interval<double>(0.0, 1.0).contains(0.5));
    EXPECT_TRUE(Interval<int>(-10, 11).contains(0));
    EXPECT_FALSE(Interval<int>(-5, -2).contains(1));
    EXPECT_FALSE(Interval<double>(1.3, 3.6).contains(5.0));
}

TEST(IntervalTest, ContainsAfterShift) {
    Interval<double> I(0.0, 5.0);
    Interval<double> I_up = I + 10.5;
    Interval<double> I_dn = I - 11.0;

    EXPECT_TRUE(I_up.contains(13.5));
    EXPECT_TRUE(I_dn.contains(-9.5));
}

TEST(IntervalTest, BoundsAfterScaling) {
    Interval<int> I(-3, 10);

    I.rescale(10);

    EXPECT_TRUE((I.lo() == -30) && (I.hi() == 100));

    I.shift(-5);

    EXPECT_TRUE((I.lo() == -35) && (I.hi() == 95));
}

TEST(IntervalTest, ContainsAfterScale) {
    Interval<double> I(-6, 10.0);

    EXPECT_TRUE((I * 5).contains(-24.3));
    EXPECT_TRUE((I * (-6)).contains(-45));
    EXPECT_FALSE((I * 2).contains(234.4));
    EXPECT_FALSE((I * (-10)).contains(100));

    EXPECT_TRUE((I / 6).contains(0.1));
    EXPECT_TRUE((I / (-2)).contains(-4.0));
}

TEST(UprightRectangleTest, Contains) {
    Interval<double> I_x(0.0, 1.0);
    Interval<double> I_y(5.0, 10.0);

    UprightRectangle<double> R(I_x, I_y);

    EXPECT_TRUE(R.contains(0.0, 6.0));
}

TEST(EllipseTest, Constructors) {
    real_t x0 = 5;
    real_t y0 = 6;
    real_t a = 10;
    real_t b = 6;
    real_t t = 0.5 * PI;

    Ellipse A(x0, y0, a, b, t);

    Ellipse B(A.center(), A.D());

    EXPECT_TRUE((A.D() - B.D()).norm() < 100 * TOL);
    EXPECT_TRUE((A.area() - B.area()) < 100 * TOL);
    // EXPECT_TRUE( (A.angle() - B.angle()) < TOL);
}

TEST(EllipseTest, Contains) {
    Ellipse unit_circle(0, 0, 1, 1, 0);

    EXPECT_TRUE(unit_circle.contains(0, 0));
    EXPECT_TRUE(unit_circle.contains(0.5, 0.5));
    EXPECT_FALSE(unit_circle.contains(100, 100));

    Ellipse shifted_unit_circle(1, 1, 1, 1, 0);

    EXPECT_FALSE(shifted_unit_circle.contains(vec_t{{0, 0}}));
    EXPECT_TRUE(shifted_unit_circle.contains(cplx_t(1, 1)));
}

TEST(EllipseTest, BoundingBoxDimensions) {
    Ellipse two_to_one(0, 0, 2, 1, 0);

    UprightRectangle bbox = two_to_one.bounding_box();

    EXPECT_TRUE(bbox.contains(0, 0));
    EXPECT_TRUE(bbox.contains(0, 1));
    EXPECT_TRUE(bbox.contains(0.5, 0));
    EXPECT_TRUE(bbox.contains(1, 1.5));

    EXPECT_FALSE(bbox.contains(10, 10));
}

TEST(EllipseTest, AreaRatio) {
    Ellipse A(0, 0, 2, 10.1, 0);
    UprightRectangle bbox = A.bounding_box();

    EXPECT_TRUE(abs((A.area() / bbox.area()) - A.up()) < TOL);
    EXPECT_TRUE(A.area() < bbox.area());

    A = Ellipse(10, 12, 2, 10.1, 0.5 * PI);
    bbox = A.bounding_box();

    EXPECT_TRUE(abs((A.area() / bbox.area()) - A.up()) < TOL);
    EXPECT_TRUE(A.area() < bbox.area());

    A = Ellipse(-5, -5, 15, 10, 0.5 * PI);
    bbox = A.bounding_box();

    EXPECT_TRUE(abs(600 - bbox.area()) < TOL);
}

TEST(EllipseTest, ZAndE) {
    Ellipse A(0, 0, 4, 6, 0);
    real_t a = A.D(0, 0);
    real_t c = A.D(1, 1);

    real_t z = A.z();
    real_t e = A.e();

    EXPECT_TRUE(abs(e * std::pow(LAMBDA.decimal().get_d(), -z.get_d()) - a) <
                TOL);
    EXPECT_TRUE(abs(e * std::pow(LAMBDA.decimal().get_d(), z.get_d()) - c) <
                TOL);
}

TEST(EllipseTest, Normalization) {
    Ellipse A(-120, -0.123, 10, 11, 0.89);

    real_t area = A.area();
    real_t scale = A.normalize();

    EXPECT_TRUE((A.area() - PI) < 100 * TOL);

    A.rescale(1.0 / scale);

    EXPECT_TRUE((area - A.area()) < 100 * TOL);
}
