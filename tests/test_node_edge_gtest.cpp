#include <gtest/gtest.h>
#include <stdexcept>
#include <sstream>
#include <cmath>
#include <chrono>
#include <vector>
#include <limits>
#include "graph/node.hpp"
#include "graph/edge.hpp"
#include "graph/json.hpp"

// Test fixture for Node tests
class NodeTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test nodes with various configurations
        testNode1.id = "node1";
        testNode1.params["maxHealth"] = Value{100};
        testNode1.params["type"] = Value{std::string("player")};
        testNode1.vars["health"] = Value{80};
        testNode1.vars["experience"] = Value{1500};
        testNode1.properties["active"] = Value{true};
        testNode1.properties["level"] = Value{5};

        testNode2.id = "node2";
        testNode2.params["difficulty"] = Value{3.5};
        testNode2.vars["score"] = Value{0};
        testNode2.vars["completed"] = Value{false};
        testNode2.properties["name"] = Value{std::string("TestLevel")};
    }

    DefaultNode testNode1;
    DefaultNode testNode2;
};

// Test fixture for Edge tests
class EdgeTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test nodes for edge evaluation
        sourceNode.id = "source";
        sourceNode.vars["health"] = Value{75};
        sourceNode.vars["mana"] = Value{50};
        sourceNode.properties["canCast"] = Value{true};
        sourceNode.properties["level"] = Value{10};

        targetNode.id = "target";
        targetNode.vars["health"] = Value{100};
        targetNode.vars["mana"] = Value{100};
    }

    DefaultNode sourceNode;
    DefaultNode targetNode;
};

// Helper function tests
TEST(ValueToStringTest, AllValueTypes) {
    EXPECT_EQ(valueToString(Value{true}), "true");
    EXPECT_EQ(valueToString(Value{false}), "false");
    EXPECT_EQ(valueToString(Value{static_cast<int64_t>(42)}), "42");
    EXPECT_EQ(valueToString(Value{-17.5}), "-17.500000");
    EXPECT_EQ(valueToString(Value{std::string("hello")}), "\"hello\"");
}

// DefaultNode basic functionality tests
TEST_F(NodeTest, BasicNodeCreation) {
    DefaultNode node;
    node.id = "test";
    EXPECT_EQ(node.id, "test");
    EXPECT_TRUE(node.params.empty());
    EXPECT_TRUE(node.vars.empty());
    EXPECT_TRUE(node.properties.empty());
}

TEST_F(NodeTest, ParameterOperations) {
    // Test hasParam and getParam
    EXPECT_TRUE(testNode1.hasParam("maxHealth"));
    EXPECT_FALSE(testNode1.hasParam("nonexistent"));

    const Value* healthParam = testNode1.getParam("maxHealth");
    ASSERT_NE(healthParam, nullptr);
    EXPECT_TRUE(std::holds_alternative<int64_t>(*healthParam));
    EXPECT_EQ(std::get<int64_t>(*healthParam), 100);

    const Value* typeParam = testNode1.getParam("type");
    ASSERT_NE(typeParam, nullptr);
    EXPECT_TRUE(std::holds_alternative<std::string>(*typeParam));
    EXPECT_EQ(std::get<std::string>(*typeParam), "player");

    // Test non-existent parameter
    const Value* nonexistent = testNode1.getParam("nonexistent");
    EXPECT_EQ(nonexistent, nullptr);
}

TEST_F(NodeTest, VariableOperations) {
    // Test hasVar and getVar
    EXPECT_TRUE(testNode1.hasVar("health"));
    EXPECT_FALSE(testNode1.hasVar("nonexistent"));

    const Value* healthVar = testNode1.getVar("health");
    ASSERT_NE(healthVar, nullptr);
    EXPECT_TRUE(std::holds_alternative<int64_t>(*healthVar));
    EXPECT_EQ(std::get<int64_t>(*healthVar), 80);

    // Test setVar
    testNode1.setVar("newVar", Value{42.5});
    EXPECT_TRUE(testNode1.hasVar("newVar"));
    const Value* newVar = testNode1.getVar("newVar");
    ASSERT_NE(newVar, nullptr);
    EXPECT_TRUE(std::holds_alternative<double>(*newVar));
    EXPECT_EQ(std::get<double>(*newVar), 42.5);

    // Test variable modification
    testNode1.setVar("health", Value{90});
    const Value* modifiedHealth = testNode1.getVar("health");
    ASSERT_NE(modifiedHealth, nullptr);
    EXPECT_EQ(std::get<int64_t>(*modifiedHealth), 90);

    // Test non-existent variable
    const Value* nonexistent = testNode1.getVar("nonexistent");
    EXPECT_EQ(nonexistent, nullptr);
}

