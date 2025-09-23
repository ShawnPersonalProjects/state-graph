#include <gtest/gtest.h>
#include <stdexcept>
#include <chrono>
#include <vector>
#include "graph/phase_edge.hpp"
#include "graph/node.hpp"
#include "graph/json.hpp"

// Test fixture for PhaseEdge tests
class PhaseEdgeTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test nodes for phase edge evaluation
        sourceNode.id = "source_node";
        sourceNode.vars["health"] = Value{75};
        sourceNode.vars["mana"] = Value{50};
        sourceNode.vars["level"] = Value{10};
        sourceNode.vars["phase_id"] = Value{std::string("phase1")};
        sourceNode.properties["canTransition"] = Value{true};
        sourceNode.properties["completed"] = Value{false};
        sourceNode.properties["score"] = Value{150};

        targetNode.id = "target_node";
        targetNode.vars["health"] = Value{100};
        targetNode.vars["mana"] = Value{100};
        targetNode.vars["phase_id"] = Value{std::string("phase2")};
    }

    DefaultNode sourceNode;
    DefaultNode targetNode;
};

// Basic PhaseEdge functionality tests
TEST_F(PhaseEdgeTest, BasicPhaseEdgeCreation) {
    DefaultPhaseEdge edge;
    edge.from = "phase1";
    edge.to = "phase2";
    edge.condition_expr = "true";
    edge.compiled = compileExpression("true");
    
    EXPECT_EQ(edge.from, "phase1");
    EXPECT_EQ(edge.to, "phase2");
    EXPECT_EQ(edge.condition_expr, "true");
    EXPECT_NE(edge.compiled, nullptr);
}

TEST_F(PhaseEdgeTest, PhaseEdgeEvaluation) {
    DefaultPhaseEdge edge;
    edge.from = "phase1";
    edge.to = "phase2";
    edge.condition_expr = "health > 50";
    edge.compiled = compileExpression("health > 50");
    
    // Should evaluate to true (health = 75)
    EXPECT_TRUE(edge.evaluate(sourceNode));
    
    // Modify health to make condition false
    sourceNode.setVar("health", Value{30});
    EXPECT_FALSE(edge.evaluate(sourceNode));
    
    // Test with null compiled expression
    DefaultPhaseEdge nullEdge;
    nullEdge.compiled = nullptr;
    EXPECT_FALSE(nullEdge.evaluate(sourceNode)); // Should default to false (different from regular edge)
}

TEST_F(PhaseEdgeTest, BooleanLiteralConditions) {
    // True condition
    DefaultPhaseEdge trueEdge;
    trueEdge.condition_expr = "true";
    trueEdge.compiled = compileExpression("true");
    EXPECT_TRUE(trueEdge.evaluate(sourceNode));

    // False condition
    DefaultPhaseEdge falseEdge;
    falseEdge.condition_expr = "false";
    falseEdge.compiled = compileExpression("false");
    EXPECT_FALSE(falseEdge.evaluate(sourceNode));
}

TEST_F(PhaseEdgeTest, VariableBasedConditions) {
    // Health-based transition
    DefaultPhaseEdge healthEdge;
    healthEdge.condition_expr = "health >= 75";
    healthEdge.compiled = compileExpression("health >= 75");
    EXPECT_TRUE(healthEdge.evaluate(sourceNode)); // health = 75

    // Mana-based transition
    DefaultPhaseEdge manaEdge;
    manaEdge.condition_expr = "mana < 60";
    manaEdge.compiled = compileExpression("mana < 60");
    EXPECT_TRUE(manaEdge.evaluate(sourceNode)); // mana = 50

    // Level-based transition
    DefaultPhaseEdge levelEdge;
    levelEdge.condition_expr = "level == 10";
    levelEdge.compiled = compileExpression("level == 10");
    EXPECT_TRUE(levelEdge.evaluate(sourceNode)); // level = 10
}

