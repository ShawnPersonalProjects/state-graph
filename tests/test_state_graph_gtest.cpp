#include <gtest/gtest.h>
#include "../include/graph/state_graph.hpp"
#include "../include/graph/node.hpp"
#include "../include/graph/edge.hpp"
#include "../include/graph/expression.hpp"
#include "../include/graph/value.hpp"

class StateGraphTest : public ::testing::Test {
protected:
    using TestGraph = StateGraph<DefaultNode, DefaultEdge>;
    TestGraph graph;
    
    void SetUp() override {
        // Clear any previous state
        graph.clear();
    }
    
    // Helper function to create edges using the actual DefaultEdge interface
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
    
    // Helper to create nodes with properties
    DefaultNode createNode(const std::string& id, 
                          const std::unordered_map<std::string, Value>& props = {}) {
        DefaultNode node;
        node.id = id;
        for (const auto& [key, value] : props) {
            node.setVar(key, value);
        }
        return node;
    }
};

TEST_F(StateGraphTest, BasicGraphCreation) {
    EXPECT_FALSE(graph.hasCurrentState());
    // Note: StateGraph doesn't expose getNodes() or getEdges() methods
    // So we can't directly test those - we'll test indirectly through behavior
}

TEST_F(StateGraphTest, AddNodes) {
    auto node1 = createNode("start");
    auto node2 = createNode("end");
    
    graph.addNode(node1);
    graph.addNode(node2);
    
    // Test that we can set initial state to a node we added
    EXPECT_TRUE(graph.setInitialState("start"));
    EXPECT_TRUE(graph.hasCurrentState());
    EXPECT_EQ(graph.currentStateId(), "start");
}

TEST_F(StateGraphTest, AddEdges) {
    auto node1 = createNode("start");
    auto node2 = createNode("end");
    
    graph.addNode(node1);
    graph.addNode(node2);
    
    auto edge = createEdge("start", "end");
    EXPECT_NO_THROW(graph.addEdge(std::move(edge)));
}

TEST_F(StateGraphTest, SetInitialState) {
    auto node1 = createNode("start");
    graph.addNode(node1);
    
    EXPECT_TRUE(graph.setInitialState("start"));
    EXPECT_TRUE(graph.hasCurrentState());
    EXPECT_EQ(graph.currentStateId(), "start");
    
    // Test setting invalid state
    EXPECT_FALSE(graph.setInitialState("nonexistent"));
}

TEST_F(StateGraphTest, SimpleStep) {
    auto node1 = createNode("start");
    auto node2 = createNode("end");
    
    graph.addNode(node1);
    graph.addNode(node2);
    
    auto edge = createEdge("start", "end", "true");
    graph.addEdge(std::move(edge));
    
    graph.setInitialState("start");
    
    auto result = graph.step();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, "end");
    EXPECT_EQ(graph.currentStateId(), "end");
}

TEST_F(StateGraphTest, ConditionalTransition) {
    auto node1 = createNode("start", {{"health", Value{100}}});
    auto node2 = createNode("good");
    auto node3 = createNode("bad");
    
    graph.addNode(node1);
    graph.addNode(node2);
    graph.addNode(node3);
    
    auto edge1 = createEdge("start", "good", "health > 50");
    auto edge2 = createEdge("start", "bad", "health <= 50");
    
    graph.addEdge(std::move(edge1));
    graph.addEdge(std::move(edge2));
    
    graph.setInitialState("start");
    
    // With health=100, should go to "good"
    auto result = graph.step();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, "good");
}

TEST_F(StateGraphTest, ActionApplication) {
    auto node1 = createNode("start");
    auto node2 = createNode("powered_up");
    
    graph.addNode(node1);
    graph.addNode(node2);
    
    std::unordered_map<std::string, Value> actions{
        {"health", Value{150}},
        {"score", Value{100}}
    };
    
    auto edge = createEdge("start", "powered_up", "true", actions);
    graph.addEdge(std::move(edge));
    
    graph.setInitialState("start");
    
    auto result = graph.step();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, "powered_up");
    
    // Check that actions were applied to the destination node
    const auto& currentNode = graph.currentNode();
    auto healthVar = currentNode.getVar("health");
    auto scoreVar = currentNode.getVar("score");
    ASSERT_NE(healthVar, nullptr);
    ASSERT_NE(scoreVar, nullptr);
    EXPECT_EQ(std::get<int64_t>(*healthVar), 150);
    EXPECT_EQ(std::get<int64_t>(*scoreVar), 100);
}

TEST_F(StateGraphTest, NoValidTransition) {
    auto node1 = createNode("start");
    auto node2 = createNode("unreachable");
    
    graph.addNode(node1);
    graph.addNode(node2);
    
    auto edge = createEdge("start", "unreachable", "false");
    graph.addEdge(std::move(edge));
    
    graph.setInitialState("start");
    
    auto result = graph.step();
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(graph.currentStateId(), "start");
}

TEST_F(StateGraphTest, NodeVariableAccess) {
    auto node1 = createNode("start", {{"health", Value{100}}, {"score", Value{0}}});
    graph.addNode(node1);
    graph.setInitialState("start");
    
    const auto& currentNode = graph.currentNode();
    auto healthVar = currentNode.getVar("health");
    auto scoreVar = currentNode.getVar("score");
    ASSERT_NE(healthVar, nullptr);
    ASSERT_NE(scoreVar, nullptr);
    EXPECT_EQ(std::get<int64_t>(*healthVar), 100);
    EXPECT_EQ(std::get<int64_t>(*scoreVar), 0);
    
    // Modify node variables
    auto& mutableNode = graph.currentNode();
    mutableNode.setVar("health", Value{50});
    mutableNode.setVar("score", Value{25});
    
    auto updatedHealthVar = currentNode.getVar("health");
    auto updatedScoreVar = currentNode.getVar("score");
    ASSERT_NE(updatedHealthVar, nullptr);
    ASSERT_NE(updatedScoreVar, nullptr);
    EXPECT_EQ(std::get<int64_t>(*updatedHealthVar), 50);
    EXPECT_EQ(std::get<int64_t>(*updatedScoreVar), 25);
}

