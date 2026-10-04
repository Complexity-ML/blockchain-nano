#pragma once
#include <cstdint>
#include <functional>
#include <limits>
#include <span>
#include <string_view>
namespace nano::snapshot_detail {
// Match the canonical lower-case 0x encoding without allocating or decoding.
// This is encoding reuse only; it grants no execution-validation exemption.
inline bool canonical_hex_matches(std::string_view encoded,std::span<const uint8_t> bytes,
 const std::function<void()>& cancel={}) {
 if(bytes.size()>(std::numeric_limits<size_t>::max()-2)/2||encoded.size()!=2+2*bytes.size()||!encoded.starts_with("0x"))return false;
 constexpr char digits[]="0123456789abcdef";
 for(size_t i=0;i<bytes.size();++i){
  if(cancel&&i%4096==0)cancel();
  if(encoded[2+2*i]!=digits[bytes[i]>>4]||encoded[3+2*i]!=digits[bytes[i]&15])return false;
 }
 if(cancel)cancel();return true;
}
}
