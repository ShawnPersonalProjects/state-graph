#include <gtest/gtest.h>
#include <stdexcept>
#include <limits>
#include <cmath>
#include "graph/expression.hpp"
#include "graph/node.hpp"

// Test fixture for Expression tests
class ExpressionTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a test node with various properties and variables
        testNode.properties["health"] = Value{100};
        testNode.properties["name"] = Value{std::string("TestNode")};
        testNode.properties["active"] = Value{true};
        testNode.properties["score"] = Value{42.5};
        
        testNode.setVar("x", Value{10});
        testNode.setVar("y", Value{20});
        testNode.setVar("flag", Value{false});
        testNode.setVar("message", Value{std::string("hello")});
    }

    DefaultNode testNode;
};

// Lexer Tests
class LexerTest : public ::testing::Test {
protected:
    void testTokens(const std::string& input, const std::vector<std::pair<TokKind, std::string>>& expected) {
        Lexer lexer(input);
        for (const auto& [expectedKind, expectedText] : expected) {
            Token token = lexer.next();
            EXPECT_EQ(token.kind, expectedKind) << "Token kind mismatch for: " << expectedText;
            EXPECT_EQ(token.text, expectedText) << "Token text mismatch";
        }
        // Should end with END token
        Token endToken = lexer.next();
        EXPECT_EQ(endToken.kind, TokKind::END);
    }
};

TEST_F(LexerTest, BasicTokens) {
    testTokens("true", {{TokKind::BOOL, "true"}});
    testTokens("false", {{TokKind::BOOL, "false"}});
    testTokens("123", {{TokKind::NUM, "123"}});
    testTokens("45.67", {{TokKind::NUM, "45.67"}});
    testTokens("variable", {{TokKind::ID, "variable"}});
    testTokens("properties.health", {{TokKind::ID, "properties.health"}});
}

TEST_F(LexerTest, NegativeNumbers) {
    testTokens("-1", {{TokKind::NUM, "-1"}});
    testTokens("-42", {{TokKind::NUM, "-42"}});
    testTokens("-3.14", {{TokKind::NUM, "-3.14"}});
    testTokens("-0", {{TokKind::NUM, "-0"}});
    testTokens("-123.456", {{TokKind::NUM, "-123.456"}});
}

TEST_F(LexerTest, StringLiterals) {
    testTokens("\"hello\"", {{TokKind::STR, "hello"}});
    testTokens("\"hello world\"", {{TokKind::STR, "hello world"}});
    testTokens("\"\"", {{TokKind::STR, ""}});
}

TEST_F(LexerTest, Operators) {
    testTokens("&&", {{TokKind::OP, "&&"}});
    testTokens("||", {{TokKind::OP, "||"}});
    testTokens("==", {{TokKind::OP, "=="}});
    testTokens("!=", {{TokKind::OP, "!="}});
    testTokens("<=", {{TokKind::OP, "<="}});
    testTokens(">=", {{TokKind::OP, ">="}});
    testTokens("<", {{TokKind::OP, "<"}});
    testTokens(">", {{TokKind::OP, ">"}});
    testTokens("!", {{TokKind::OP, "!"}});
}

TEST_F(LexerTest, Parentheses) {
    testTokens("()", {{TokKind::LP, "("}, {TokKind::RP, ")"}});
}

TEST_F(LexerTest, ComplexExpression) {
    testTokens("x > 5 && properties.active", {
        {TokKind::ID, "x"},
        {TokKind::OP, ">"},
        {TokKind::NUM, "5"},
        {TokKind::OP, "&&"},
        {TokKind::ID, "properties.active"}
    });
}

TEST_F(LexerTest, WhitespaceHandling) {
    testTokens("  x   >   5  ", {
        {TokKind::ID, "x"},
        {TokKind::OP, ">"},
        {TokKind::NUM, "5"}
    });
}

TEST_F(LexerTest, ErrorCases) {
    // Unterminated string
    EXPECT_THROW({
        Lexer lexer("\"unterminated");
        lexer.next();
    }, std::runtime_error);

    // Unexpected character
    EXPECT_THROW({
        Lexer lexer("@");
        lexer.next();
    }, std::runtime_error);
}

