#include <gtest/gtest.h>
#include "../include/graph/multi_phase_state_graph.hpp"
#include "../include/graph/node.hpp"
#include "../include/graph/edge.hpp"
#include "../include/graph/phase_edge.hpp"
#include "../include/graph/expression.hpp"
#include "../include/graph/value.hpp"
#include <fstream>
#include <chrono>

class MultiPhaseStateGraphTest : public ::testing::Test {
protected:
    using TestGraph = MultiPhaseStateGraph<DefaultNode, DefaultEdge, DefaultPhaseEdge>;
    TestGraph multiGraph;
    
    void SetUp() override {
        multiGraph.clear();
    }
    
    // Helper function to create nodes with properties
    DefaultNode createNode(const std::string& id, 
                          const std::unordered_map<std::string, Value>& vars = {}) {
        DefaultNode node;
        node.id = id;
        for (const auto& [key, value] : vars) {
            node.setVar(key, value);
        }
        return node;
    }
    
    // Helper function to create edges
    DefaultEdge createEdge(const std::string& from, const std::string& to, 
                          const std::string& condition = "true",
                          const std::unordered_map<std::string, Value>& actions = {}) {
        DefaultEdge edge;
        edge.from = from;
        edge.to = to;
        edge.condition_expr = condition;
        edge.compiled = compileExpression(condition);
        edge.actions = actions;
        return edge;
    }
    
    // Helper function to create phase edges
    DefaultPhaseEdge createPhaseEdge(const std::string& from, const std::string& to,
                                   const std::string& condition = "true") {
        DefaultPhaseEdge edge;
        edge.from = from;
        edge.to = to;
        edge.condition_expr = condition;
        edge.compiled = compileExpression(condition);
        return edge;
    }
    
    // Helper to add a complete phase to the graph
    void addPhase(const std::string& phaseId, 
                 const std::vector<DefaultNode>& nodes,
                 std::vector<DefaultEdge>&& edges,
                 const std::string& initialState = "") {
        TestGraph::Phase phase;
        phase.id = phaseId;
        phase.initial_state = initialState;
        
        for (const auto& node : nodes) {
            phase.graph.addNode(node);
        }
        for (auto& edge : edges) {
            phase.graph.addEdge(std::move(edge));
        }
        
        if (!initialState.empty()) {
            phase.graph.setInitialState(initialState);
        }
        
        // Manually add to multiGraph (since there's no public addPhase method)
        // We'll use JSON loading for most tests
    }
};

TEST_F(MultiPhaseStateGraphTest, BasicGraphCreation) {
    // Test that a new graph has no current phase
    EXPECT_THROW(multiGraph.currentPhaseId(), std::runtime_error);
    EXPECT_THROW(multiGraph.currentStateId(), std::runtime_error);
    EXPECT_THROW(multiGraph.currentNode(), std::runtime_error);
}

TEST_F(MultiPhaseStateGraphTest, LoadSimpleJSON) {
    // Create a simple JSON config
    std::string jsonContent = R"({
        "phases": [
            {
                "id": "phase1",
                "initial_state": "start",
                "nodes": [
                    {"id": "start", "vars": {}},
                    {"id": "end", "vars": {}}
                ],
                "edges": [
                    {"from": "start", "to": "end", "condition": "true", "actions": {}}
                ]
            }
        ]
    })";
    
    std::ofstream tmpFile("test_multi_config.json");
    tmpFile << jsonContent;
    tmpFile.close();
    
    EXPECT_TRUE(multiGraph.loadFromJson("test_multi_config.json"));
    
    // Check that we have a current phase and state
    EXPECT_EQ(multiGraph.currentPhaseId(), "phase1");
    EXPECT_EQ(multiGraph.currentStateId(), "start");
    
    // Clean up
    std::remove("test_multi_config.json");
}

TEST_F(MultiPhaseStateGraphTest, LoadMultiplePhases) {
    std::string jsonContent = R"({
        "phases": [
            {
                "id": "intro",
                "initial_state": "welcome",
                "nodes": [
                    {"id": "welcome", "vars": {}},
                    {"id": "ready", "vars": {}}
                ],
                "edges": [
                    {"from": "welcome", "to": "ready", "condition": "true", "actions": {}}
                ]
            },
            {
                "id": "gameplay",
                "initial_state": "playing",
                "nodes": [
                    {"id": "playing", "vars": {}},
                    {"id": "paused", "vars": {}}
                ],
                "edges": [
                    {"from": "playing", "to": "paused", "condition": "false", "actions": {}}
                ]
            }
        ]
    })";
    
    std::ofstream tmpFile("test_multi_phases.json");
    tmpFile << jsonContent;
    tmpFile.close();
    
    EXPECT_TRUE(multiGraph.loadFromJson("test_multi_phases.json"));
    
    // Should start in first phase
    EXPECT_EQ(multiGraph.currentPhaseId(), "intro");
    EXPECT_EQ(multiGraph.currentStateId(), "welcome");
    
    std::remove("test_multi_phases.json");
}

