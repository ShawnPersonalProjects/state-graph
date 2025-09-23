#pragma once
#include <string>
#include <unordered_map>
#include <iostream>
#include "json.hpp"
#include "value.hpp"

// Helper function to convert Value to string for printing
inline std::string valueToString(const Value& v) {
    if (auto p = std::get_if<bool>(&v)) return *p ? "true" : "false";
    if (auto p = std::get_if<int64_t>(&v)) return std::to_string(*p);
    if (auto p = std::get_if<double>(&v)) return std::to_string(*p);
    if (auto p = std::get_if<std::string>(&v)) return "\"" + *p + "\"";
    return "unknown";
}

struct DefaultNode {
    std::string id;
    // Static descriptive / configuration parameters
    std::unordered_map<std::string, Value> params;
    // Mutable runtime variables
    std::unordered_map<std::string, Value> vars;
    // Properties that can be referenced in conditions
    std::unordered_map<std::string, Value> properties;

    static Value parseValue(const nlohmann::json& j) {
        if (j.is_number_integer()) return static_cast<int64_t>(j.get<int64_t>());
        if (j.is_number_float())   return j.get<double>();
        if (j.is_boolean())        return j.get<bool>();
        if (j.is_string())         return j.get<std::string>();
        throw std::runtime_error("Unsupported value type");
    }

    static DefaultNode from_json(const nlohmann::json& j) {
        DefaultNode n;
        n.id = j.at("id").get<std::string>();
        if (j.contains("params")) {
            for (auto it = j["params"].begin(); it != j["params"].end(); ++it) {
                n.params[it.key()] = parseValue(it.value());
            }
        }
        if (j.contains("vars")) {
            for (auto it = j["vars"].begin(); it != j["vars"].end(); ++it) {
                n.vars[it.key()] = parseValue(it.value());
            }
        }
        if (j.contains("properties")) {
            for (auto it = j["properties"].begin(); it != j["properties"].end(); ++it) {
                n.properties[it.key()] = parseValue(it.value());
            }
        }
        return n;
    }

    bool hasParam(const std::string& k) const { return params.find(k) != params.end(); }
    const Value* getParam(const std::string& k) const {
        auto it = params.find(k);
        return it == params.end() ? nullptr : &it->second;
    }
    bool hasVar(const std::string& k) const { return vars.find(k) != vars.end(); }
    const Value* getVar(const std::string& k) const {
        auto it = vars.find(k);
        return it == vars.end() ? nullptr : &it->second;
    }
    void setVar(const std::string& k, Value v) { vars[k] = std::move(v); }
    
    bool hasProperty(const std::string& k) const { return properties.find(k) != properties.end(); }
    const Value* getProperty(const std::string& k) const {
        auto it = properties.find(k);
        return it == properties.end() ? nullptr : &it->second;
    }
    
    void print() const {
        std::cout << "Node '" << id << "':\n";
        
        // Print parameters
        std::cout << "  Parameters: ";
        if (params.empty()) {
            std::cout << "(none)";
        } else {
            bool first = true;
            for (const auto& [key, value] : params) {
                if (!first) std::cout << ", ";
                std::cout << key << "=" << valueToString(value);
                first = false;
            }
        }
        std::cout << "\n";
        
        // Print variables
        std::cout << "  Variables: ";
        if (vars.empty()) {
            std::cout << "(none)";
        } else {
            bool first = true;
            for (const auto& [key, value] : vars) {
                if (!first) std::cout << ", ";
                std::cout << key << "=" << valueToString(value);
                first = false;
            }
        }
        std::cout << "\n";
        
        // Print properties
        std::cout << "  Properties: ";
        if (properties.empty()) {
            std::cout << "(none)";
        } else {
            bool first = true;
            for (const auto& [key, value] : properties) {
                if (!first) std::cout << ", ";
                std::cout << key << "=" << valueToString(value);
                first = false;
            }
        }
        std::cout << "\n";
    }
}
; // end struct