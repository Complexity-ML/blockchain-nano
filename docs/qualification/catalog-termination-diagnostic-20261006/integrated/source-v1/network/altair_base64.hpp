#pragma once
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>
#include <stdexcept>
// Canonical RFC 4648 base64 for local IPC only. Peer framing remains SSZ hex.
inline std::string altair_base64(std::span<const uint8_t> bytes){
 const char* alphabet="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
 if(bytes.size()>SIZE_MAX-2||(bytes.size()+2)/3>SIZE_MAX/4)throw std::length_error("base64 size");
 std::string out;out.reserve((bytes.size()+2)/3*4);
 for(size_t i=0;i<bytes.size();i+=3){const auto n=bytes.size()-i;uint32_t value=uint32_t(bytes[i])<<16;if(n>1)value|=uint32_t(bytes[i+1])<<8;if(n>2)value|=bytes[i+2];out.push_back(alphabet[(value>>18)&63]);out.push_back(alphabet[(value>>12)&63]);out.push_back(n>1?alphabet[(value>>6)&63]:'=');out.push_back(n>2?alphabet[value&63]:'=');}return out;
}
inline std::optional<std::vector<uint8_t>> altair_unbase64(std::string_view text,size_t maximum){
 if(text.size()%4)return std::nullopt;if(text.empty())return std::vector<uint8_t>{};
 const size_t padding=(text.back()=='=')+(text.size()>1&&text[text.size()-2]=='=');
 const size_t count=text.size()/4*3-padding;if(count>maximum)return std::nullopt;
 auto digit=[](char c)->int{if(c>='A'&&c<='Z')return c-'A';if(c>='a'&&c<='z')return c-'a'+26;if(c>='0'&&c<='9')return c-'0'+52;if(c=='+')return 62;if(c=='/')return 63;return -1;};
 // Validate alphabet and unused tail bits BEFORE payload allocation.
 for(size_t i=0;i<text.size()-padding;++i)if(digit(text[i])<0)return std::nullopt;
 if(padding==1&&(digit(text[text.size()-2])&3))return std::nullopt;
 if(padding==2&&(digit(text[text.size()-3])&15))return std::nullopt;
 std::vector<uint8_t> out;out.reserve(count);
 for(size_t i=0;i<text.size();i+=4){uint32_t value=0;for(size_t j=0;j<4;++j)value=(value<<6)|(text[i+j]=='='?0:digit(text[i+j]));out.push_back(uint8_t(value>>16));if(out.size()<count)out.push_back(uint8_t(value>>8));if(out.size()<count)out.push_back(uint8_t(value));}return out;
}
