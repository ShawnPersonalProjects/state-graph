#include <gtest/gtest.h>
#include <stdexcept>
#include <limits>
#include <chrono>
#include <cmath>
#include "graph/value.hpp"

// Test fixture for Value tests
class ValueTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set up common test data
        int_value = static_cast<int64_t>(42);
        negative_int_value = static_cast<int64_t>(-17);
        zero_int_value = static_cast<int64_t>(0);
        
        double_value = 3.14;
        negative_double_value = -2.5;
        zero_double_value = 0.0;
        
        true_value = true;
        false_value = false;
        
        string_value = std::string("hello world");
        empty_string_value = std::string("");
    }

    // Test data
    Value int_value;
    Value negative_int_value;
    Value zero_int_value;
    Value double_value;
    Value negative_double_value;
    Value zero_double_value;
    Value true_value;
    Value false_value;
    Value string_value;
    Value empty_string_value;
};

// Test Value variant creation and type checking
TEST_F(ValueTest, VariantCreation) {
    // Test int64_t values
    EXPECT_TRUE(std::holds_alternative<int64_t>(int_value));
    EXPECT_EQ(std::get<int64_t>(int_value), 42);
    
    EXPECT_TRUE(std::holds_alternative<int64_t>(negative_int_value));
    EXPECT_EQ(std::get<int64_t>(negative_int_value), -17);
    
    EXPECT_TRUE(std::holds_alternative<int64_t>(zero_int_value));
    EXPECT_EQ(std::get<int64_t>(zero_int_value), 0);
    
    // Test double values
    EXPECT_TRUE(std::holds_alternative<double>(double_value));
    EXPECT_DOUBLE_EQ(std::get<double>(double_value), 3.14);
    
    EXPECT_TRUE(std::holds_alternative<double>(negative_double_value));
    EXPECT_DOUBLE_EQ(std::get<double>(negative_double_value), -2.5);
    
    EXPECT_TRUE(std::holds_alternative<double>(zero_double_value));
    EXPECT_DOUBLE_EQ(std::get<double>(zero_double_value), 0.0);
    
    // Test bool values
    EXPECT_TRUE(std::holds_alternative<bool>(true_value));
    EXPECT_TRUE(std::get<bool>(true_value));
    
    EXPECT_TRUE(std::holds_alternative<bool>(false_value));
    EXPECT_FALSE(std::get<bool>(false_value));
    
    // Test string values
    EXPECT_TRUE(std::holds_alternative<std::string>(string_value));
    EXPECT_EQ(std::get<std::string>(string_value), "hello world");
    
    EXPECT_TRUE(std::holds_alternative<std::string>(empty_string_value));
    EXPECT_EQ(std::get<std::string>(empty_string_value), "");
}

// Test toNumber function with valid inputs
TEST_F(ValueTest, ToNumberValidInputs) {
    // Test int64_t to double conversion
    EXPECT_DOUBLE_EQ(toNumber(int_value), 42.0);
    EXPECT_DOUBLE_EQ(toNumber(negative_int_value), -17.0);
    EXPECT_DOUBLE_EQ(toNumber(zero_int_value), 0.0);
    
    // Test double values
    EXPECT_DOUBLE_EQ(toNumber(double_value), 3.14);
    EXPECT_DOUBLE_EQ(toNumber(negative_double_value), -2.5);
    EXPECT_DOUBLE_EQ(toNumber(zero_double_value), 0.0);
}

// Test toNumber function with invalid inputs
TEST_F(ValueTest, ToNumberInvalidInputs) {
    // Test error cases - should throw std::runtime_error
    EXPECT_THROW(toNumber(true_value), std::runtime_error);
    EXPECT_THROW(toNumber(false_value), std::runtime_error);
    EXPECT_THROW(toNumber(string_value), std::runtime_error);
    EXPECT_THROW(toNumber(empty_string_value), std::runtime_error);
}

