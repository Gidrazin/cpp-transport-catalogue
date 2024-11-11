#pragma once
#include "json.h"
#include <optional>
#include <string>
#include <stack>

namespace json {


class Builder {

	class DictItemContext;
	class ArrayItemContext;

	class BuilderItemContext {
	public:
		BuilderItemContext(Builder& builder);
		Builder& GetBuilder();
	private:
		Builder& builder_;
	};

	class KeyItemContext : public BuilderItemContext {
	public:
		DictItemContext Value(Node::Value);
		DictItemContext StartDict(); 
		ArrayItemContext StartArray();
	};

	class DictItemContext : public BuilderItemContext {
	public:
		KeyItemContext Key(std::string);
		Builder& EndDict();
	};

	class ArrayItemContext : public BuilderItemContext {
	public:
		ArrayItemContext Value(Node::Value);
		DictItemContext StartDict(); 
		ArrayItemContext StartArray();
		Builder& EndArray();
	};

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

private:
	Node root_;
	std::stack<Node*> stack_;
	std::stack<std::pair<char, bool>> braces_;
	std::optional<std::string> key_;
	bool has_value_ = false;
};

} //namespace json
