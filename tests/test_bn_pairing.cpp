#include <nlohmann/json.hpp>
#include <charconv>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
namespace nano { bool bn_pairing_check(const std::vector<uint8_t>& input); }
int main(int argc,char** argv){try{
 if(argc!=2)throw std::invalid_argument("expected pairing fixture path");
 std::ifstream file(argv[1]);nlohmann::json vectors;file>>vectors;
 if(!vectors.is_array()||vectors.empty())throw std::invalid_argument("empty pairing fixtures");
 size_t index=0,failures=0;
 for(const auto& vector:vectors){
  const auto input=vector.at("Input").get<std::string>();
  if(input.size()%2)throw std::invalid_argument("odd fixture hex length");
  std::vector<uint8_t> bytes(input.size()/2);
  for(size_t i=0;i<bytes.size();++i){unsigned value=0;auto part=std::from_chars(input.data()+2*i,input.data()+2*i+2,value,16);if(part.ec!=std::errc{}||part.ptr!=input.data()+2*i+2)throw std::invalid_argument("invalid fixture hex");bytes[i]=uint8_t(value);}
  auto expected=vector.at("Expected").get<std::string>();
  if(expected!=std::string(64,'0')&&expected!=std::string(63,'0')+"1")throw std::invalid_argument("invalid pairing expectation");
  bool actual=false;std::string error;
  try{actual=nano::bn_pairing_check(bytes);}catch(const std::exception& e){error=e.what();}
  bool matched=error.empty()&&actual==(expected.back()=='1');if(!matched)++failures;
  std::cout<<nlohmann::json{{"vector",index++},{"inputBytes",bytes.size()},{"expected",expected.back()=='1'},{"actual",actual},{"error",error},{"matched",matched}}.dump()<<'\n';
 }
 std::cout<<nlohmann::json{{"vectors",index},{"failed",failures},{"coldProcess",true}}.dump()<<'\n';return failures?1:0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