// Test toNumber with extreme values
TEST_F(ValueTest, ToNumberExtremeValues) {
    // Test maximum int64_t
    Value max_int = static_cast<int64_t>(std::numeric_limits<int64_t>::max());
    EXPECT_NO_THROW(toNumber(max_int));
    EXPECT_DOUBLE_EQ(toNumber(max_int), static_cast<double>(std::numeric_limits<int64_t>::max()));
    
    // Test minimum int64_t
    Value min_int = static_cast<int64_t>(std::numeric_limits<int64_t>::min());
    EXPECT_NO_THROW(toNumber(min_int));
    EXPECT_DOUBLE_EQ(toNumber(min_int), static_cast<double>(std::numeric_limits<int64_t>::min()));
    
    // Test very small and large doubles
    Value small_double = std::numeric_limits<double>::min();
    EXPECT_DOUBLE_EQ(toNumber(small_double), std::numeric_limits<double>::min());
    
    Value large_double = std::numeric_limits<double>::max();
    EXPECT_DOUBLE_EQ(toNumber(large_double), std::numeric_limits<double>::max());
    
    // Test special double values
    Value inf_value = std::numeric_limits<double>::infinity();
    EXPECT_TRUE(std::isinf(toNumber(inf_value)));
    
    Value neg_inf_value = -std::numeric_limits<double>::infinity();
    EXPECT_TRUE(std::isinf(toNumber(neg_inf_value)));
}

// Test toBool function with valid inputs
TEST_F(ValueTest, ToBoolValidInputs) {
    EXPECT_TRUE(toBool(true_value));
    EXPECT_FALSE(toBool(false_value));
}

// Test toBool function with invalid inputs
TEST_F(ValueTest, ToBoolInvalidInputs) {
    // Test error cases - should throw std::runtime_error
    EXPECT_THROW(toBool(int_value), std::runtime_error);
    EXPECT_THROW(toBool(negative_int_value), std::runtime_error);
    EXPECT_THROW(toBool(zero_int_value), std::runtime_error);
    EXPECT_THROW(toBool(double_value), std::runtime_error);
    EXPECT_THROW(toBool(negative_double_value), std::runtime_error);
    EXPECT_THROW(toBool(zero_double_value), std::runtime_error);
    EXPECT_THROW(toBool(string_value), std::runtime_error);
    EXPECT_THROW(toBool(empty_string_value), std::runtime_error);
}

// Test toString function with valid inputs
TEST_F(ValueTest, ToStringValidInputs) {
    EXPECT_EQ(toString(string_value), "hello world");
    EXPECT_EQ(toString(empty_string_value), "");
    
    // Test special characters
    Value special_chars = std::string("special chars: !@#$%^&*()");
    EXPECT_EQ(toString(special_chars), "special chars: !@#$%^&*()");
    
    // Test unicode/extended characters
    Value unicode_string = std::string("café résumé naïve");
    EXPECT_EQ(toString(unicode_string), "café résumé naïve");
    
    // Test newlines and tabs
    Value multiline_string = std::string("line1\nline2\tindented");
    EXPECT_EQ(toString(multiline_string), "line1\nline2\tindented");
}

// Test toString function with invalid inputs
TEST_F(ValueTest, ToStringInvalidInputs) {
    // Test error cases - should throw std::runtime_error
    EXPECT_THROW(toString(int_value), std::runtime_error);
    EXPECT_THROW(toString(negative_int_value), std::runtime_error);
    EXPECT_THROW(toString(zero_int_value), std::runtime_error);
    EXPECT_THROW(toString(double_value), std::runtime_error);
    EXPECT_THROW(toString(negative_double_value), std::runtime_error);
    EXPECT_THROW(toString(zero_double_value), std::runtime_error);
    EXPECT_THROW(toString(true_value), std::runtime_error);
    EXPECT_THROW(toString(false_value), std::runtime_error);
}

// Test edge cases and special scenarios
TEST_F(ValueTest, EdgeCases) {
    // Test very long string
    std::string long_string(10000, 'x');
    Value long_string_value = long_string;
    EXPECT_EQ(toString(long_string_value), long_string);
    
    // Test string with null characters
    std::string null_string = "before\0after";
    Value null_string_value = null_string;
    EXPECT_EQ(toString(null_string_value), null_string);
    
    // Test zero values
    EXPECT_DOUBLE_EQ(toNumber(zero_int_value), 0.0);
    EXPECT_DOUBLE_EQ(toNumber(zero_double_value), 0.0);
    
    // Test negative zero
    Value neg_zero = -0.0;
    EXPECT_DOUBLE_EQ(toNumber(neg_zero), -0.0);
    
    // Test boolean edge cases
    EXPECT_TRUE(toBool(true_value));
    EXPECT_FALSE(toBool(false_value));
}