TEST_F(PhaseEdgeTest, PropertyBasedConditions) {
    // Property-based transitions
    DefaultPhaseEdge canTransitionEdge;
    canTransitionEdge.condition_expr = "properties.canTransition";
    canTransitionEdge.compiled = compileExpression("properties.canTransition");
    EXPECT_TRUE(canTransitionEdge.evaluate(sourceNode)); // canTransition = true

    DefaultPhaseEdge completedEdge;
    completedEdge.condition_expr = "!properties.completed";
    completedEdge.compiled = compileExpression("!properties.completed");
    EXPECT_TRUE(completedEdge.evaluate(sourceNode)); // completed = false

    DefaultPhaseEdge scoreEdge;
    scoreEdge.condition_expr = "properties.score > 100";
    scoreEdge.compiled = compileExpression("properties.score > 100");
    EXPECT_TRUE(scoreEdge.evaluate(sourceNode)); // score = 150
}

TEST_F(PhaseEdgeTest, ComplexConditions) {
    // Complex AND condition
    DefaultPhaseEdge complexAndEdge;
    complexAndEdge.condition_expr = "health > 50 && properties.canTransition";
    complexAndEdge.compiled = compileExpression("health > 50 && properties.canTransition");
    EXPECT_TRUE(complexAndEdge.evaluate(sourceNode)); // both conditions true

    // Complex OR condition
    DefaultPhaseEdge complexOrEdge;
    complexOrEdge.condition_expr = "health < 30 || mana >= 50";
    complexOrEdge.compiled = compileExpression("health < 30 || mana >= 50");
    EXPECT_TRUE(complexOrEdge.evaluate(sourceNode)); // second condition true

    // Multiple conditions with parentheses
    DefaultPhaseEdge parenthesesEdge;
    parenthesesEdge.condition_expr = "(health > 70 && mana > 40) || properties.completed";
    parenthesesEdge.compiled = compileExpression("(health > 70 && mana > 40) || properties.completed");
    EXPECT_TRUE(parenthesesEdge.evaluate(sourceNode)); // first group true

    // Negation with complex condition
    DefaultPhaseEdge negationEdge;
    negationEdge.condition_expr = "!(health < 50 && mana < 30)";
    negationEdge.compiled = compileExpression("!(health < 50 && mana < 30)");
    EXPECT_TRUE(negationEdge.evaluate(sourceNode)); // negated false = true
}

TEST_F(PhaseEdgeTest, PhaseIdBasedConditions) {
    // Test phase_id variable access
    DefaultPhaseEdge phaseIdEdge;
    phaseIdEdge.condition_expr = "phase_id == \"phase1\"";
    phaseIdEdge.compiled = compileExpression("phase_id == \"phase1\"");
    EXPECT_TRUE(phaseIdEdge.evaluate(sourceNode)); // phase_id = "phase1"

    // Different phase check
    DefaultPhaseEdge differentPhaseEdge;
    differentPhaseEdge.condition_expr = "phase_id != \"phase2\"";
    differentPhaseEdge.compiled = compileExpression("phase_id != \"phase2\"");
    EXPECT_TRUE(differentPhaseEdge.evaluate(sourceNode)); // phase_id = "phase1" != "phase2"

    // Phase-dependent complex condition
    DefaultPhaseEdge phaseComplexEdge;
    phaseComplexEdge.condition_expr = "phase_id == \"phase1\" && health > 50";
    phaseComplexEdge.compiled = compileExpression("phase_id == \"phase1\" && health > 50");
    EXPECT_TRUE(phaseComplexEdge.evaluate(sourceNode)); // both conditions true
}

TEST_F(PhaseEdgeTest, JsonDeserialization) {
    nlohmann::json phaseEdgeJson = R"({
        "from": "combat_phase",
        "to": "victory_phase",
        "condition": "health > 25 && properties.canTransition"
    })"_json;

    DefaultPhaseEdge edge = DefaultPhaseEdge::from_json(phaseEdgeJson);
    
    EXPECT_EQ(edge.from, "combat_phase");
    EXPECT_EQ(edge.to, "victory_phase");
    EXPECT_EQ(edge.condition_expr, "health > 25 && properties.canTransition");
    EXPECT_NE(edge.compiled, nullptr);
    
    // Test evaluation
    EXPECT_TRUE(edge.evaluate(sourceNode)); // health=75, canTransition=true
}

