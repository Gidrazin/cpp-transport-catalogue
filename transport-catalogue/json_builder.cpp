#include "json_builder.h"
namespace json {

using namespace std::literals;

//----------------BUILDER-----------------

Builder::KeyItemContext Builder::Key(std::string key){
    if (!stack_.top()->IsMap() || key_.has_value()){
        throw std::logic_error("Expected NOT a key here!"s);
    }
    braces_.top().second = true;
    key_ = key;
    has_value_ = false;
    return {*this};
}

Builder& Builder::Value(Node::Value value){
    if (braces_.empty()){
        if (has_value_){
            throw std::logic_error("Expected NOT a value here!");
        }
    } else {
        if (stack_.top()->IsMap() && !key_.has_value()){
            throw std::logic_error("Expected NOT a value here!");
        }
        braces_.top().second = false;
    }

    if (stack_.top()->IsArray()){
        std::get<Array>(stack_.top()->GetValue()).emplace_back(value);
    } else if (stack_.top()->IsMap()) {
        std::get<Dict>(stack_.top()->GetValue()).emplace(key_.value(), value);
        key_ = std::nullopt;
    } else {
        stack_.top()->GetValue() = value;
    }
    has_value_ = true;
    return *this;
}

Builder::DictItemContext Builder::StartDict(){
    if (braces_.empty() && has_value_){
        throw std::logic_error("Expected NOT a Dict here!");
    }
    if (!braces_.empty() && braces_.top().first == '{' && !braces_.top().second){
        throw std::logic_error("Expected NOT a Dict here!");
    }
    if (!braces_.empty() && braces_.top().first == '{' && braces_.top().second){
        braces_.top().second = false;
    }

    if (stack_.top()->IsArray()){
        Array& arr = std::get<Array>(stack_.top()->GetValue());
        auto& new_top = arr.emplace_back(Dict());
        stack_.push(&new_top);
    } else if (stack_.top()->IsMap()) {
        Dict& dict = std::get<Dict>(stack_.top()->GetValue());
        dict[key_.value()] = Dict();
        stack_.push(&dict.at(key_.value()));
        key_ = std::nullopt;
    } else {
        stack_.top()->GetValue() = Dict();
    }
    braces_.push({'{', false});
    has_value_ = true;
    return {*this};
}

Builder::ArrayItemContext Builder::StartArray(){
    if (braces_.empty() && has_value_){
        throw std::logic_error("Expected NOT an Array here!");
    }
    if (!braces_.empty() && braces_.top().first == '{' && !braces_.top().second){
        throw std::logic_error("Expected NOT an Array here!");
    }
    if (!braces_.empty() && braces_.top().first == '{' && braces_.top().second){
        braces_.top().second = false;
    }

    if (stack_.top()->IsArray()){
        Array& arr = std::get<Array>(stack_.top()->GetValue());
        auto& new_top = arr.emplace_back(Array());
        stack_.push(&new_top);
    } else if (stack_.top()->IsMap()) {
        Dict& dict = std::get<Dict>(stack_.top()->GetValue());
        dict[key_.value()] = Array();
        stack_.push(&dict.at(key_.value()));
        key_ = std::nullopt;
    } else {
        stack_.top()->GetValue() = Array();
    }

    braces_.push({'[', false});
    has_value_ = false;
    return {*this};
}

Builder& Builder::EndDict(){
    if (!stack_.top()->IsMap()){
        throw std::logic_error("Not a Dict!");
    }
    braces_.pop();
    stack_.pop();
    has_value_ = true;
    return *this;
}

Builder& Builder::EndArray(){
    if (!stack_.top()->IsArray()){
        throw std::logic_error("Not an Array!");
    }
    braces_.pop();
    stack_.pop();
    has_value_ = true;
    return *this;
}

Node Builder::Build(){
    if (stack_.size() > 1){
        throw std::logic_error("Not ready to build!");
    }
    if (std::holds_alternative<nullptr_t>(root_.GetValue())){
        throw std::logic_error("Nothing to build!!");
    }
    return root_;
}

//---------------BUILDER_ITEM_CONTEXT---------------

Builder::BuilderItemContext::BuilderItemContext(Builder& builder)
: builder_(builder)
{}

Builder& Builder::BuilderItemContext::GetBuilder(){
    return builder_;
}
//----------------DICT_ITEM_CONTEXT-----------------


Builder::KeyItemContext Builder::DictItemContext::Key(std::string key) {
    return GetBuilder().Key(key);
}

Builder& Builder::DictItemContext::EndDict() {
    return GetBuilder().EndDict();
}

//-----------------KEY_ITEM_CONTEXT-----------------

Builder::DictItemContext Builder::KeyItemContext::Value(Node::Value value){
    return {GetBuilder().Value(value)};
}

Builder::DictItemContext Builder::KeyItemContext::StartDict(){
    return GetBuilder().StartDict();
}
Builder::ArrayItemContext Builder::KeyItemContext::StartArray(){
    return GetBuilder().StartArray();
}

//-----------------ARRAY_ITEM_CONTEXT-----------------

Builder::ArrayItemContext Builder::ArrayItemContext::Value(Node::Value value){
    return {GetBuilder().Value(value)};
}
Builder::DictItemContext Builder::ArrayItemContext::StartDict(){
    return GetBuilder().StartDict();
}
Builder::ArrayItemContext Builder::ArrayItemContext::StartArray(){
    return GetBuilder().StartArray();
}
Builder& Builder::ArrayItemContext::EndArray(){
    return GetBuilder().EndArray();
}


} //namespace json