TEST_F(MultiPhaseStateGraphTest, PhaseTransitions) {
    std::string jsonContent = R"({
        "phases": [
            {
                "id": "phase1",
                "initial_state": "start",
                "nodes": [
                    {"id": "start", "vars": {"score": 0}},
                    {"id": "middle", "vars": {"score": 0}}
                ],
                "edges": [
                    {"from": "start", "to": "middle", "condition": "true", "actions": {"score": 50}}
                ]
            },
            {
                "id": "phase2",
                "initial_state": "begin",
                "nodes": [
                    {"id": "begin", "vars": {}},
                    {"id": "finish", "vars": {}}
                ],
                "edges": [
                    {"from": "begin", "to": "finish", "condition": "true", "actions": {}}
                ]
            }
        ],
        "phase_edges": [
            {"from": "phase1", "to": "phase2", "condition": "score >= 50"}
        ]
    })";
    
    std::ofstream tmpFile("test_phase_transitions.json");
    tmpFile << jsonContent;
    tmpFile.close();
    
    EXPECT_TRUE(multiGraph.loadFromJson("test_phase_transitions.json"));
    
    // Initially in phase1
    EXPECT_EQ(multiGraph.currentPhaseId(), "phase1");
    EXPECT_EQ(multiGraph.currentStateId(), "start");
    
    // Step 1: start -> middle (sets score = 50)
    auto result1 = multiGraph.step();
    ASSERT_TRUE(result1.has_value());
    EXPECT_FALSE(result1->phase_changed);
    EXPECT_TRUE(result1->state_changed);
    EXPECT_EQ(result1->phase_id, "phase1");
    EXPECT_EQ(result1->state_id, "middle");
    
    // Step 2: phase transition should occur (score >= 50)
    auto result2 = multiGraph.step();
    ASSERT_TRUE(result2.has_value());
    EXPECT_TRUE(result2->phase_changed);
    EXPECT_EQ(result2->phase_id, "phase2");
    EXPECT_EQ(result2->state_id, "begin");
    
    std::remove("test_phase_transitions.json");
}

TEST_F(MultiPhaseStateGraphTest, SetInitialPhase) {
    std::string jsonContent = R"({
        "phases": [
            {
                "id": "phase1",
                "initial_state": "start1",
                "nodes": [{"id": "start1", "vars": {}}],
                "edges": []
            },
            {
                "id": "phase2", 
                "initial_state": "start2",
                "nodes": [{"id": "start2", "vars": {}}],
                "edges": []
            }
        ]
    })";
    
    std::ofstream tmpFile("test_initial_phase.json");
    tmpFile << jsonContent;
    tmpFile.close();
    
    EXPECT_TRUE(multiGraph.loadFromJson("test_initial_phase.json"));
    
    // Should start in phase1
    EXPECT_EQ(multiGraph.currentPhaseId(), "phase1");
    
    // Change to phase2
    EXPECT_TRUE(multiGraph.setInitialPhase("phase2"));
    EXPECT_EQ(multiGraph.currentPhaseId(), "phase2");
    EXPECT_EQ(multiGraph.currentStateId(), "start2");
    
    // Try invalid phase
    EXPECT_FALSE(multiGraph.setInitialPhase("nonexistent"));
    
    std::remove("test_initial_phase.json");
}

TEST_F(MultiPhaseStateGraphTest, NodeVariableAccess) {
    std::string jsonContent = R"({
        "phases": [
            {
                "id": "test_phase",
                "initial_state": "test_state",
                "nodes": [
                    {"id": "test_state", "vars": {"health": 100, "mana": 50}}
                ],
                "edges": []
            }
        ]
    })";
    
    std::ofstream tmpFile("test_variables.json");
    tmpFile << jsonContent;
    tmpFile.close();
    
    EXPECT_TRUE(multiGraph.loadFromJson("test_variables.json"));
    
    // Test variable access
    const auto& node = multiGraph.currentNode();
    auto healthVar = node.getVar("health");
    auto manaVar = node.getVar("mana");
    
    ASSERT_NE(healthVar, nullptr);
    ASSERT_NE(manaVar, nullptr);
    EXPECT_EQ(std::get<int64_t>(*healthVar), 100);
    EXPECT_EQ(std::get<int64_t>(*manaVar), 50);
    
    // Test variable modification
    auto& mutableNode = multiGraph.currentNode();
    mutableNode.setVar("health", Value{75});
    
    auto updatedHealth = node.getVar("health");
    ASSERT_NE(updatedHealth, nullptr);
    EXPECT_EQ(std::get<int64_t>(*updatedHealth), 75);
    
    std::remove("test_variables.json");
}

