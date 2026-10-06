#include "json_frame.hpp"
#include "json.hpp"
#include <iostream>
#include <string>
using Json=nlohmann::json;
void check(bool ok,const char* text){if(!ok)throw std::runtime_error(text);}
int main(){try{
 const auto document=Json{{"text","a\\\"}[]{b"},{"array",Json::array({1,Json{{"x",true}},nullptr})}}.dump();
 for(size_t split=0;split<=document.size();++split){JsonFrame frame;bool first=frame.feed(std::string_view(document).substr(0,split));check(first==(split==document.size()),"completion before closing object");check(frame.feed(std::string_view(document).substr(split)),"split frame completion");}
 JsonFrame fragmented;for(size_t i=0;i<document.size();++i)check(fragmented.feed(std::string_view(document).substr(i,1))==(i+1==document.size()),"single byte fragments including escaped quotes");
 auto reject=[](const std::string& value){JsonFrame frame;bool failed=false;try{frame.feed(value);}catch(const std::exception&){failed=true;}check(failed,"invalid framing accepted");};
 reject("[]");reject("null");reject("{]");reject("{}{}");reject("{}x");
 std::string deep="{\"x\":";deep+=std::string(64,'[');deep+="0";deep+=std::string(64,']');deep+="}";reject(deep);
 std::string allowed="{\"x\":";allowed+=std::string(63,'[');allowed+="0";allowed+=std::string(63,']');allowed+="}";JsonFrame bounded;check(bounded.feed(allowed)&&Json::parse(allowed).is_object(),"64-level boundary");
 JsonFrame trailing;check(trailing.feed(" {} ")&&trailing.feed("\n\t"),"JSON whitespace accepted");
 JsonFrame partial;check(!partial.feed("{\"x\":\"unfinished"),"partial string stays incomplete");
 JsonFrame grammar;check(grammar.feed("{broken}")&&Json::parse("{broken}",nullptr,false).is_discarded(),"framing does not replace JSON grammar validation");
 JsonFrame small(8);check(small.feed("{\"a\":1} "),"exact byte limit");bool failed=false;try{small.feed(" ");}catch(const std::exception&){failed=true;}check(failed,"fragmented byte limit enforced");
 std::string large="{\"data\":\""+std::string(1024*1024,'a')+"\"}";JsonFrame linear;
 for(size_t i=0;i<large.size();i+=31){auto chunk=std::string_view(large).substr(i,31);check(linear.feed(chunk)==(i+chunk.size()==large.size()),"large fragmented frame");}
 std::cout<<"JSON framing bounds and fragmented strings passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
