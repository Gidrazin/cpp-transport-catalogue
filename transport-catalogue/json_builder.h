#pragma once
#include "json.h"
#include <optional>
#include <string>
#include <stack>

namespace json {


class Builder {

	class BuilderItemContext;
	class DictItemContext;
	class ArrayItemContext;
	class KeyItemContext;

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
		KeyItemContext Key(std::string) = delete;
		Builder& EndDict() = delete;
		Builder& EndArray() = delete;
		Node Build()= delete;
	};

	class DictItemContext : public BuilderItemContext {
	public:
		KeyItemContext Key(std::string);
		Builder& EndDict();
		Builder& Value(Node::Value) = delete;
		DictItemContext StartDict() = delete;
		ArrayItemContext StartArray() = delete;
		Builder& EndArray() = delete;
		Node Build() = delete;
	};

	class ArrayItemContext : public BuilderItemContext {
	public:
		ArrayItemContext Value(Node::Value);
		DictItemContext StartDict(); 
		ArrayItemContext StartArray();
		Builder& EndArray();
		KeyItemContext Key(std::string) = delete;
		Builder& EndDict() = delete;
		Node Build() = delete;
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