TEST_F(MultiPhaseStateGraphTest, ComplexGameScenario) {
    std::string jsonContent = R"({
        "phases": [
            {
                "id": "menu",
                "initial_state": "main_menu",
                "nodes": [
                    {"id": "main_menu", "vars": {"selection": 0}},
                    {"id": "options", "vars": {}},
                    {"id": "start_game", "vars": {}}
                ],
                "edges": [
                    {"from": "main_menu", "to": "options", "condition": "selection == 1", "actions": {}},
                    {"from": "main_menu", "to": "start_game", "condition": "selection == 2", "actions": {}}
                ]
            },
            {
                "id": "game",
                "initial_state": "playing",
                "nodes": [
                    {"id": "playing", "vars": {"lives": 3, "score": 0}},
                    {"id": "game_over", "vars": {}}
                ],
                "edges": [
                    {"from": "playing", "to": "game_over", "condition": "lives <= 0", "actions": {}}
                ]
            },
            {
                "id": "end",
                "initial_state": "results",
                "nodes": [
                    {"id": "results", "vars": {}},
                    {"id": "high_score", "vars": {}}
                ],
                "edges": [
                    {"from": "results", "to": "high_score", "condition": "score > 1000", "actions": {}}
                ]
            }
        ],
        "phase_edges": [
            {"from": "menu", "to": "game", "condition": "true"},
            {"from": "game", "to": "end", "condition": "lives <= 0"}
        ]
    })";
    
    std::ofstream tmpFile("test_game_scenario.json");
    tmpFile << jsonContent;
    tmpFile.close();
    
    EXPECT_TRUE(multiGraph.loadFromJson("test_game_scenario.json"));
    
    // Start in menu phase
    EXPECT_EQ(multiGraph.currentPhaseId(), "menu");
    EXPECT_EQ(multiGraph.currentStateId(), "main_menu");
    
    // Set selection to start game
    auto& menuNode = multiGraph.currentNode();
    menuNode.setVar("selection", Value{2});
    
    // First step should trigger phase transition to game (the phase edge condition "true" always matches)
    auto result1 = multiGraph.step();
    ASSERT_TRUE(result1.has_value());
    EXPECT_TRUE(result1->phase_changed);
    EXPECT_EQ(result1->phase_id, "game");
    EXPECT_EQ(result1->state_id, "playing");
    
    // Verify game state
    const auto& gameNode = multiGraph.currentNode();
    auto livesVar = gameNode.getVar("lives");
    ASSERT_NE(livesVar, nullptr);
    EXPECT_EQ(std::get<int64_t>(*livesVar), 3);
    
    std::remove("test_game_scenario.json");
}

TEST_F(MultiPhaseStateGraphTest, StepWithoutPhaseChange) {
    std::string jsonContent = R"({
        "phases": [
            {
                "id": "test_phase",
                "initial_state": "state1",
                "nodes": [
                    {"id": "state1", "vars": {}},
                    {"id": "state2", "vars": {}},
                    {"id": "state3", "vars": {}}
                ],
                "edges": [
                    {"from": "state1", "to": "state2", "condition": "true", "actions": {}},
                    {"from": "state2", "to": "state3", "condition": "true", "actions": {}}
                ]
            }
        ]
    })";
    
    std::ofstream tmpFile("test_no_phase_change.json");
    tmpFile << jsonContent;
    tmpFile.close();
    
    EXPECT_TRUE(multiGraph.loadFromJson("test_no_phase_change.json"));
    
    // Step 1: state1 -> state2
    auto result1 = multiGraph.step();
    ASSERT_TRUE(result1.has_value());
    EXPECT_FALSE(result1->phase_changed);
    EXPECT_TRUE(result1->state_changed);
    EXPECT_EQ(result1->state_id, "state2");
    
    // Step 2: state2 -> state3
    auto result2 = multiGraph.step();
    ASSERT_TRUE(result2.has_value());
    EXPECT_FALSE(result2->phase_changed);
    EXPECT_TRUE(result2->state_changed);
    EXPECT_EQ(result2->state_id, "state3");
    
    // Step 3: no transitions available
    auto result3 = multiGraph.step();
    ASSERT_TRUE(result3.has_value());
    EXPECT_FALSE(result3->phase_changed);
    EXPECT_FALSE(result3->state_changed);
    EXPECT_EQ(result3->state_id, "state3");
    
    std::remove("test_no_phase_change.json");
}