TEST_F(NodeTest, PropertyOperations) {
    // Test hasProperty and getProperty
    EXPECT_TRUE(testNode1.hasProperty("active"));
    EXPECT_FALSE(testNode1.hasProperty("nonexistent"));

    const Value* activeProperty = testNode1.getProperty("active");
    ASSERT_NE(activeProperty, nullptr);
    EXPECT_TRUE(std::holds_alternative<bool>(*activeProperty));
    EXPECT_EQ(std::get<bool>(*activeProperty), true);

    const Value* levelProperty = testNode1.getProperty("level");
    ASSERT_NE(levelProperty, nullptr);
    EXPECT_TRUE(std::holds_alternative<int64_t>(*levelProperty));
    EXPECT_EQ(std::get<int64_t>(*levelProperty), 5);

    // Test non-existent property
    const Value* nonexistent = testNode1.getProperty("nonexistent");
    EXPECT_EQ(nonexistent, nullptr);
}

TEST_F(NodeTest, ParseValueFunction) {
    // Test parsing different JSON value types
    nlohmann::json intJson = 42;
    Value intValue = DefaultNode::parseValue(intJson);
    EXPECT_TRUE(std::holds_alternative<int64_t>(intValue));
    EXPECT_EQ(std::get<int64_t>(intValue), 42);

    nlohmann::json doubleJson = 3.14;
    Value doubleValue = DefaultNode::parseValue(doubleJson);
    EXPECT_TRUE(std::holds_alternative<double>(doubleValue));
    EXPECT_DOUBLE_EQ(std::get<double>(doubleValue), 3.14);

    nlohmann::json boolJson = true;
    Value boolValue = DefaultNode::parseValue(boolJson);
    EXPECT_TRUE(std::holds_alternative<bool>(boolValue));
    EXPECT_EQ(std::get<bool>(boolValue), true);

    nlohmann::json stringJson = "test";
    Value stringValue = DefaultNode::parseValue(stringJson);
    EXPECT_TRUE(std::holds_alternative<std::string>(stringValue));
    EXPECT_EQ(std::get<std::string>(stringValue), "test");

    // Test unsupported type
    nlohmann::json arrayJson = nlohmann::json::array();
    EXPECT_THROW(DefaultNode::parseValue(arrayJson), std::runtime_error);
}

TEST_F(NodeTest, JsonDeserialization) {
    nlohmann::json nodeJson = R"({
        "id": "testNode",
        "params": {
            "maxHealth": 100,
            "type": "enemy"
        },
        "vars": {
            "health": 80,
            "speed": 2.5
        },
        "properties": {
            "aggressive": true,
            "level": 3
        }
    })"_json;

    DefaultNode node = DefaultNode::from_json(nodeJson);
    
    EXPECT_EQ(node.id, "testNode");
    
    // Check params
    EXPECT_TRUE(node.hasParam("maxHealth"));
    EXPECT_EQ(std::get<int64_t>(*node.getParam("maxHealth")), 100);
    EXPECT_TRUE(node.hasParam("type"));
    EXPECT_EQ(std::get<std::string>(*node.getParam("type")), "enemy");
    
    // Check vars
    EXPECT_TRUE(node.hasVar("health"));
    EXPECT_EQ(std::get<int64_t>(*node.getVar("health")), 80);
    EXPECT_TRUE(node.hasVar("speed"));
    EXPECT_EQ(std::get<double>(*node.getVar("speed")), 2.5);
    
    // Check properties
    EXPECT_TRUE(node.hasProperty("aggressive"));
    EXPECT_EQ(std::get<bool>(*node.getProperty("aggressive")), true);
    EXPECT_TRUE(node.hasProperty("level"));
    EXPECT_EQ(std::get<int64_t>(*node.getProperty("level")), 3);
}

TEST_F(NodeTest, JsonDeserializationMinimal) {
    // Test with minimal JSON (only id required)
    nlohmann::json minimalJson = R"({"id": "minimal"})"_json;
    DefaultNode node = DefaultNode::from_json(minimalJson);
    
    EXPECT_EQ(node.id, "minimal");
    EXPECT_TRUE(node.params.empty());
    EXPECT_TRUE(node.vars.empty());
    EXPECT_TRUE(node.properties.empty());
}

TEST_F(NodeTest, JsonDeserializationError) {
    // Test missing required field
    nlohmann::json invalidJson = R"({"params": {}})"_json;
    EXPECT_THROW(DefaultNode::from_json(invalidJson), nlohmann::json::exception);
}