TEST_F(StateGraphTest, EdgeConditionEvaluation) {
    auto node1 = createNode("start", {{"level", Value{5}}});
    auto node2 = createNode("advanced");
    auto node3 = createNode("beginner");
    
    graph.addNode(node1);
    graph.addNode(node2);
    graph.addNode(node3);
    
    auto edge1 = createEdge("start", "advanced", "level >= 5");
    auto edge2 = createEdge("start", "beginner", "level < 5");
    
    graph.addEdge(std::move(edge1));
    graph.addEdge(std::move(edge2));
    
    graph.setInitialState("start");
    
    // With level=5, should go to "advanced"
    auto result = graph.step();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, "advanced");
}

TEST_F(StateGraphTest, LoadFromJSON) {
    // Create a simple JSON config file
    std::string jsonContent = R"({
        "nodes": [
            {"id": "start", "variables": {}},
            {"id": "end", "variables": {}}
        ],
        "edges": [
            {"from": "start", "to": "end", "condition": "true", "actions": {}}
        ]
    })";
    
    // Write to a temporary file
    std::ofstream tmpFile("test_config.json");
    tmpFile << jsonContent;
    tmpFile.close();
    
    EXPECT_TRUE(graph.loadFromJson("test_config.json"));
    
    EXPECT_TRUE(graph.hasCurrentState()); // loadFromJson sets current to first node
    
    // Clean up
    std::remove("test_config.json");
}

TEST_F(StateGraphTest, MultiStepSequence) {
    auto node1 = createNode("start");
    auto node2 = createNode("middle");
    auto node3 = createNode("end");
    
    graph.addNode(node1);
    graph.addNode(node2);
    graph.addNode(node3);
    
    auto edge1 = createEdge("start", "middle", "true");
    auto edge2 = createEdge("middle", "end", "true");
    
    graph.addEdge(std::move(edge1));
    graph.addEdge(std::move(edge2));
    
    graph.setInitialState("start");
    
    // First step: start -> middle
    auto result1 = graph.step();
    ASSERT_TRUE(result1.has_value());
    EXPECT_EQ(*result1, "middle");
    
    // Second step: middle -> end
    auto result2 = graph.step();
    ASSERT_TRUE(result2.has_value());
    EXPECT_EQ(*result2, "end");
    
    // Third step: no outgoing edges from end
    auto result3 = graph.step();
    EXPECT_FALSE(result3.has_value());
    EXPECT_EQ(graph.currentStateId(), "end");
}

TEST_F(StateGraphTest, GameLikeScenario) {
    // Create a simple RPG-like state machine
    auto startNode = createNode("start", {{"health", Value{100}}, {"score", Value{0}}});
    auto fightNode = createNode("fight", {{"health", Value{100}}}); // Make sure fight node has health
    auto victoryNode = createNode("victory");
    auto defeatNode = createNode("defeat");
    
    graph.addNode(startNode);
    graph.addNode(fightNode);
    graph.addNode(victoryNode);
    graph.addNode(defeatNode);
    
    // Transitions
    auto toFight = createEdge("start", "fight", "true");
    auto toVictory = createEdge("fight", "victory", "health > 20", 
                               {{"score", Value{100}}, {"level", Value{2}}});
    auto toDefeat = createEdge("fight", "defeat", "health <= 20");
    
    graph.addEdge(std::move(toFight));
    graph.addEdge(std::move(toVictory));
    graph.addEdge(std::move(toDefeat));
    
    graph.setInitialState("start");
    
    // Start -> Fight
    auto result1 = graph.step();
    ASSERT_TRUE(result1.has_value());
    EXPECT_EQ(*result1, "fight");
    
    // Fight -> Victory (health=100 > 20)
    auto result2 = graph.step();
    ASSERT_TRUE(result2.has_value());
    EXPECT_EQ(*result2, "victory");
    
    // Check victory rewards were applied
    const auto& currentNode = graph.currentNode();
    auto scoreVar = currentNode.getVar("score");
    auto levelVar = currentNode.getVar("level");
    ASSERT_NE(scoreVar, nullptr);
    ASSERT_NE(levelVar, nullptr);
    EXPECT_EQ(std::get<int64_t>(*scoreVar), 100);
    EXPECT_EQ(std::get<int64_t>(*levelVar), 2);
}

TEST_F(StateGraphTest, ErrorHandling) {
    // Test adding edge with non-existent nodes
    auto edge = createEdge("nonexistent1", "nonexistent2");
    EXPECT_THROW(graph.addEdge(std::move(edge)), std::runtime_error);
    
    // Test duplicate node IDs
    auto node1 = createNode("duplicate");
    auto node2 = createNode("duplicate");
    
    graph.addNode(node1);
    EXPECT_THROW(graph.addNode(node2), std::runtime_error);
    
    // Test accessing current state when none is set
    EXPECT_THROW(graph.currentStateId(), std::runtime_error);
    EXPECT_THROW(graph.currentNode(), std::runtime_error);
}