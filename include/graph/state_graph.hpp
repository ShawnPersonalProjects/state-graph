#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include <optional>
#include <functional>
#include <stdexcept>
#include <fstream>
#include "json.hpp"
#include "node.hpp"
#include "edge.hpp"

template<typename NodeT, typename EdgeT>
class StateGraph {
public:
    using Node = NodeT;
    using Edge = EdgeT;

private:
    std::vector<Node> nodes_;
    std::vector<Edge> edges_;
    std::unordered_map<std::string, std::size_t> node_index_;
    std::vector<std::vector<std::size_t>> adjacency_;
    std::optional<std::size_t> current_index_;

public:
    void clear() {
        nodes_.clear();
        edges_.clear();
        node_index_.clear();
        adjacency_.clear();
        current_index_.reset();
    }

    void addNode(const Node& n) {
        if (node_index_.count(n.id))
            throw std::runtime_error("Duplicate node id: " + n.id);
        node_index_[n.id] = nodes_.size();
        nodes_.push_back(n);
        adjacency_.emplace_back();
    }

    void addEdge(Edge e) {
        auto fi = node_index_.find(e.from);
        auto ti = node_index_.find(e.to);
        if (fi == node_index_.end() || ti == node_index_.end())
            throw std::runtime_error("Edge references unknown node");
        std::size_t idx = edges_.size();
        edges_.push_back(std::move(e));
        adjacency_[fi->second].push_back(idx);
    }

    bool setInitialState(const std::string& id) {
        auto it = node_index_.find(id);
        if (it == node_index_.end()) return false;
        current_index_ = it->second;
        return true;
    }

    bool hasCurrentState() const {
        return current_index_.has_value();
    }

    const std::string& currentStateId() const {
        if (!current_index_) throw std::runtime_error("No current state");
        return nodes_[*current_index_].id;
    }

    Node& currentNode() {
        if (!current_index_) throw std::runtime_error("No current state");
        return nodes_[*current_index_];
    }
    const Node& currentNode() const {
        if (!current_index_) throw std::runtime_error("No current state");
        return nodes_[*current_index_];
    }

    // Evaluate all outgoing edges' logical expressions
    std::optional<std::string> step() {
        if (!current_index_) return std::nullopt;
        auto fromIdx = *current_index_;
        const Node& cur = nodes_[fromIdx];
        for (auto edgeIdx : adjacency_[fromIdx]) {
            auto& e = edges_[edgeIdx];
            if (e.evaluate(cur)) {
                current_index_ = node_index_[e.to];
                // apply actions to destination node
                Node& dest = nodes_[*current_index_];
                for (auto& [k,v]: e.actions) {
                    dest.setVar(k,v);
                }
                return dest.id;
            }
        }
        return std::nullopt;
    }

    bool loadFromJson(const std::string& filePath) {
        std::ifstream in(filePath);
        if (!in.is_open()) return false;
        nlohmann::json j;
        in >> j;
        clear();
        for (auto& nj : j.at("nodes")) addNode(Node::from_json(nj));
        for (auto& ej : j.at("edges")) addEdge(Edge::from_json(ej));
        if (!nodes_.empty()) current_index_ = 0;
        return true;
    }
};
