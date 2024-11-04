#pragma once

#include <iostream>
#include <map>
#include <string>
#include <variant>
#include <vector>

namespace json {

class Node;
using Dict = std::map<std::string, Node>;
using Array = std::vector<Node>;
using Number = std::variant<int, double>;

// Эта ошибка должна выбрасываться при ошибках парсинга JSON
class ParsingError : public std::runtime_error {
public:
    using runtime_error::runtime_error;
};

class Node final : private std::variant<std::nullptr_t, Array, Dict, bool, int, double, std::string> {
public:

    using variant::variant;
    using Value = variant;

    const Value& GetValue() const;

    bool IsNull() const;
    bool IsArray() const;
    bool IsBool() const;
    bool IsMap() const;
    bool IsDouble() const;
    bool IsPureDouble() const;
    bool IsInt() const;
    bool IsString() const;

    int AsInt() const;
    bool AsBool() const;
    double AsDouble() const;
    const std::string& AsString() const;
    const Array& AsArray() const;
    const Dict& AsMap() const;

    bool operator==(const Node& other) const;
    bool operator!=(const Node& other) const;

};

// Контекст вывода, хранит ссылку на поток вывода и текущий отсуп
struct PrintContext {
    std::ostream& out;
    int indent_step = 4;
    int indent = 0;

    void PrintIndent() const {
        for (int i = 0; i < indent; ++i) {
            out.put(' ');
        }
    }

    // Возвращает новый контекст вывода с увеличенным смещением
    PrintContext Indented() const {
        return {out, indent_step, indent_step + indent};
    }
};

struct NodePrinter {
    PrintContext& ost_;
    void operator()(std::nullptr_t) const;
    void operator()(const Array& array) const;
    void operator()(const Dict dict) const;
    void operator()(const bool boolean) const;
    void operator()(const int integer_number) const;
    void operator()(const double double_number) const;
    void operator()(const std::string& str) const;
};

void PrintNode(const Node& node, PrintContext& out);

class Document {
public:
    explicit Document(Node root);

    const Node& GetRoot() const;

    bool operator==(const Document& other) const ;
    bool operator!=(const Document& other) const ;

private:
    Node root_;
};

Document Load(std::istream& input);

void Print(const Document& doc, std::ostream& output);

}  // namespace json