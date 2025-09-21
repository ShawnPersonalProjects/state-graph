#pragma once
#include <string>
#include <unordered_map>
#include "json.hpp"
#include <memory>
#include "value.hpp"
#include "expression.hpp"
#include "node.hpp"

struct DefaultEdge {
    std::string from;
    std::string to;
    std::string condition_expr;
    std::unique_ptr<Expression> compiled;
    std::unordered_map<std::string, Value> actions; // variable updates applied to destination

    static Value parseValue(const nlohmann::json& j) {
        if (j.is_number_integer()) return static_cast<int64_t>(j.get<int64_t>());
        if (j.is_number_float())   return j.get<double>();
        if (j.is_boolean())        return j.get<bool>();
        if (j.is_string())         return j.get<std::string>();
        throw std::runtime_error("Unsupported action value type");
    }

    static DefaultEdge from_json(const nlohmann::json& j) {
        DefaultEdge e;
        e.from = j.at("from").get<std::string>();
        e.to   = j.at("to").get<std::string>();
        e.condition_expr = j.at("condition").get<std::string>();
        e.compiled = compileExpression(e.condition_expr);
        if (j.contains("actions")) {
            for (auto it = j["actions"].begin(); it != j["actions"].end(); ++it) {
                e.actions[it.key()] = parseValue(it.value());
            }
        }
        return e;
    }

    bool evaluate(const DefaultNode& current) const {
        return compiled ? compiled->eval(current) : true;
    }
};