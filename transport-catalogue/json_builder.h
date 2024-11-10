#pragma once
#include "json.h"
#include <optional>
#include <string>
#include <stack>

namespace json {
class KeyItemContext;
class DictItemContext;
class ArrayItemContext;

class Builder {
public:
	Builder(){
		stack_.push(&root_);
	}
	KeyItemContext Key(std::string);
	Builder& Value(Node::Value);
	DictItemContext StartDict(); 
	ArrayItemContext StartArray();
	Builder& EndDict();
	Builder& EndArray();
	Node Build();
	Node& GetValue(Node* node){
		return *node;
	}

private:
	Node root_;
	std::stack<Node*> stack_;
	std::stack<std::pair<char, bool>> braces_;
	std::optional<std::string> key_;
	bool has_value_ = false;
};

class KeyItemContext : public Builder {
public:
	KeyItemContext(Builder& builder);
	KeyItemContext Key(std::string) = delete;
	DictItemContext Value(Node::Value);
	DictItemContext StartDict(); 
	ArrayItemContext StartArray();
	Builder& EndDict() = delete;
	Builder& EndArray() = delete;
	Node Build() = delete;
private:
	Builder& builder_;
};

class DictItemContext : public Builder {
public:
	DictItemContext(Builder& builder);
	KeyItemContext Key(std::string);
	Builder& Value(Node::Value) = delete;
	DictItemContext StartDict() = delete; 
	ArrayItemContext StartArray() = delete;
	Builder& EndDict();
	Builder& EndArray() = delete;
	Node Build() = delete;
private:
	Builder& builder_;
};

class ArrayItemContext : public Builder {
public:
	ArrayItemContext(Builder& builder);
	KeyItemContext Key(std::string) = delete;
	ArrayItemContext Value(Node::Value);
	DictItemContext StartDict(); 
	ArrayItemContext StartArray();
	Builder& EndDict() = delete;
	Builder& EndArray();
	Node Build() = delete;
private:
	Builder& builder_;
};


} //namespace json
