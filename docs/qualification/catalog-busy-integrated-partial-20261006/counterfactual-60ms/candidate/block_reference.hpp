#pragma once
#include "json.hpp"
#include <algorithm>
inline bool valid_block_reference(const nlohmann::json& value){
 if(!value.is_object()||value.size()!=3||!value.contains("version")||!value.at("version").is_number_integer()||value.at("version")!=1||!value.contains("hash")||!value.at("hash").is_string()||!value.contains("number")||!value.at("number").is_string())return false;
 auto hex=[](const std::string& s){return s.size()>2&&s.starts_with("0x")&&std::all_of(s.begin()+2,s.end(),[](char c){return (c>='0'&&c<='9')||(c>='a'&&c<='f');});};
 const auto& hash=value.at("hash").get_ref<const std::string&>();const auto& number=value.at("number").get_ref<const std::string&>();
 return hash.size()==66&&hex(hash)&&number.size()<=18&&hex(number)&&(number.size()==3||number[2]!='0');
}
