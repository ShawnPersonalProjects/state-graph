#include <iostream>
#include "graph/multi_phase_state_graph.hpp"
#include "graph/node.hpp"
#include "graph/edge.hpp"
#include "graph/phase_edge.hpp"

int main() {
    using Node = DefaultNode;
    using Edge = DefaultEdge;
    using PhaseEdge = DefaultPhaseEdge;
    MultiPhaseStateGraph<Node, Edge, PhaseEdge> mpg;

    if (!mpg.loadFromJson("../config/sample_graph.json")) {
        std::cerr << "Config load failed\n";
        return 1;
    }

    std::cout << "Config loaded\n";

    mpg.setInitialPhase("Main");
    std::cout << "Start Phase: " << mpg.currentPhaseId()
              << " State: " << mpg.currentStateId() << "\n";

    for (int i=0; i<15; ++i) {
        mpg.currentNode().print();
        std::cout << "\n";
        auto r = mpg.step();
        if (!r) { std::cout << "No step\n"; break; }
        std::cout << "[" << i << "] Phase=" << r->phase_id
                  << " State=" << r->state_id
                  << (r->phase_changed ? " (phase change)" : "")
                  << (r->state_changed ? " (state change)" : "")
                  << "\n";
    }
    return 0;
}