// Parser Tests
TEST_F(ExpressionTest, BooleanLiterals) {
    auto expr1 = compileExpression("true");
    EXPECT_TRUE(expr1->eval(testNode));

    auto expr2 = compileExpression("false");
    EXPECT_FALSE(expr2->eval(testNode));
}

TEST_F(ExpressionTest, NumberLiterals) {
    auto expr1 = compileExpression("1");
    EXPECT_TRUE(expr1->eval(testNode));  // Non-zero is true

    auto expr2 = compileExpression("0");
    EXPECT_FALSE(expr2->eval(testNode)); // Zero is false

    auto expr3 = compileExpression("42.5");
    EXPECT_TRUE(expr3->eval(testNode));
}

TEST_F(ExpressionTest, NegativeNumbers) {
    // Test negative number literals
    auto expr1 = compileExpression("-1");
    EXPECT_TRUE(expr1->eval(testNode));  // Non-zero negative is true

    auto expr2 = compileExpression("-0");
    EXPECT_FALSE(expr2->eval(testNode)); // Negative zero is false

    auto expr3 = compileExpression("-42.5");
    EXPECT_TRUE(expr3->eval(testNode));  // Non-zero negative float is true

    // Test negative numbers in comparisons
    auto expr4 = compileExpression("-1 > -2");
    EXPECT_TRUE(expr4->eval(testNode));

    auto expr5 = compileExpression("-5 < 0");
    EXPECT_TRUE(expr5->eval(testNode));

    auto expr6 = compileExpression("x > -1");
    EXPECT_TRUE(expr6->eval(testNode));  // x is 10, so 10 > -1

    auto expr7 = compileExpression("-10 == -10");
    EXPECT_TRUE(expr7->eval(testNode));

    auto expr8 = compileExpression("-3.14 != -2.71");
    EXPECT_TRUE(expr8->eval(testNode));
}

TEST_F(ExpressionTest, StringLiterals) {
    auto expr1 = compileExpression("\"hello\"");
    EXPECT_TRUE(expr1->eval(testNode));  // Non-empty string is true

    auto expr2 = compileExpression("\"\"");
    EXPECT_FALSE(expr2->eval(testNode)); // Empty string is false
}

TEST_F(ExpressionTest, VariableAccess) {
    auto expr1 = compileExpression("x");
    EXPECT_TRUE(expr1->eval(testNode));  // x = 10, non-zero is true

    auto expr2 = compileExpression("flag");
    EXPECT_FALSE(expr2->eval(testNode)); // flag = false

    auto expr3 = compileExpression("message");
    EXPECT_TRUE(expr3->eval(testNode));  // message = "hello", non-empty is true
}

TEST_F(ExpressionTest, PropertyAccess) {
    auto expr1 = compileExpression("properties.health");
    EXPECT_TRUE(expr1->eval(testNode));  // health = 100, non-zero is true

    auto expr2 = compileExpression("properties.active");
    EXPECT_TRUE(expr2->eval(testNode));  // active = true

    auto expr3 = compileExpression("properties.name");
    EXPECT_TRUE(expr3->eval(testNode));  // name = "TestNode", non-empty is true
}

TEST_F(ExpressionTest, LogicalOperators) {
    auto expr1 = compileExpression("true && true");
    EXPECT_TRUE(expr1->eval(testNode));

    auto expr2 = compileExpression("true && false");
    EXPECT_FALSE(expr2->eval(testNode));

    auto expr3 = compileExpression("false || true");
    EXPECT_TRUE(expr3->eval(testNode));

    auto expr4 = compileExpression("false || false");
    EXPECT_FALSE(expr4->eval(testNode));
}

TEST_F(ExpressionTest, NotOperator) {
    auto expr1 = compileExpression("!true");
    EXPECT_FALSE(expr1->eval(testNode));

    auto expr2 = compileExpression("!false");
    EXPECT_TRUE(expr2->eval(testNode));

    auto expr3 = compileExpression("!!true");
    EXPECT_TRUE(expr3->eval(testNode));

    auto expr4 = compileExpression("!properties.active");
    EXPECT_FALSE(expr4->eval(testNode)); // active = true, so !true = false
}