TEST_F(PhaseEdgeTest, JsonDeserializationSimple) {
    nlohmann::json simpleJson = R"({
        "from": "start",
        "to": "end",
        "condition": "true"
    })"_json;

    DefaultPhaseEdge edge = DefaultPhaseEdge::from_json(simpleJson);
    
    EXPECT_EQ(edge.from, "start");
    EXPECT_EQ(edge.to, "end");
    EXPECT_EQ(edge.condition_expr, "true");
    EXPECT_TRUE(edge.evaluate(sourceNode));
}

TEST_F(PhaseEdgeTest, JsonDeserializationError) {
    // Test missing required fields
    nlohmann::json invalidJson1 = R"({"from": "A", "condition": "true"})"_json;
    EXPECT_THROW(DefaultPhaseEdge::from_json(invalidJson1), nlohmann::json::exception);

    nlohmann::json invalidJson2 = R"({"to": "B", "condition": "true"})"_json;
    EXPECT_THROW(DefaultPhaseEdge::from_json(invalidJson2), nlohmann::json::exception);

    nlohmann::json invalidJson3 = R"({"from": "A", "to": "B"})"_json;
    EXPECT_THROW(DefaultPhaseEdge::from_json(invalidJson3), nlohmann::json::exception);
}

TEST_F(PhaseEdgeTest, ErrorHandling) {
    // Unknown variable should evaluate to false (graceful degradation)
    DefaultPhaseEdge unknownVarEdge;
    unknownVarEdge.condition_expr = "unknownVariable";
    unknownVarEdge.compiled = compileExpression("unknownVariable");
    EXPECT_FALSE(unknownVarEdge.evaluate(sourceNode));

    // Unknown property should evaluate to false
    DefaultPhaseEdge unknownPropEdge;
    unknownPropEdge.condition_expr = "properties.unknownProperty";
    unknownPropEdge.compiled = compileExpression("properties.unknownProperty");
    EXPECT_FALSE(unknownPropEdge.evaluate(sourceNode));

    // Invalid comparison should throw
    sourceNode.setVar("stringVar", Value{std::string("not_a_number")});
    DefaultPhaseEdge invalidCompEdge;
    invalidCompEdge.condition_expr = "stringVar > 5";
    invalidCompEdge.compiled = compileExpression("stringVar > 5");
    EXPECT_THROW(invalidCompEdge.evaluate(sourceNode), std::runtime_error);
}

TEST_F(PhaseEdgeTest, ExpressionCompilationErrors) {
    // Invalid expression syntax should throw during compilation
    EXPECT_THROW({
        DefaultPhaseEdge edge;
        edge.condition_expr = "invalid && )";
        edge.compiled = compileExpression("invalid && )");
    }, std::runtime_error);

    // Empty expression should throw
    EXPECT_THROW({
        DefaultPhaseEdge edge;
        edge.condition_expr = "";
        edge.compiled = compileExpression("");
    }, std::runtime_error);
}

// Parameterized tests for various phase transition conditions
class PhaseTransitionTest : public PhaseEdgeTest,
                           public ::testing::WithParamInterface<std::pair<std::string, bool>> {};

TEST_P(PhaseTransitionTest, EvaluateCondition) {
    auto [condition, expected] = GetParam();
    
    DefaultPhaseEdge edge;
    edge.condition_expr = condition;
    edge.compiled = compileExpression(condition);
    
    EXPECT_EQ(edge.evaluate(sourceNode), expected) << "Condition: " << condition;
}

