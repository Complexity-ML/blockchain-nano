#include "../src/consensus_capella_snapshot_hex.hpp"
#include <iostream>
#include <vector>
#include <stdexcept>
using nano::snapshot_detail::canonical_hex_matches;
int main(){try{
 auto check=[](bool ok){if(!ok)throw std::runtime_error("canonical encoding mismatch");};
 std::vector<uint8_t> bytes;std::string expected="0x";const std::string digits="0123456789abcdef";
 for(unsigned i=0;i<256;++i){bytes.push_back(i);expected+=digits[i/16];expected+=digits[i%16];}
 check(canonical_hex_matches(expected,bytes));unsigned rejected=0;
 for(size_t i=0;i<bytes.size();++i){auto changed=bytes;changed[i]^=1;check(!canonical_hex_matches(expected,changed));++rejected;}
 for(size_t i=2;i<expected.size();++i){auto changed=expected;changed[i]=changed[i]=='0'?'1':'0';check(!canonical_hex_matches(changed,bytes));++rejected;}
 for(auto text:{std::string("0X")+expected.substr(2),expected+"0",expected.substr(1),expected.substr(0,expected.size()-1)}){check(!canonical_hex_matches(text,bytes));++rejected;}
 auto upper=expected;upper[22]='A';check(!canonical_hex_matches(upper,bytes));++rejected;
 check(canonical_hex_matches("0x",{}));check(!canonical_hex_matches("",{}));
 std::vector<uint8_t> large(16385,0);std::string large_hex="0x"+std::string(large.size()*2,'0');size_t calls=0;
 check(canonical_hex_matches(large_hex,large,[&]{++calls;}));check(calls==6);
 bool cancelled=false;calls=0;try{canonical_hex_matches(large_hex,large,[&]{if(++calls==3)throw std::runtime_error("cancel");});}catch(const std::runtime_error&){cancelled=true;}check(cancelled&&calls==3);
 std::cout<<rejected<<" mutation/format refusals; all byte values, empty input and bounded cancellation passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