TEST_F(ExpressionTest, ComparisonOperators) {
    auto expr1 = compileExpression("x > 5");
    EXPECT_TRUE(expr1->eval(testNode));  // 10 > 5

    auto expr2 = compileExpression("x < 5");
    EXPECT_FALSE(expr2->eval(testNode)); // 10 < 5

    auto expr3 = compileExpression("x >= 10");
    EXPECT_TRUE(expr3->eval(testNode));  // 10 >= 10

    auto expr4 = compileExpression("x <= 10");
    EXPECT_TRUE(expr4->eval(testNode));  // 10 <= 10

    auto expr5 = compileExpression("x == 10");
    EXPECT_TRUE(expr5->eval(testNode));  // 10 == 10

    auto expr6 = compileExpression("x != 10");
    EXPECT_FALSE(expr6->eval(testNode)); // 10 != 10
}

TEST_F(ExpressionTest, PropertyComparisons) {
    auto expr1 = compileExpression("properties.health > 50");
    EXPECT_TRUE(expr1->eval(testNode));  // 100 > 50

    auto expr2 = compileExpression("properties.score == 42.5");
    EXPECT_TRUE(expr2->eval(testNode));  // 42.5 == 42.5

    auto expr3 = compileExpression("properties.name == \"TestNode\"");
    EXPECT_TRUE(expr3->eval(testNode));  // "TestNode" == "TestNode"

    auto expr4 = compileExpression("properties.active == true");
    EXPECT_TRUE(expr4->eval(testNode));  // true == true
}

TEST_F(ExpressionTest, ComplexExpressions) {
    auto expr1 = compileExpression("x > 5 && properties.active");
    EXPECT_TRUE(expr1->eval(testNode));  // 10 > 5 && true

    auto expr2 = compileExpression("(x > 15) || (properties.health > 50)");
    EXPECT_TRUE(expr2->eval(testNode));  // false || true

    auto expr3 = compileExpression("!flag && properties.active");
    EXPECT_TRUE(expr3->eval(testNode));  // !false && true

    auto expr4 = compileExpression("x == y || properties.name != \"\"");
    EXPECT_TRUE(expr4->eval(testNode));  // false || true
}

TEST_F(ExpressionTest, Parentheses) {
    auto expr1 = compileExpression("(true)");
    EXPECT_TRUE(expr1->eval(testNode));

    auto expr2 = compileExpression("!(false)");
    EXPECT_TRUE(expr2->eval(testNode));

    auto expr3 = compileExpression("(x > 5) && (y > 15)");
    EXPECT_TRUE(expr3->eval(testNode));  // true && true
}

TEST_F(ExpressionTest, OperatorPrecedence) {
    // && has higher precedence than ||
    auto expr1 = compileExpression("false || true && false");
    EXPECT_FALSE(expr1->eval(testNode)); // false || (true && false) = false || false = false

    auto expr2 = compileExpression("true && false || true");
    EXPECT_TRUE(expr2->eval(testNode));  // (true && false) || true = false || true = true
}

TEST_F(ExpressionTest, NumericTypeConversion) {
    // Test int vs double comparison
    testNode.setVar("intVar", Value{42});
    testNode.setVar("doubleVar", Value{42.0});

    auto expr1 = compileExpression("intVar == doubleVar");
    EXPECT_TRUE(expr1->eval(testNode));  // 42 == 42.0 should be true

    auto expr2 = compileExpression("intVar == 42.0");
    EXPECT_TRUE(expr2->eval(testNode));  // 42 == 42.0 should be true
}

TEST_F(ExpressionTest, ErrorHandling) {
    // Unknown variable - should return false, not throw (in eval context)
    auto expr1 = compileExpression("unknownVar");
    EXPECT_FALSE(expr1->eval(testNode));  // Unknown variables evaluate to false

    // Unknown property - should return false, not throw (in eval context)
    auto expr2 = compileExpression("properties.unknownProp");
    EXPECT_FALSE(expr2->eval(testNode));  // Unknown properties evaluate to false

    // Invalid comparison (string with number) - should throw when extracting values
    testNode.setVar("stringVar", Value{std::string("not_a_number")});
    EXPECT_THROW({
        auto expr = compileExpression("stringVar > 5");
        expr->eval(testNode);
    }, std::runtime_error);
}

