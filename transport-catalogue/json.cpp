#include "json.h"
#include <sstream>
#include <iostream>

using namespace std;

namespace json {

namespace {

Number ParseNumber(std::istream& input) {
    using namespace std::literals;

    std::string parsed_num;

    // Считывает в parsed_num очередной символ из input
    auto read_char = [&parsed_num, &input] {
        parsed_num += static_cast<char>(input.get());
        if (!input) {
            throw ParsingError("Failed to read number from stream"s);
        }
    };

    // Считывает одну или более цифр в parsed_num из input
    auto read_digits = [&input, read_char] {
        if (!std::isdigit(input.peek())) {
            throw ParsingError("A digit is expected"s);
        }
        while (std::isdigit(input.peek())) {
            read_char();
        }
    };

    if (input.peek() == '-') {
        read_char();
    }
    // Парсим целую часть числа
    if (input.peek() == '0') {
        read_char();
        // После 0 в JSON не могут идти другие цифры
    } else {
        read_digits();
    }

    bool is_int = true;
    // Парсим дробную часть числа
    if (input.peek() == '.') {
        read_char();
        read_digits();
        is_int = false;
    }

    // Парсим экспоненциальную часть числа
    if (int ch = input.peek(); ch == 'e' || ch == 'E') {
        read_char();
        if (ch = input.peek(); ch == '+' || ch == '-') {
            read_char();
        }
        read_digits();
        is_int = false;
    }

    try {
        if (is_int) {
            // Сначала пробуем преобразовать строку в int
            try {
                return std::stoi(parsed_num);
            } catch (...) {
                // В случае неудачи, например, при переполнении,
                // код ниже попробует преобразовать строку в double
            }
        }
        return std::stod(parsed_num);
    } catch (...) {
        throw ParsingError("Failed to convert "s + parsed_num + " to number"s);
    }
}

std::string ParseString(std::istream& input) {
    using namespace std::literals;
    
    auto it = std::istreambuf_iterator<char>(input);
    auto end = std::istreambuf_iterator<char>();
    std::string s;
    while (true) {
        if (it == end) {
            // Поток закончился до того, как встретили закрывающую кавычку?
            throw ParsingError("String parsing error");
        }
        const char ch = *it;
        if (ch == '"') {
            // Встретили закрывающую кавычку
            ++it;
            break;
        } else if (ch == '\\') {
            // Встретили начало escape-последовательности
            ++it;
            if (it == end) {
                // Поток завершился сразу после символа обратной косой черты
                throw ParsingError("String parsing error");
            }
            const char escaped_char = *(it);
            // Обрабатываем одну из последовательностей: \\, \n, \t, \r, \"
            switch (escaped_char) {
                case 'n':
                    s.push_back('\n');
                    break;
                case 't':
                    s.push_back('\t');
                    break;
                case 'r':
                    s.push_back('\r');
                    break;
                case '"':
                    s.push_back('"');
                    break;
                case '\\':
                    s.push_back('\\');
                    break;
                default:
                    // Встретили неизвестную escape-последовательность
                    throw ParsingError("Unrecognized escape sequence \\"s + escaped_char);
            }
        } else if (ch == '\n' || ch == '\r') {
            // Строковый литерал внутри- JSON не может прерываться символами \r или \n
            throw ParsingError("Unexpected end of line"s);
        } else {
            // Просто считываем очередной символ и помещаем его в результирующую строку
            s.push_back(ch);
        }
        ++it;
    }

    return s;
}

Node LoadNode(istream& input);

Node LoadNone(istream& input){
    std::string literal;
    char c;
    while (std::isalpha(input.peek())) {
        input >> c;
        literal += c;
    }
    if (literal == "null"s){
        return Node();
    }
    throw ParsingError("Null parsing error"s);
}

Node LoadBool(std::istream& input){
    std::string literal;
    char c;
    while (std::isalpha(input.peek())) {
        input >> c;
        literal += c;
    }
    if (literal == "true"s){
        return Node(true);
    } else if (literal == "false"s){
        return Node(false);
    }
    throw ParsingError("Boolean parsing error"s);
}

Node LoadArray(istream& input) {
    Array result;

    for (char c; input >> c && c != ']';) {
        if (c != ',') {
            input.putback(c);
        }
        result.push_back(LoadNode(input));
    }

    if (!input) {
        throw ParsingError("Array parsing error"s);
    }

    return Node(move(result));
}

Node LoadNumber(istream& input) {
    Number number = ParseNumber(input);
    if (std::holds_alternative<int>(number)){
        return Node(std::get<int>(number));
    }
    return Node(std::get<double>(number));
}

Node LoadString(istream& input) {
    return Node(std::move(ParseString(input)));
}

Node LoadDict(istream& input) {
    Dict result;

    for (char c; input >> c && c != '}';) {
        if (c == ',') {
            input >> c;
        }

        string key = LoadString(input).AsString();
        input >> c;
        result.insert({move(key), LoadNode(input)});
    }

    if (!input) {
        throw ParsingError("Dict parsing error"s);
    }

    return Node(move(result));
}

Node LoadNode(istream& input) {
    char c;
    input >> c;

    if (c == '[') {
        return LoadArray(input);
    } else if (c == '{') {
        return LoadDict(input);
    } else if (c == '"') {
        return LoadString(input);
    } else if (c == 'n') {
        input.putback(c);
        return LoadNone(input);
    } else if (c == 't'){
        input.putback(c);
        return LoadBool(input);
    } else if (c == 'f') {
        input.putback(c);
        return LoadBool(input);
    } else {
        input.putback(c);
        return LoadNumber(input);
    }
}

}  // namespace


//-------------------NODE--------------------

const Node::Value& Node::GetValue() const { return *this; }

bool Node::IsNull() const { return std::holds_alternative<std::nullptr_t>(*this); }
bool Node::IsBool() const { return std::holds_alternative<bool>(*this); }
bool Node::IsInt() const { return std::holds_alternative<int>(*this); }
bool Node::IsDouble() const { return this->IsInt() || std::holds_alternative<double>(*this); }
bool Node::IsPureDouble() const { return std::holds_alternative<double>(*this); }
bool Node::IsString() const { return std::holds_alternative<std::string>(*this); }
bool Node::IsArray() const { return std::holds_alternative<Array>(*this); }
bool Node::IsMap() const { return std::holds_alternative<Dict>(*this); }

bool Node::operator==(const Node& other) const { return *this == other.GetValue(); }
bool Node::operator!=(const Node& other) const { return !(*this == other.GetValue()); }


int Node::AsInt() const {
    try {
        return std::get<int>(GetValue());
    } catch (const std::bad_variant_access& ex) {
        throw std::logic_error("Wrong Type"s);
    }
}

bool Node::AsBool() const {
    try {
        return std::get<bool>(GetValue());
    } catch (const std::bad_variant_access& ex) {
        throw std::logic_error("Wrong Type"s);
    }
}

double Node::AsDouble() const {
    try {
        return std::get<double>(GetValue());
    } catch (const std::bad_variant_access& ex) {
        try {
            return static_cast<int>(std::get<int>(GetValue()));
        } catch (const std::bad_variant_access& ex) {
            throw std::logic_error("Wrong Type"s);
        }
    }
}

const string& Node::AsString() const {
    try {
        return std::get<std::string>(GetValue());
    } catch (const std::bad_variant_access& ex) {
        throw std::logic_error("Wrong Type"s);
    }
}

const Array& Node::AsArray() const {
    try {
        return std::get<Array>(GetValue());
    } catch (const std::bad_variant_access& ex) {
        throw std::logic_error("Wrong Type"s);
    }
}

const Dict& Node::AsMap() const {
    try {
        return std::get<Dict>(GetValue());
    } catch (const std::bad_variant_access& ex) {
        throw std::logic_error("Wrong Type"s);
    }
}


//-------------------NODE_PRINTER---------------


void NodePrinter::operator()(std::nullptr_t) const { ost_.out << "null"s; }
void NodePrinter::operator()(const Array& array) const {
    ost_.out << '[' << '\n';
    ost_.indent += ost_.indent_step;
    ost_.PrintIndent();

    bool first = true;
    for (const Node& node : array){
        if (!first){
            ost_.out << ',' << '\n';
            ost_.PrintIndent();
        }
        PrintNode(node, ost_);
        first = false;
    }
    ost_.out << '\n';
    ost_.indent -= ost_.indent_step;
    ost_.PrintIndent();
    ost_ .out<< ']';
}
void NodePrinter::operator()(const Dict dict) const {
    ost_.out << '{' << '\n';
    ost_.indent += ost_.indent_step;
    ost_.PrintIndent();

    bool first = true;
    for (const auto& [key, value] : dict){
        if (!first){
            ost_.out << ',' << '\n';
            ost_.PrintIndent();
        }
        ost_.out << '"' << key << '"' << ':' << ' ';
        PrintNode(value, ost_);
        first = false;
    }
    ost_.out << '\n';
    ost_.indent -= ost_.indent_step;
    ost_.PrintIndent();
    ost_.out << '}';
}
void NodePrinter::operator()(const bool boolean) const { ost_.out << boolalpha << boolean; }
void NodePrinter::operator()(const int integer_number) const { ost_.out << integer_number; }
void NodePrinter::operator()(const double double_number) const {ost_.out << double_number; }
void NodePrinter::operator()(const std::string& str) const { 
    std:: string literal;
    literal += '"';
    for (char c : str) {
        if (c == '"'){
            literal += '\\';
            literal += '"';
        } else if (c == '\\') {
            literal += '\\';
            literal += '\\';
        } else if (c == '\r') {
            literal += '\\';
            literal += 'r';
        } else if (c == '\n') {
            literal += '\\';
            literal += 'n';
        } else if (c == '\t') {
            literal += '\\';
            literal += 't';
        } else {
            literal += c;
        }
    }
    literal += '"';

    ost_.out << literal;

/*
    ost_ << '"';
    for (char c : str) {
        if (c == '"'){
            ost_ << '\"';
        } else if (c == '\\') {
            ost_ << '\\' << '\\';
        } else if (c == '\r') {
            ost_ << '\\' << 'r';
        } else if (c == '\n') {
            ost_ << '\\' << 'n';
        } else if (c == '\t') {
            ost_ << '\\' << 't';
        } else {
            ost_ << c;
        }
    }
    ost_ << '"';
*/
}


//-------------------DOCUMENT-------------------


Document::Document(Node root): root_(move(root)) {}

const Node& Document::GetRoot() const {
    return root_;
}

bool Document::operator==(const Document& other) const {
    return GetRoot() == other.GetRoot();
}

bool Document::operator!=(const Document& other) const {
    return !(GetRoot() == other.GetRoot());
}

Document Load(istream& input) {
    return Document{LoadNode(input)};
}

void PrintNode(const Node& node, PrintContext& out) {
    std::visit(NodePrinter{out}, node.GetValue());
}

void Print(const Document& doc, std::ostream& output) {
    PrintContext ctx = {output};
    PrintNode(doc.GetRoot(), ctx);
}

}  // namespace json