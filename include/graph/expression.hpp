#pragma once
#include <string>
#include <cctype>
#include <vector>
#include <memory>
#include <unordered_map>
#include <stdexcept>
#include "value.hpp"
#include "node.hpp"

class Expression {
public:
    virtual ~Expression() = default;
    virtual bool eval(const DefaultNode& node) const = 0;
};

enum class TokKind { ID, NUM, STR, BOOL, OP, LP, RP, END };

struct Token {
    TokKind kind;
    std::string text;
};

class Lexer {
    const std::string& s;
    size_t i=0;
public:
    explicit Lexer(const std::string& in): s(in) {}
    Token next() {
        while (i<s.size() && std::isspace((unsigned char)s[i])) ++i;
        if (i>=s.size()) return {TokKind::END,""};
        char c = s[i];
        if (std::isalpha((unsigned char)c) || c=='_') {
            size_t start=i++;
            while (i<s.size() && (std::isalnum((unsigned char)s[i])||s[i]=='_'||s[i]=='.')) ++i;
            std::string id = s.substr(start,i-start);
            if (id=="true" || id=="false") return {TokKind::BOOL,id};
            return {TokKind::ID,id};
        }
        if (std::isdigit((unsigned char)c)) {
            size_t start=i++;
            while (i<s.size() && (std::isdigit((unsigned char)s[i])||s[i]=='.')) ++i;
            return {TokKind::NUM, s.substr(start,i-start)};
        }
        if (c=='"') {
            ++i;
            size_t start=i;
            while (i<s.size() && s[i]!='"') ++i;
            if (i>=s.size()) throw std::runtime_error("Unterminated string literal");
            std::string str = s.substr(start,i-start);
            ++i;
            return {TokKind::STR,str};
        }
        // operators
        auto match2=[&](char a,char b){ return i+1<s.size() && s[i]==a && s[i+1]==b; };
        if (match2('&','&')) { i+=2; return {TokKind::OP,"&&"}; }
        if (match2('|','|')) { i+=2; return {TokKind::OP,"||"}; }
        if (match2('=','=')) { i+=2; return {TokKind::OP,"=="}; }
        if (match2('!','=')) { i+=2; return {TokKind::OP,"!="}; }
        if (match2('<','=')) { i+=2; return {TokKind::OP,"<="}; }
        if (match2('>','=')) { i+=2; return {TokKind::OP,">="}; }
        if (c=='<' || c=='>' || c=='!' ) { ++i; return {TokKind::OP,std::string(1,c)}; }
        if (c=='(') { ++i; return {TokKind::LP,"("}; }
        if (c==')') { ++i; return {TokKind::RP,")"}; }
        throw std::runtime_error(std::string("Unexpected char: ")+c);
    }
};

struct ASTValue {
    enum Type { IDENT, NUMBER, STRING, BOOLEAN } type;
    std::string text;
    double num=0;
    bool b=false;
};

class ExprNode : public Expression {
public:
    // For logical / comparison tree
    enum Kind { LEAF, NOT, AND, OR, CMP } kind=LEAF;
    ASTValue leaf{};
    std::string cmpOp;
    std::unique_ptr<ExprNode> left, right;

    bool eval(const DefaultNode& node) const override {
        switch(kind) {
            case LEAF: {
                switch(leaf.type) {
                    case ASTValue::BOOLEAN: return leaf.b;
                    case ASTValue::NUMBER:  return leaf.num != 0.0;
                    case ASTValue::STRING:  return !leaf.text.empty();
                    case ASTValue::IDENT: {
                        const Value* v = nullptr;
                        if (leaf.text.substr(0, 11) == "properties.") {
                            // Handle property access: properties.propertyName
                            std::string propName = leaf.text.substr(11);
                            v = node.getProperty(propName);
                        } else {
                            // Handle variable access
                            v = node.getVar(leaf.text);
                        }
                        if (!v) return false;
                        if (std::holds_alternative<bool>(*v)) return std::get<bool>(*v);
                        if (std::holds_alternative<int64_t>(*v)) return std::get<int64_t>(*v)!=0;
                        if (std::holds_alternative<double>(*v)) return std::get<double>(*v)!=0.0;
                        if (std::holds_alternative<std::string>(*v)) return !std::get<std::string>(*v).empty();
                        return false;
                    }
                }
            }
            case NOT: return !left->eval(node);
            case AND: return left->eval(node) && right->eval(node);
            case OR:  return left->eval(node) || right->eval(node);
            case CMP: {
                // Evaluate comparison operands
                // Left & right must be leaf or subtree returning value semantics
                Value lv = extractValue(*left,node);
                Value rv = extractValue(*right,node);
                if (cmpOp=="=="||cmpOp=="!=") {
                    bool eq = valueEquals(lv,rv);
                    return cmpOp=="=="?eq:!eq;
                }
                // numeric comparisons
                double ln = toNumberPromote(lv);
                double rn = toNumberPromote(rv);
                if (cmpOp=="<") return ln<rn;
                if (cmpOp=="<=") return ln<=rn;
                if (cmpOp==">") return ln>rn;
                if (cmpOp==">=") return ln>=rn;
                throw std::runtime_error("Unknown comparison op");
            }
        }
        return false;
    }

private:
    static double toNumberPromote(const Value& v) {
        if (std::holds_alternative<int64_t>(v)) return static_cast<double>(std::get<int64_t>(v));
        if (std::holds_alternative<double>(v)) return std::get<double>(v);
        throw std::runtime_error("Non-numeric in numeric comparison");
    }
    static bool valueEquals(const Value& a, const Value& b) {
        if (a.index()!=b.index()) {
            // allow numeric cross-compare
            if ((std::holds_alternative<int64_t>(a)||std::holds_alternative<double>(a)) &&
                (std::holds_alternative<int64_t>(b)||std::holds_alternative<double>(b))) {
                return toNumberPromote(a)==toNumberPromote(b);
            }
            return false;
        }
        return a==b;
    }
    static Value extractLeaf(const ExprNode& n, const DefaultNode& node) {
        if (n.kind!=LEAF) throw std::runtime_error("Expected leaf");
        switch(n.leaf.type) {
            case ASTValue::BOOLEAN: return n.leaf.b;
            case ASTValue::NUMBER:  return n.leaf.num;
            case ASTValue::STRING:  return n.leaf.text;
            case ASTValue::IDENT: {
                const Value* v = nullptr;
                if (n.leaf.text.substr(0, 11) == "properties.") {
                    // Handle property access: properties.propertyName
                    std::string propName = n.leaf.text.substr(11);
                    v = node.getProperty(propName);
                } else {
                    // Handle variable access
                    v = node.getVar(n.leaf.text);
                }
                if (!v) {
                    if (n.leaf.text.substr(0, 11) == "properties.") {
                        throw std::runtime_error("Unknown property: " + n.leaf.text.substr(11));
                    } else {
                        throw std::runtime_error("Unknown var: " + n.leaf.text);
                    }
                }
                return *v;
            }
        }
        throw std::runtime_error("Invalid leaf");
    }
    static Value extractValue(const ExprNode& n, const DefaultNode& node) {
        if (n.kind==LEAF) return extractLeaf(n,node);
        // If non-leaf, treat boolean result as bool Value
        return Value{ n.eval(node) };
    }
};