TEST_F(ExpressionTest, ParserErrorHandling) {
    // Missing closing parenthesis
    EXPECT_THROW({
        compileExpression("(true");
    }, std::runtime_error);

    // Invalid token in primary
    EXPECT_THROW({
        compileExpression("&&");
    }, std::runtime_error);

    // Unexpected end
    EXPECT_THROW({
        compileExpression("true &&");
    }, std::runtime_error);
}

// Parameterized tests for various boolean expressions
class BooleanExpressionTest : public ExpressionTest,
                              public ::testing::WithParamInterface<std::pair<std::string, bool>> {};

TEST_P(BooleanExpressionTest, EvaluateExpression) {
    const auto& [expression, expected] = GetParam();
    auto expr = compileExpression(expression);
    EXPECT_EQ(expr->eval(testNode), expected) << "Expression: " << expression;
}

INSTANTIATE_TEST_SUITE_P(
    VariousBooleanExpressions,
    BooleanExpressionTest,
    ::testing::Values(
        std::make_pair("true", true),
        std::make_pair("false", false),
        std::make_pair("!true", false),
        std::make_pair("!false", true),
        std::make_pair("true && true", true),
        std::make_pair("true && false", false),
        std::make_pair("false && true", false),
        std::make_pair("false && false", false),
        std::make_pair("true || true", true),
        std::make_pair("true || false", true),
        std::make_pair("false || true", true),
        std::make_pair("false || false", false),
        std::make_pair("x > 5", true),    // x = 10
        std::make_pair("x < 5", false),   // x = 10
        std::make_pair("y >= 20", true),  // y = 20
        std::make_pair("y <= 20", true),  // y = 20
        std::make_pair("properties.health == 100", true),
        std::make_pair("properties.active", true),
        std::make_pair("!flag", true),    // flag = false
        std::make_pair("message != \"\"", true), // message = "hello"
        std::make_pair("(x > 5) && properties.active", true),
        std::make_pair("(x < 5) || (y > 15)", true)
    )
);

// Performance tests
TEST_F(ExpressionTest, PerformanceTest) {
    auto expr = compileExpression("x > 5 && properties.health > 50 && properties.active");
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Evaluate expression many times
    bool result = true;
    for (int i = 0; i < 100000; ++i) {
        result = result && expr->eval(testNode);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    EXPECT_TRUE(result);
    
    // Performance check - should complete in reasonable time
    // This is just to ensure no major performance regression
    EXPECT_LT(duration.count(), 1000000); // Less than 1 second for 100k evaluations
}

// Edge cases and robustness tests
TEST_F(ExpressionTest, EdgeCases) {
    // Very long expression
    std::string longExpr = "true";
    for (int i = 0; i < 100; ++i) {
        longExpr += " && true";
    }
    auto expr1 = compileExpression(longExpr);
    EXPECT_TRUE(expr1->eval(testNode));

    // Deeply nested parentheses
    auto expr2 = compileExpression("((((true))))");
    EXPECT_TRUE(expr2->eval(testNode));

    // Multiple negations
    auto expr3 = compileExpression("!!!!!!true");
    EXPECT_TRUE(expr3->eval(testNode));

    // Zero value handling
    testNode.setVar("zero", Value{0});
    auto expr4 = compileExpression("zero");
    EXPECT_FALSE(expr4->eval(testNode));

    // Floating point precision
    testNode.setVar("float1", Value{0.1 + 0.2});
    testNode.setVar("float2", Value{0.3});
    // Note: Due to floating point precision, this might not be exactly equal
    // But our comparison should handle reasonable precision
}

// Test AST node creation and structure
TEST_F(ExpressionTest, ASTStructure) {
    // Test that expressions compile without throwing
    EXPECT_NO_THROW(compileExpression("true"));
    EXPECT_NO_THROW(compileExpression("x > 5"));
    EXPECT_NO_THROW(compileExpression("properties.health"));
    EXPECT_NO_THROW(compileExpression("!flag && (x > y || properties.active)"));
}