TEST_F(NodeTest, PrintFunction) {
    // Test print function (mainly for coverage)
    // Redirect cout to string stream
    std::stringstream buffer;
    std::streambuf* orig = std::cout.rdbuf(buffer.rdbuf());
    
    testNode1.print();
    
    std::cout.rdbuf(orig);
    std::string output = buffer.str();
    
    EXPECT_TRUE(output.find("Node 'node1'") != std::string::npos);
    EXPECT_TRUE(output.find("maxHealth=100") != std::string::npos);
    EXPECT_TRUE(output.find("health=80") != std::string::npos);
    EXPECT_TRUE(output.find("active=true") != std::string::npos);
}

TEST_F(NodeTest, PrintEmptyNode) {
    DefaultNode emptyNode;
    emptyNode.id = "empty";
    
    std::stringstream buffer;
    std::streambuf* orig = std::cout.rdbuf(buffer.rdbuf());
    
    emptyNode.print();
    
    std::cout.rdbuf(orig);
    std::string output = buffer.str();
    
    EXPECT_TRUE(output.find("Node 'empty'") != std::string::npos);
    EXPECT_TRUE(output.find("(none)") != std::string::npos);
}

// DefaultEdge tests
TEST_F(EdgeTest, BasicEdgeCreation) {
    DefaultEdge edge;
    edge.from = "A";
    edge.to = "B";
    edge.condition_expr = "true";
    edge.compiled = compileExpression("true");
    
    EXPECT_EQ(edge.from, "A");
    EXPECT_EQ(edge.to, "B");
    EXPECT_EQ(edge.condition_expr, "true");
    EXPECT_NE(edge.compiled, nullptr);
    EXPECT_TRUE(edge.actions.empty());
}

TEST_F(EdgeTest, EdgeEvaluation) {
    DefaultEdge edge;
    edge.from = "source";
    edge.to = "target";
    edge.condition_expr = "health > 50";
    edge.compiled = compileExpression("health > 50");
    
    // Should evaluate to true (health = 75)
    EXPECT_TRUE(edge.evaluate(sourceNode));
    
    // Modify health to make condition false
    sourceNode.setVar("health", Value{30});
    EXPECT_FALSE(edge.evaluate(sourceNode));
    
    // Test with null compiled expression
    DefaultEdge nullEdge;
    nullEdge.compiled = nullptr;
    EXPECT_TRUE(nullEdge.evaluate(sourceNode)); // Should default to true
}

TEST_F(EdgeTest, ComplexConditions) {
    DefaultEdge edge;
    edge.condition_expr = "health > 50 && properties.canCast";
    edge.compiled = compileExpression(edge.condition_expr);
    
    EXPECT_TRUE(edge.evaluate(sourceNode)); // health=75, canCast=true
    
    // Change canCast to false
    sourceNode.properties["canCast"] = Value{false};
    EXPECT_FALSE(edge.evaluate(sourceNode)); // health=75, canCast=false
}

TEST_F(EdgeTest, EdgeParseValue) {
    // Test parsing different action value types
    nlohmann::json intJson = 42;
    Value intValue = DefaultEdge::parseValue(intJson);
    EXPECT_TRUE(std::holds_alternative<int64_t>(intValue));
    EXPECT_EQ(std::get<int64_t>(intValue), 42);

    nlohmann::json doubleJson = 3.14;
    Value doubleValue = DefaultEdge::parseValue(doubleJson);
    EXPECT_TRUE(std::holds_alternative<double>(doubleValue));
    EXPECT_DOUBLE_EQ(std::get<double>(doubleValue), 3.14);

    nlohmann::json boolJson = false;
    Value boolValue = DefaultEdge::parseValue(boolJson);
    EXPECT_TRUE(std::holds_alternative<bool>(boolValue));
    EXPECT_EQ(std::get<bool>(boolValue), false);

    nlohmann::json stringJson = "action";
    Value stringValue = DefaultEdge::parseValue(stringJson);
    EXPECT_TRUE(std::holds_alternative<std::string>(stringValue));
    EXPECT_EQ(std::get<std::string>(stringValue), "action");

    // Test unsupported type
    nlohmann::json objectJson = nlohmann::json::object();
    EXPECT_THROW(DefaultEdge::parseValue(objectJson), std::runtime_error);
}

