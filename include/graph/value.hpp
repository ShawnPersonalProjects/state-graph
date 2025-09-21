#pragma once
#include <variant>
#include <string>
#include <cstdint>
#include <stdexcept>

using Value = std::variant<int64_t, double, bool, std::string>;

inline double toNumber(const Value& v) {
    if (auto p = std::get_if<int64_t>(&v)) return static_cast<double>(*p);
    if (auto p2 = std::get_if<double>(&v)) return *p2;
    throw std::runtime_error("Value is not numeric");
}

inline bool toBool(const Value& v) {
    if (auto p = std::get_if<bool>(&v)) return *p;
    throw std::runtime_error("Value is not bool");
}

inline std::string toString(const Value& v) {
    if (auto p = std::get_if<std::string>(&v)) return *p;
    throw std::runtime_error("Value is not string");
}