INSTANTIATE_TEST_SUITE_P(
    VariousPhaseConditions,
    PhaseTransitionTest,
    ::testing::Values(
        // Basic literals
        std::make_pair("true", true),
        std::make_pair("false", false),
        
        // Variable comparisons (health=75, mana=50, level=10)
        std::make_pair("health > 50", true),
        std::make_pair("health < 50", false),
        std::make_pair("health >= 75", true),
        std::make_pair("health <= 75", true),
        std::make_pair("health == 75", true),
        std::make_pair("health != 75", false),
        std::make_pair("mana == 50", true),
        std::make_pair("level > 5", true),
        
        // Property access (canTransition=true, completed=false, score=150)
        std::make_pair("properties.canTransition", true),
        std::make_pair("!properties.canTransition", false),
        std::make_pair("properties.completed", false),
        std::make_pair("!properties.completed", true),
        std::make_pair("properties.score > 100", true),
        std::make_pair("properties.score < 100", false),
        
        // Phase ID tests (phase_id="phase1")
        std::make_pair("phase_id == \"phase1\"", true),
        std::make_pair("phase_id == \"phase2\"", false),
        std::make_pair("phase_id != \"phase2\"", true),
        
        // Complex logical operations
        std::make_pair("health > 50 && mana > 30", true),
        std::make_pair("health > 50 && mana > 60", false),
        std::make_pair("health < 50 || mana > 30", true),
        std::make_pair("health < 50 || mana < 30", false),
        
        // Mixed variable and property conditions
        std::make_pair("health > 70 && properties.canTransition", true),
        std::make_pair("health < 70 && properties.canTransition", false),
        std::make_pair("level >= 10 || properties.completed", true),
        
        // Parentheses and precedence
        std::make_pair("(health > 50) && (mana > 40)", true),
        std::make_pair("!(health < 50 || mana < 40)", true),
        std::make_pair("health > 50 && (mana > 60 || properties.canTransition)", true)
    )
);