TEST_F(EdgeTest, JsonDeserialization) {
    nlohmann::json edgeJson = R"({
        "from": "nodeA",
        "to": "nodeB",
        "condition": "health > 25 && mana >= 10",
        "actions": {
            "damage": 10,
            "status": "attacked",
            "critical": false
        }
    })"_json;

    DefaultEdge edge = DefaultEdge::from_json(edgeJson);
    
    EXPECT_EQ(edge.from, "nodeA");
    EXPECT_EQ(edge.to, "nodeB");
    EXPECT_EQ(edge.condition_expr, "health > 25 && mana >= 10");
    EXPECT_NE(edge.compiled, nullptr);
    
    // Check actions
    EXPECT_EQ(edge.actions.size(), 3);
    EXPECT_TRUE(edge.actions.count("damage"));
    EXPECT_EQ(std::get<int64_t>(edge.actions["damage"]), 10);
    EXPECT_TRUE(edge.actions.count("status"));
    EXPECT_EQ(std::get<std::string>(edge.actions["status"]), "attacked");
    EXPECT_TRUE(edge.actions.count("critical"));
    EXPECT_EQ(std::get<bool>(edge.actions["critical"]), false);
    
    // Test evaluation
    EXPECT_TRUE(edge.evaluate(sourceNode)); // health=75, mana=50
}

TEST_F(EdgeTest, JsonDeserializationNoActions) {
    nlohmann::json edgeJson = R"({
        "from": "start",
        "to": "end",
        "condition": "true"
    })"_json;

    DefaultEdge edge = DefaultEdge::from_json(edgeJson);
    
    EXPECT_EQ(edge.from, "start");
    EXPECT_EQ(edge.to, "end");
    EXPECT_EQ(edge.condition_expr, "true");
    EXPECT_TRUE(edge.actions.empty());
    EXPECT_TRUE(edge.evaluate(sourceNode));
}

TEST_F(EdgeTest, JsonDeserializationError) {
    // Test missing required fields
    nlohmann::json invalidJson1 = R"({"from": "A", "condition": "true"})"_json;
    EXPECT_THROW(DefaultEdge::from_json(invalidJson1), nlohmann::json::exception);

    nlohmann::json invalidJson2 = R"({"to": "B", "condition": "true"})"_json;
    EXPECT_THROW(DefaultEdge::from_json(invalidJson2), nlohmann::json::exception);

    nlohmann::json invalidJson3 = R"({"from": "A", "to": "B"})"_json;
    EXPECT_THROW(DefaultEdge::from_json(invalidJson3), nlohmann::json::exception);
}

TEST_F(EdgeTest, PrintFunction) {
    DefaultEdge edge;
    edge.from = "source";
    edge.to = "target";
    edge.condition_expr = "health > 50";
    edge.actions["heal"] = Value{20};
    edge.actions["boost"] = Value{std::string("speed")};
    
    std::stringstream buffer;
    std::streambuf* orig = std::cout.rdbuf(buffer.rdbuf());
    
    edge.print();
    
    std::cout.rdbuf(orig);
    std::string output = buffer.str();
    
    EXPECT_TRUE(output.find("Edge: source -> target") != std::string::npos);
    EXPECT_TRUE(output.find("Condition: health > 50") != std::string::npos);
    EXPECT_TRUE(output.find("heal=20") != std::string::npos);
    EXPECT_TRUE(output.find("boost=\"speed\"") != std::string::npos);
}

TEST_F(EdgeTest, PrintEmptyActions) {
    DefaultEdge edge;
    edge.from = "A";
    edge.to = "B";
    edge.condition_expr = "true";
    
    std::stringstream buffer;
    std::streambuf* orig = std::cout.rdbuf(buffer.rdbuf());
    
    edge.print();
    
    std::cout.rdbuf(orig);
    std::string output = buffer.str();
    
    EXPECT_TRUE(output.find("Edge: A -> B") != std::string::npos);
    EXPECT_TRUE(output.find("Actions: (none)") != std::string::npos);
}

// Parameterized tests for various node configurations
class NodeParameterTest : public ::testing::TestWithParam<std::tuple<std::string, Value, std::string>> {};

TEST_P(NodeParameterTest, ParameterAccess) {
    auto [key, value, type] = GetParam();
    
    DefaultNode node;
    node.id = "test";
    node.params[key] = value;
    
    EXPECT_TRUE(node.hasParam(key));
    const Value* retrieved = node.getParam(key);
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(*retrieved, value);
}

INSTANTIATE_TEST_SUITE_P(
    VariousParameters,
    NodeParameterTest,
    ::testing::Values(
        std::make_tuple("health", Value{100}, "int"),
        std::make_tuple("speed", Value{2.5}, "double"),
        std::make_tuple("active", Value{true}, "bool"),
        std::make_tuple("name", Value{std::string("test")}, "string"),
        std::make_tuple("level", Value{static_cast<int64_t>(5)}, "int64"),
        std::make_tuple("visible", Value{false}, "bool"),
        std::make_tuple("description", Value{std::string("")}, "empty_string")
    )
);

