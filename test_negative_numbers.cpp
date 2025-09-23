#include <iostream>
#include "../include/graph/expression.hpp"
#include "../include/graph/node.hpp"

int main() {
    std::cout << "Testing negative number support in expressions...\n";
    
    // Create a test node
    DefaultNode node;
    node.id = "test";
    node.setVar("x", Value(5));
    
    // Test various negative number expressions
    std::vector<std::string> expressions = {
        "-1 > 0",           // Should be false
        "-5 < 0",           // Should be true  
        "x > -1",           // Should be true (5 > -1)
        "x == -5",          // Should be false
        "-10 != -5",        // Should be true
        "-3.14 < 0",        // Should be true
        "-0 == 0"           // Should be true
    };
    
    std::vector<bool> expected = {false, true, true, false, true, true, true};
    
    bool allPassed = true;
    for (size_t i = 0; i < expressions.size(); ++i) {
        try {
            auto expr = compileExpression(expressions[i]);
            bool result = expr->eval(node);
            
            std::cout << "Expression: \"" << expressions[i] << "\" = " 
                      << (result ? "true" : "false");
            
            if (result == expected[i]) {
                std::cout << " ✓ PASS\n";
            } else {
                std::cout << " ✗ FAIL (expected " 
                          << (expected[i] ? "true" : "false") << ")\n";
                allPassed = false;
            }
        } catch (const std::exception& e) {
            std::cout << "Expression: \"" << expressions[i] << "\" = ERROR: " 
                      << e.what() << " ✗ FAIL\n";
            allPassed = false;
        }
    }
    
    std::cout << "\nOverall result: " << (allPassed ? "ALL TESTS PASSED!" : "SOME TESTS FAILED!") << "\n";
    
    return allPassed ? 0 : 1;
}