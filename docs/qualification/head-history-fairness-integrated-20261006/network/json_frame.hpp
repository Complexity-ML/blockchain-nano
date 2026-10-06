#pragma once
#include <array>
#include <cstddef>
#include <stdexcept>
#include <string_view>

// Framing only; complete documents must still pass the JSON parser. This
// bounds work for fragmented input and rejects nesting before recursive parse.
class JsonFrame {
 std::array<char,64> stack_{};
 size_t depth_=0,size_=0,limit_;
 bool started_=false,complete_=false,string_=false,escaped_=false;
 static bool space(char ch){return ch==' '||ch=='\t'||ch=='\r'||ch=='\n';}
public:
 static constexpr size_t maximum_size=16*1024*1024;
 explicit JsonFrame(size_t limit=maximum_size):limit_(limit){}
 bool feed(std::string_view chunk){
  if(chunk.size()>limit_-size_)throw std::invalid_argument("JSON frame too large");
  size_+=chunk.size();
  for(char ch:chunk){
   if(complete_){if(!space(ch))throw std::invalid_argument("trailing JSON frame data");continue;}
   if(!started_){if(space(ch))continue;if(ch!='{')throw std::invalid_argument("JSON frame must be an object");started_=true;}
   if(string_){if(escaped_)escaped_=false;else if(ch=='\\')escaped_=true;else if(ch=='\"')string_=false;continue;}
   if(ch=='\"'){string_=true;continue;}
   if(ch=='{'||ch=='['){if(depth_==stack_.size())throw std::invalid_argument("JSON frame too deeply nested");stack_[depth_++]=ch;}
   else if(ch=='}'||ch==']'){
    if(!depth_||stack_[depth_-1]!=(ch=='}'?'{':'['))throw std::invalid_argument("unbalanced JSON frame");
    if(--depth_==0)complete_=true;
   }
  }
  return complete_;
 }
};