// Parameterized tests for edge conditions
class EdgeConditionTest : public EdgeTest,
                          public ::testing::WithParamInterface<std::pair<std::string, bool>> {};

TEST_P(EdgeConditionTest, EvaluateCondition) {
    auto [condition, expected] = GetParam();
    
    DefaultEdge edge;
    edge.condition_expr = condition;
    edge.compiled = compileExpression(condition);
    
    EXPECT_EQ(edge.evaluate(sourceNode), expected) << "Condition: " << condition;
}

INSTANTIATE_TEST_SUITE_P(
    VariousConditions,
    EdgeConditionTest,
    ::testing::Values(
        std::make_pair("true", true),
        std::make_pair("false", false),
        std::make_pair("health > 50", true),    // health = 75
        std::make_pair("health < 50", false),   // health = 75
        std::make_pair("mana == 50", true),     // mana = 50
        std::make_pair("properties.canCast", true),  // canCast = true
        std::make_pair("properties.level > 5", true),   // level = 10
        std::make_pair("health > 50 && mana >= 50", true),
        std::make_pair("health < 50 || mana > 40", true),
        std::make_pair("!properties.canCast", false)
    )
);

// Performance and stress tests
TEST_F(NodeTest, PerformanceTest) {
    auto start = std::chrono::high_resolution_clock::now();
    
    // Create and manipulate many nodes
    std::vector<DefaultNode> nodes;
    for (int i = 0; i < 1000; ++i) {
        DefaultNode node;
        node.id = "node_" + std::to_string(i);
        node.params["index"] = Value{static_cast<int64_t>(i)};
        node.vars["value"] = Value{i * 2.5};
        node.properties["active"] = Value{i % 2 == 0};
        nodes.push_back(std::move(node));
    }
    
    // Access all elements
    int sum = 0;
    for (const auto& node : nodes) {
        if (node.hasVar("value")) {
            sum += static_cast<int>(std::get<double>(*node.getVar("value")));
        }
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    EXPECT_GT(sum, 0);
    EXPECT_LT(duration.count(), 100000); // Should complete in reasonable time
}

TEST_F(EdgeTest, PerformanceTest) {
    DefaultEdge edge;
    edge.condition_expr = "health > 50 && mana >= 25 && properties.canCast";
    edge.compiled = compileExpression(edge.condition_expr);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Evaluate edge many times
    bool result = true;
    for (int i = 0; i < 10000; ++i) {
        result = result && edge.evaluate(sourceNode);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    EXPECT_TRUE(result);
    EXPECT_LT(duration.count(), 500000); // Should complete in reasonable time
}

// Edge cases and robustness tests
TEST_F(NodeTest, EdgeCases) {
    DefaultNode node;
    
    // Empty ID
    node.id = "";
    EXPECT_EQ(node.id, "");
    
    // Very long ID
    std::string longId(1000, 'x');
    node.id = longId;
    EXPECT_EQ(node.id, longId);
    
    // Unicode in strings
    node.params["unicode"] = Value{std::string("🎮🚀⭐")};
    const Value* unicode = node.getParam("unicode");
    ASSERT_NE(unicode, nullptr);
    EXPECT_EQ(std::get<std::string>(*unicode), "🎮🚀⭐");
    
    // Large numbers
    node.vars["large"] = Value{static_cast<int64_t>(9223372036854775807LL)};
    const Value* large = node.getVar("large");
    ASSERT_NE(large, nullptr);
    EXPECT_EQ(std::get<int64_t>(*large), 9223372036854775807LL);
    
    // Special float values
    node.vars["infinity"] = Value{std::numeric_limits<double>::infinity()};
    const Value* inf = node.getVar("infinity");
    ASSERT_NE(inf, nullptr);
    EXPECT_TRUE(std::isinf(std::get<double>(*inf)));
}

TEST_F(EdgeTest, EdgeCases) {
    // Edge with empty condition
    DefaultEdge edge;
    edge.condition_expr = "";
    
    // This should throw when compiling empty expression
    EXPECT_THROW(compileExpression(""), std::runtime_error);
    
    // Very long condition
    std::string longCondition = "true";
    for (int i = 0; i < 100; ++i) {
        longCondition += " && true";
    }
    edge.condition_expr = longCondition;
    edge.compiled = compileExpression(longCondition);
    EXPECT_TRUE(edge.evaluate(sourceNode));
    
    // Many actions
    for (int i = 0; i < 100; ++i) {
        edge.actions["action_" + std::to_string(i)] = Value{i};
    }
    EXPECT_EQ(edge.actions.size(), 100);
}