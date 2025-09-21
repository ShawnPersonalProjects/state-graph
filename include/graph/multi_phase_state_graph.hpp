#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include <optional>
#include <stdexcept>
#include "json.hpp"
#include "state_graph.hpp"
#include "phase_edge.hpp"

template<typename NodeT, typename EdgeT, typename PhaseEdgeT>
class MultiPhaseStateGraph {
public:
    using Node = NodeT;
    using Edge = EdgeT;
    using PhaseEdge = PhaseEdgeT;

    struct Phase {
        std::string id;
        StateGraph<NodeT, EdgeT> graph;
        std::string initial_state; // optional override
    };

private:
    std::vector<Phase> phases_;
    std::unordered_map<std::string, std::size_t> phase_index_;
    std::vector<PhaseEdge> phase_edges_;
    std::vector<std::vector<std::size_t>> phase_adj_; // per phase -> phase edge indices
    std::optional<std::size_t> current_phase_;

public:
    void clear() {
        phases_.clear();
        phase_index_.clear();
        phase_edges_.clear();
        phase_adj_.clear();
        current_phase_.reset();
    }

    const std::string& currentPhaseId() const {
        if (!current_phase_) throw std::runtime_error("No current phase");
        return phases_[*current_phase_].id;
    }
    const std::string& currentStateId() const {
        if (!current_phase_) throw std::runtime_error("No current phase");
        return phases_[*current_phase_].graph.currentStateId();
    }
    Node& currentNode() {
        if (!current_phase_) throw std::runtime_error("No current phase");
        return phases_[*current_phase_].graph.currentNode();
    }
    const Node& currentNode() const {
        if (!current_phase_) throw std::runtime_error("No current phase");
        return phases_[*current_phase_].graph.currentNode();
    }

    bool setInitialPhase(const std::string& phaseId) {
        auto it = phase_index_.find(phaseId);
        if (it == phase_index_.end()) return false;
        current_phase_ = it->second;
        auto& p = phases_[*current_phase_];
        if (!p.initial_state.empty()) p.graph.setInitialState(p.initial_state);
        return true;
    }

    // One advancement:
    // 1. Try node-level transition inside current phase (graph.step()).
    // 2. Evaluate phase edges; if condition true -> switch phase (and set its initial state).
    // Returns tuple (phase_changed, state_changed, new_phase_id, new_state_id).
    struct StepResult {
        bool phase_changed=false;
        bool state_changed=false;
        std::string phase_id;
        std::string state_id;
    };

    std::optional<StepResult> step() {
        if (!current_phase_) return std::nullopt;
        StepResult r;
        auto& phase = phases_[*current_phase_];

        // Node-level step
        auto prevState = phase.graph.currentStateId();
        if (auto ns = phase.graph.step()) {
            r.state_changed = true;
        }
        // Phase-level evaluation (after node step)
        auto& curNode = phase.graph.currentNode();
        for (auto peIdx : phase_adj_[*current_phase_]) {
            auto& pe = phase_edges_[peIdx];
            if (pe.evaluate(curNode)) {
                // Phase transition
                auto targetIt = phase_index_.find(pe.to);
                if (targetIt == phase_index_.end())
                    throw std::runtime_error("Phase edge to unknown phase: " + pe.to);
                current_phase_ = targetIt->second;
                auto& newPhase = phases_[*current_phase_];
                // If new phase has no current state set yet, set its initial
                if (!newPhase.graph.hasCurrentState()) {
                    if (!newPhase.initial_state.empty())
                        newPhase.graph.setInitialState(newPhase.initial_state);
                }
                r.phase_changed = true;
                break;
            }
        }
        r.phase_id = currentPhaseId();
        r.state_id = currentStateId();
        return r;
    }

    bool loadFromJson(const std::string& filePath) {
        clear();
        std::ifstream in(filePath);
        if (!in.is_open()) return false;
        nlohmann::json j;
        in >> j;

        if (!j.contains("phases") || !j["phases"].is_array())
            throw std::runtime_error("Missing phases array");

        // Load phases
        for (auto& pj : j["phases"]) {
            Phase p;
            p.id = pj.at("id").get<std::string>();
            if (phase_index_.count(p.id))
                throw std::runtime_error("Duplicate phase id: " + p.id);
            if (pj.contains("nodes"))
                for (auto& nj : pj["nodes"]) p.graph.addNode(Node::from_json(nj));
            if (pj.contains("edges"))
                for (auto& ej : pj["edges"]) p.graph.addEdge(Edge::from_json(ej));
            if (pj.contains("initial_state"))
                p.initial_state = pj["initial_state"].get<std::string>();
            if (!p.initial_state.empty())
                p.graph.setInitialState(p.initial_state);
            phase_index_[p.id] = phases_.size();
            phases_.push_back(std::move(p));
            phase_adj_.emplace_back();
        }

        // Load phase edges
        if (j.contains("phase_edges")) {
            for (auto& fe : j["phase_edges"]) {
                auto edge = PhaseEdge::from_json(fe);
                auto fi = phase_index_.find(edge.from);
                auto ti = phase_index_.find(edge.to);
                if (fi == phase_index_.end() || ti == phase_index_.end())
                    throw std::runtime_error("Phase edge references unknown phase");
                std::size_t idx = phase_edges_.size();
                phase_edges_.push_back(std::move(edge));
                phase_adj_[fi->second].push_back(idx);
            }
        }

        // Set first phase as current if any
        if (!phases_.empty()) {
            current_phase_ = 0;
            auto& p = phases_[0];
            if (!p.graph.hasCurrentState() && !p.initial_state.empty())
                p.graph.setInitialState(p.initial_state);
        }
        return true;
    }
};