// Performance tests
TEST_F(PhaseEdgeTest, PerformanceTest) {
    DefaultPhaseEdge edge;
    edge.condition_expr = "health > 50 && mana >= 25 && properties.canTransition && level >= 5";
    edge.compiled = compileExpression(edge.condition_expr);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Evaluate phase edge many times
    bool result = true;
    for (int i = 0; i < 50000; ++i) {
        result = result && edge.evaluate(sourceNode);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    EXPECT_TRUE(result);
    EXPECT_LT(duration.count(), 1000000); // Should complete in reasonable time (< 1 second)
}

TEST_F(PhaseEdgeTest, MassPhaseEdgeCreation) {
    auto start = std::chrono::high_resolution_clock::now();
    
    // Create many phase edges with different conditions
    std::vector<DefaultPhaseEdge> edges;
    for (int i = 0; i < 1000; ++i) {
        DefaultPhaseEdge edge;
        edge.from = "phase_" + std::to_string(i);
        edge.to = "phase_" + std::to_string(i + 1);
        edge.condition_expr = "health > " + std::to_string(i % 100);
        edge.compiled = compileExpression(edge.condition_expr);
        edges.push_back(std::move(edge));
    }
    
    // Evaluate all edges
    int trueCount = 0;
    for (const auto& edge : edges) {
        if (edge.evaluate(sourceNode)) {
            trueCount++;
        }
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    EXPECT_GT(trueCount, 0);
    EXPECT_LT(duration.count(), 1000); // Should complete in reasonable time (< 1 second)
}

// Edge cases and robustness tests
TEST_F(PhaseEdgeTest, EdgeCases) {
    // Very long condition expression
    std::string longCondition = "true";
    for (int i = 0; i < 50; ++i) {
        longCondition += " && true";
    }
    DefaultPhaseEdge longEdge;
    longEdge.condition_expr = longCondition;
    longEdge.compiled = compileExpression(longCondition);
    EXPECT_TRUE(longEdge.evaluate(sourceNode));

    // Deeply nested parentheses
    DefaultPhaseEdge nestedEdge;
    nestedEdge.condition_expr = "((((health > 50))))";
    nestedEdge.compiled = compileExpression("((((health > 50))))");
    EXPECT_TRUE(nestedEdge.evaluate(sourceNode));

    // Multiple negations
    DefaultPhaseEdge multiNegEdge;
    multiNegEdge.condition_expr = "!!!!!!false";
    multiNegEdge.compiled = compileExpression("!!!!!!false");
    EXPECT_FALSE(multiNegEdge.evaluate(sourceNode));

    // Empty phase IDs
    DefaultPhaseEdge emptyPhaseEdge;
    emptyPhaseEdge.from = "";
    emptyPhaseEdge.to = "";
    emptyPhaseEdge.condition_expr = "true";
    emptyPhaseEdge.compiled = compileExpression("true");
    EXPECT_TRUE(emptyPhaseEdge.evaluate(sourceNode));

    // Very long phase IDs
    std::string longPhaseId(1000, 'p');
    DefaultPhaseEdge longPhaseEdge;
    longPhaseEdge.from = longPhaseId;
    longPhaseEdge.to = longPhaseId + "_next";
    longPhaseEdge.condition_expr = "true";
    longPhaseEdge.compiled = compileExpression("true");
    EXPECT_TRUE(longPhaseEdge.evaluate(sourceNode));
}

TEST_F(PhaseEdgeTest, SpecialValueHandling) {
    // Zero values
    sourceNode.setVar("zero", Value{0});
    DefaultPhaseEdge zeroEdge;
    zeroEdge.condition_expr = "zero == 0";
    zeroEdge.compiled = compileExpression("zero == 0");
    EXPECT_TRUE(zeroEdge.evaluate(sourceNode));

    // Negative values
    sourceNode.setVar("negative", Value{-42});
    DefaultPhaseEdge negativeEdge;
    negativeEdge.condition_expr = "negative < 0";
    negativeEdge.compiled = compileExpression("negative < 0");
    EXPECT_TRUE(negativeEdge.evaluate(sourceNode));

    // Floating point values
    sourceNode.setVar("float", Value{3.14159});
    DefaultPhaseEdge floatEdge;
    floatEdge.condition_expr = "float > 3.0";
    floatEdge.compiled = compileExpression("float > 3.0");
    EXPECT_TRUE(floatEdge.evaluate(sourceNode));

    // Empty string values
    sourceNode.setVar("emptyString", Value{std::string("")});
    DefaultPhaseEdge emptyStringEdge;
    emptyStringEdge.condition_expr = "emptyString == \"\"";
    emptyStringEdge.compiled = compileExpression("emptyString == \"\"");
    EXPECT_TRUE(emptyStringEdge.evaluate(sourceNode));
}

// Integration tests with realistic phase transition scenarios
TEST_F(PhaseEdgeTest, RealisticPhaseTransitions) {
    // Game phase transitions
    
    // Tutorial to Main Game
    DefaultPhaseEdge tutorialToMain;
    tutorialToMain.from = "tutorial";
    tutorialToMain.to = "main_game";
    tutorialToMain.condition_expr = "properties.completed && level >= 2";
    tutorialToMain.compiled = compileExpression(tutorialToMain.condition_expr);
    
    sourceNode.setVar("level", Value{2});
    sourceNode.properties["completed"] = Value{true};
    EXPECT_TRUE(tutorialToMain.evaluate(sourceNode));

    // Combat to Victory
    DefaultPhaseEdge combatToVictory;
    combatToVictory.from = "combat";
    combatToVictory.to = "victory";
    combatToVictory.condition_expr = "health > 0 && properties.enemiesDefeated";
    combatToVictory.compiled = compileExpression(combatToVictory.condition_expr);
    
    sourceNode.properties["enemiesDefeated"] = Value{true};
    EXPECT_TRUE(combatToVictory.evaluate(sourceNode));

    // Combat to Defeat
    DefaultPhaseEdge combatToDefeat;
    combatToDefeat.from = "combat";
    combatToDefeat.to = "defeat";
    combatToDefeat.condition_expr = "health <= 0";
    combatToDefeat.compiled = compileExpression(combatToDefeat.condition_expr);
    
    sourceNode.setVar("health", Value{0});
    EXPECT_TRUE(combatToDefeat.evaluate(sourceNode));

    // Level progression
    DefaultPhaseEdge levelProgression;
    levelProgression.from = "level_1";
    levelProgression.to = "level_2";
    levelProgression.condition_expr = "properties.score >= 1000 && properties.completed";
    levelProgression.compiled = compileExpression(levelProgression.condition_expr);
    
    sourceNode.properties["score"] = Value{1500};
    sourceNode.properties["completed"] = Value{true};
    EXPECT_TRUE(levelProgression.evaluate(sourceNode));
}