class Parser {
    Lexer lex;
    Token cur;
public:
    explicit Parser(const std::string& s): lex(s) { cur=lex.next(); }
    std::unique_ptr<ExprNode> parse() { return parseOr(); }
private:
    void consume(TokKind k, const char* msg) {
        if (cur.kind!=k) throw std::runtime_error(msg);
        cur=lex.next();
    }
    bool isOp(const char* op) { return cur.kind==TokKind::OP && cur.text==op; }

    std::unique_ptr<ExprNode> parseOr() {
        auto left = parseAnd();
        while (isOp("||")) {
            std::string op=cur.text; cur=lex.next();
            auto right=parseAnd();
            auto n=std::make_unique<ExprNode>();
            n->kind=ExprNode::OR; n->left=std::move(left); n->right=std::move(right);
            left=std::move(n);
        }
        return left;
    }
    std::unique_ptr<ExprNode> parseAnd() {
        auto left = parseNot();
        while (isOp("&&")) {
            cur=lex.next();
            auto right=parseNot();
            auto n=std::make_unique<ExprNode>();
            n->kind=ExprNode::AND; n->left=std::move(left); n->right=std::move(right);
            left=std::move(n);
        }
        return left;
    }
    std::unique_ptr<ExprNode> parseNot() {
        if (isOp("!")) {
            cur=lex.next();
            auto child=parseNot();
            auto n=std::make_unique<ExprNode>();
            n->kind=ExprNode::NOT; n->left=std::move(child);
            return n;
        }
        return parseCmp();
    }
    std::unique_ptr<ExprNode> parseCmp() {
        auto left = parsePrimary();
        if (cur.kind==TokKind::OP &&
            (cur.text=="=="||cur.text=="!="||cur.text=="<"||cur.text=="<="||cur.text==">"||cur.text==">=")) {
            std::string op=cur.text; cur=lex.next();
            auto right=parsePrimary();
            auto n=std::make_unique<ExprNode>();
            n->kind=ExprNode::CMP; n->cmpOp=op; n->left=std::move(left); n->right=std::move(right);
            return n;
        }
        return left;
    }
    std::unique_ptr<ExprNode> parsePrimary() {
        if (cur.kind==TokKind::LP) {
            cur=lex.next();
            auto e=parseOr();
            consume(TokKind::RP,"Expected ')'");
            return e;
        }
        auto n=std::make_unique<ExprNode>();
        n->kind=ExprNode::LEAF;
        if (cur.kind==TokKind::BOOL) {
            n->leaf.type=ASTValue::BOOLEAN;
            n->leaf.b = (cur.text=="true");
            cur=lex.next();
        } else if (cur.kind==TokKind::NUM) {
            n->leaf.type=ASTValue::NUMBER;
            n->leaf.num = std::stod(cur.text);
            cur=lex.next();
        } else if (cur.kind==TokKind::STR) {
            n->leaf.type=ASTValue::STRING;
            n->leaf.text = cur.text;
            cur=lex.next();
        } else if (cur.kind==TokKind::ID) {
            n->leaf.type=ASTValue::IDENT;
            n->leaf.text=cur.text;
            cur=lex.next();
        } else {
            throw std::runtime_error("Unexpected token in primary");
        }
        return n;
    }
};

inline std::unique_ptr<Expression> compileExpression(const std::string& expr) {
    Parser p(expr);
    return p.parse();
}