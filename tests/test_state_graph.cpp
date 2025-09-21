#include <cassert>
#include "graph/multi_phase_state_graph.hpp"
#include "graph/node.hpp"
#include "graph/edge.hpp"
#include "graph/phase_edge.hpp"

int main() {
    using Node=DefaultNode;
    using Edge=DefaultEdge;
    using PhaseEdge=DefaultPhaseEdge;

    MultiPhaseStateGraph<Node,Edge,PhaseEdge> mpg;
    assert(mpg.loadFromJson("../config/sample_graph.json"));
    assert(mpg.setInitialPhase("Main"));
    // Run until phase change
    bool phaseChanged=false;
    for (int i=0;i<20 && !phaseChanged;++i) {
        auto r=mpg.step();
        assert(r);
        if (r->phase_changed) {
            phaseChanged=true;
            assert(r->phase_id=="Recovery");
        }
    }
    assert(phaseChanged);
    return 0;
}