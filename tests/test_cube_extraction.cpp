#include <gtest/gtest.h>
#include "../include/polynomial.h"
#include "../include/symbols.h"

using namespace GiNaC;

class CubeExtractionTest : public ::testing::Test {
protected:
    symbol x{"x"};
    symbol y{"y"};
    symbol u{"u"};
    symbol v{"v"};
};

TEST_F(CubeExtractionTest, SimpleCube) {
    ex base = x + y;
    ex cube = pow(base, 3);
    
    auto result = poly::extract_cube_root(cube);
    
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(expand(result.value() - base).is_zero());
}

TEST_F(CubeExtractionTest, ProductOfCubes) {
    ex base1 = x + 1;
    ex base2 = y - 1;
    ex cube = pow(base1, 3) * pow(base2, 3);
    
    auto result = poly::extract_cube_root(cube);
    
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(expand(pow(result.value(), 3) - cube).is_zero());
}

TEST_F(CubeExtractionTest, NotACube) {
    ex poly = pow(x, 2) + y;
    
    auto result = poly::extract_cube_root(poly);
    
    EXPECT_FALSE(result.has_value());
}

TEST_F(CubeExtractionTest, SquareNotCube) {
    ex base = x + y;
    ex square = pow(base, 2);
    
    auto result = poly::extract_cube_root(square);
    
    EXPECT_FALSE(result.has_value());
}

TEST_F(CubeExtractionTest, ComplexCube) {
    ex base = u*x + v*y;
    ex cube = expand(pow(base, 3));
    
    auto result = poly::extract_cube_root(cube);
    
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(expand(pow(result.value(), 3) - cube).is_zero());
}

TEST_F(CubeExtractionTest, ZeroPolynomial) {
    ex zero = 0;
    
    auto result = poly::extract_cube_root(zero);
    
    EXPECT_FALSE(result.has_value());
}

TEST_F(CubeExtractionTest, NumericCube) {
    ex cube = 8;
    
    auto result = poly::extract_cube_root(cube);
    
    EXPECT_FALSE(result.has_value());
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