TEST_F(MultiPhaseStateGraphTest, ErrorHandling) {
    // Test loading invalid JSON
    EXPECT_FALSE(multiGraph.loadFromJson("nonexistent_file.json"));
    
    // Test invalid JSON content
    std::string invalidJson = R"({"invalid": "json"})";
    std::ofstream tmpFile("test_invalid.json");
    tmpFile << invalidJson;
    tmpFile.close();
    
    EXPECT_FALSE(multiGraph.loadFromJson("test_invalid.json"));
    
    std::remove("test_invalid.json");
}

TEST_F(MultiPhaseStateGraphTest, DuplicatePhaseIds) {
    std::string jsonContent = R"({
        "phases": [
            {
                "id": "duplicate",
                "initial_state": "start1",
                "nodes": [{"id": "start1", "vars": {}}],
                "edges": []
            },
            {
                "id": "duplicate",
                "initial_state": "start2", 
                "nodes": [{"id": "start2", "vars": {}}],
                "edges": []
            }
        ]
    })";
    
    std::ofstream tmpFile("test_duplicate.json");
    tmpFile << jsonContent;
    tmpFile.close();
    
    EXPECT_FALSE(multiGraph.loadFromJson("test_duplicate.json"));
    
    std::remove("test_duplicate.json");
}

TEST_F(MultiPhaseStateGraphTest, InvalidPhaseEdgeReference) {
    std::string jsonContent = R"({
        "phases": [
            {
                "id": "phase1",
                "initial_state": "start",
                "nodes": [{"id": "start", "vars": {}}],
                "edges": []
            }
        ],
        "phase_edges": [
            {"from": "phase1", "to": "nonexistent_phase", "condition": "true"}
        ]
    })";
    
    std::ofstream tmpFile("test_invalid_edge.json");
    tmpFile << jsonContent;
    tmpFile.close();
    
    EXPECT_FALSE(multiGraph.loadFromJson("test_invalid_edge.json"));
    
    std::remove("test_invalid_edge.json");
}

TEST_F(MultiPhaseStateGraphTest, EmptyPhases) {
    std::string jsonContent = R"({
        "phases": []
    })";
    
    std::ofstream tmpFile("test_empty.json");
    tmpFile << jsonContent;
    tmpFile.close();
    
    EXPECT_TRUE(multiGraph.loadFromJson("test_empty.json"));
    
    // Should have no current phase
    EXPECT_THROW(multiGraph.currentPhaseId(), std::runtime_error);
    
    std::remove("test_empty.json");
}

TEST_F(MultiPhaseStateGraphTest, PerformanceTest) {
    // Create a larger graph for performance testing
    std::string jsonContent = R"({
        "phases": [)";
    
    // Create 10 phases with 10 nodes each
    for (int p = 0; p < 10; ++p) {
        if (p > 0) jsonContent += ",";
        jsonContent += R"(
            {
                "id": "phase)" + std::to_string(p) + R"(",
                "initial_state": "node0",
                "nodes": [)";
        
        for (int n = 0; n < 10; ++n) {
            if (n > 0) jsonContent += ",";
            jsonContent += R"(
                    {"id": "node)" + std::to_string(n) + R"(", "vars": {"value": )" + std::to_string(n * 10) + R"(}})";
        }
        
        jsonContent += R"(
                ],
                "edges": [)";
        
        for (int e = 0; e < 9; ++e) {
            if (e > 0) jsonContent += ",";
            jsonContent += R"(
                    {"from": "node)" + std::to_string(e) + R"(", "to": "node)" + std::to_string(e + 1) + R"(", "condition": "true", "actions": {}})";
        }
        
        jsonContent += R"(
                ]
            })";
    }
    
    jsonContent += R"(
        ],
        "phase_edges": [)";
    
    for (int p = 0; p < 9; ++p) {
        if (p > 0) jsonContent += ",";
        jsonContent += R"(
            {"from": "phase)" + std::to_string(p) + R"(", "to": "phase)" + std::to_string(p + 1) + R"(", "condition": "value >= )" + std::to_string(p * 10 + 90) + R"("})";
    }
    
    jsonContent += R"(
        ]
    })";
    
    std::ofstream tmpFile("test_performance.json");
    tmpFile << jsonContent;
    tmpFile.close();
    
    auto start = std::chrono::high_resolution_clock::now();
    EXPECT_TRUE(multiGraph.loadFromJson("test_performance.json"));
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    EXPECT_LT(duration.count(), 1000); // Should load in less than 1 second
    
    // Test stepping performance
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 100; ++i) {
        auto result = multiGraph.step();
        if (!result.has_value()) break;
    }
    end = std::chrono::high_resolution_clock::now();
    
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    EXPECT_LT(duration.count(), 100); // Should step quickly
    
    std::remove("test_performance.json");
}