// Test Value assignment and copying
TEST_F(ValueTest, AssignmentAndCopying) {
    // Test copy construction
    Value copy_int = int_value;
    EXPECT_TRUE(std::holds_alternative<int64_t>(copy_int));
    EXPECT_EQ(std::get<int64_t>(copy_int), 42);
    
    Value copy_string = string_value;
    EXPECT_TRUE(std::holds_alternative<std::string>(copy_string));
    EXPECT_EQ(std::get<std::string>(copy_string), "hello world");
    
    // Test assignment
    Value assigned_value = static_cast<int64_t>(100);
    assigned_value = string_value;
    EXPECT_TRUE(std::holds_alternative<std::string>(assigned_value));
    EXPECT_EQ(std::get<std::string>(assigned_value), "hello world");
    
    // Test move semantics
    std::string original = "move test";
    Value move_value = std::move(original);
    EXPECT_TRUE(std::holds_alternative<std::string>(move_value));
    EXPECT_EQ(std::get<std::string>(move_value), "move test");
}

// Test error message content
TEST_F(ValueTest, ErrorMessages) {
    try {
        toNumber(true_value);
        FAIL() << "Expected std::runtime_error";
    } catch (const std::runtime_error& e) {
        EXPECT_STREQ(e.what(), "Value is not numeric");
    }
    
    try {
        toBool(int_value);
        FAIL() << "Expected std::runtime_error";
    } catch (const std::runtime_error& e) {
        EXPECT_STREQ(e.what(), "Value is not bool");
    }
    
    try {
        toString(int_value);
        FAIL() << "Expected std::runtime_error";
    } catch (const std::runtime_error& e) {
        EXPECT_STREQ(e.what(), "Value is not string");
    }
}

// Test type compatibility
TEST_F(ValueTest, TypeCompatibility) {
    // Test that we can store different numeric types
    Value short_value = static_cast<int64_t>(static_cast<short>(123));
    EXPECT_DOUBLE_EQ(toNumber(short_value), 123.0);
    
    Value long_value = static_cast<int64_t>(123456789L);
    EXPECT_DOUBLE_EQ(toNumber(long_value), 123456789.0);
    
    Value float_value = static_cast<double>(static_cast<float>(1.5f));
    EXPECT_DOUBLE_EQ(toNumber(float_value), 1.5);
    
    // Test string literals
    Value string_literal = std::string("literal");
    EXPECT_EQ(toString(string_literal), "literal");
    
    // Test const char* conversion
    const char* c_string = "c string";
    Value c_string_value = std::string(c_string);
    EXPECT_EQ(toString(c_string_value), "c string");
}

// Parameterized test for numeric conversions
class NumericConversionTest : public ::testing::TestWithParam<std::pair<int64_t, double>> {};

TEST_P(NumericConversionTest, IntToDoubleConversion) {
    auto [int_val, expected_double] = GetParam();
    Value v = int_val;
    EXPECT_DOUBLE_EQ(toNumber(v), expected_double);
}

INSTANTIATE_TEST_SUITE_P(
    VariousIntegers,
    NumericConversionTest,
    ::testing::Values(
        std::make_pair(0, 0.0),
        std::make_pair(1, 1.0),
        std::make_pair(-1, -1.0),
        std::make_pair(42, 42.0),
        std::make_pair(-42, -42.0),
        std::make_pair(1000, 1000.0),
        std::make_pair(-1000, -1000.0)
    )
);

// Parameterized test for boolean values
class BooleanTest : public ::testing::TestWithParam<bool> {};

TEST_P(BooleanTest, BooleanValues) {
    bool bool_val = GetParam();
    Value v = bool_val;
    EXPECT_EQ(toBool(v), bool_val);
}

INSTANTIATE_TEST_SUITE_P(
    BooleanValues,
    BooleanTest,
    ::testing::Values(true, false)
);

// Parameterized test for string values
class StringTest : public ::testing::TestWithParam<std::string> {};

TEST_P(StringTest, StringValues) {
    std::string str_val = GetParam();
    Value v = str_val;
    EXPECT_EQ(toString(v), str_val);
}

INSTANTIATE_TEST_SUITE_P(
    VariousStrings,
    StringTest,
    ::testing::Values(
        "",
        "a",
        "hello",
        "hello world",
        "special!@#$%^&*()",
        "with\nnewlines\tand\ttabs",
        "very long string that exceeds typical buffer sizes and tests memory handling",
        "unicode: café résumé naïve 中文 العربية русский"
    )
);

// Performance test for large-scale operations
TEST_F(ValueTest, PerformanceTest) {
    const int iterations = 100000;
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < iterations; ++i) {
        Value v = static_cast<int64_t>(i);
        double result = toNumber(v);
        (void)result; // Avoid unused variable warning
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // This should complete in reasonable time (less than 1 second)
    EXPECT_LT(duration.count(), 1000000);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}