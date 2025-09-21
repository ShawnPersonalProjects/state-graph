#pragma once
#include <string>
#include <memory>
#include "json.hpp"
#include "expression.hpp"
#include "node.hpp"

// Phase transition edge: condition expression evaluated against CURRENT NODE of current phase.
// Special injected variable: phase_id (string) = current phase id.
struct DefaultPhaseEdge {
    std::string from;
    std::string to;
    std::string condition_expr;
    std::unique_ptr<Expression> compiled;

    static DefaultPhaseEdge from_json(const nlohmann::json& j) {
        DefaultPhaseEdge e;
        e.from = j.at("from").get<std::string>();
        e.to   = j.at("to").get<std::string>();
        e.condition_expr = j.at("condition").get<std::string>();
        e.compiled = compileExpression(e.condition_expr);
        return e;
    }

    bool evaluate(const DefaultNode& currentNode) const {
        return compiled ? compiled->eval(currentNode) : false;
    }
};