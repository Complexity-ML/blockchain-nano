#include "nano/crypto.hpp"
#include <set>
#include <regex>
namespace nano {
namespace {
Bytes text_bytes(const std::string& value){return Bytes(value.begin(),value.end());}
struct Type {std::string base;std::vector<size_t> dimensions;};
Type parse_type(const std::string& input){
 static const std::regex syntax("([A-Za-z_$][A-Za-z0-9_$]*)(\\[[0-9]*\\])*");
 if(input.size()>256||!std::regex_match(input,syntax))throw std::invalid_argument("invalid typed data type");
 Type type;auto bracket=input.find('[');type.base=input.substr(0,bracket);
 while(bracket!=std::string::npos){auto end=input.find(']',bracket);auto count=input.substr(bracket+1,end-bracket-1);size_t size=0;
  if(!count.empty()){if(count[0]=='0'||count.size()>5)throw std::invalid_argument("invalid array size");size=std::stoul(count);if(size>10000)throw std::invalid_argument("array size limit");}
  type.dimensions.push_back(size);bracket=input.find('[',end);
 }
 return type;
}
unsigned width(const std::string& type,const std::string& prefix,unsigned limit,unsigned multiple){
 if(!type.starts_with(prefix))return 0;auto suffix=type.substr(prefix.size());
 if(suffix.empty()||suffix[0]=='0'||suffix.size()>3||!std::all_of(suffix.begin(),suffix.end(),[](char c){return c>='0'&&c<='9';}))return 0;
 unsigned n=unsigned(std::stoul(suffix));return n<=limit&&n%multiple==0?n:0;
}
bool primitive(const std::string& type){return type=="address"||type=="bool"||type=="string"||type=="bytes"||width(type,"uint",256,8)||width(type,"int",256,8)||width(type,"bytes",32,1);}
struct Encoder {
 const Json& types;size_t budget=100000;
 explicit Encoder(const Json& definitions):types(definitions){
  if(!types.is_object()||types.empty()||types.size()>256)throw std::invalid_argument("invalid type definitions");
  static const std::regex identifier("[A-Za-z_$][A-Za-z0-9_$]*");
  for(auto it=types.begin();it!=types.end();++it){
   if(!std::regex_match(it.key(),identifier)||primitive(it.key())||!it.value().is_array()||it.value().size()>256)throw std::invalid_argument("invalid struct definition");
   std::set<std::string> names;
   for(const auto& field:it.value()){
    auto name=field.at("name").get<std::string>();if(!std::regex_match(name,identifier)||!names.insert(name).second)throw std::invalid_argument("invalid or duplicate field name");
    auto type=parse_type(field.at("type").get<std::string>());if(!primitive(type.base)&&!types.contains(type.base))throw std::invalid_argument("unknown field type");
   }
  }
 }
 void dependencies(const std::string& name,std::set<std::string>& found){if(!found.insert(name).second)return;for(const auto& field:types.at(name)){auto base=parse_type(field.at("type")).base;if(types.contains(base))dependencies(base,found);}}
 std::string definition(const std::string& name){std::string out=name+"(";bool first=true;for(const auto& field:types.at(name)){if(!first)out+=",";first=false;out+=field.at("type").get<std::string>()+" "+field.at("name").get<std::string>();}return out+")";}
 Bytes struct_hash(const std::string& name,const Json& object,unsigned depth=0){
  if(!object.is_object()||!types.contains(name))throw std::invalid_argument("invalid typed struct value");
  if(depth>64||budget--==0)throw std::invalid_argument("typed data complexity limit");
  std::set<std::string> refs;dependencies(name,refs);refs.erase(name);auto encoded=definition(name);for(const auto& ref:refs)encoded+=definition(ref);
  Bytes data=keccak(text_bytes(encoded));
  for(const auto& field:types.at(name)){auto type=parse_type(field.at("type"));data=concat(std::move(data),value(type,type.dimensions.size(),object.at(field.at("name").get<std::string>()),depth+1));}
  if(object.size()!=types.at(name).size())throw std::invalid_argument("unexpected typed struct field");
  return keccak(data);
 }
 Bytes value(const Type& type,size_t dimensions,const Json& object,unsigned depth){
  if(depth>64||budget--==0)throw std::invalid_argument("typed data complexity limit");
  if(dimensions){auto length=type.dimensions[dimensions-1];if(!object.is_array()||object.size()>10000||(length&&object.size()!=length))throw std::invalid_argument("typed array length mismatch");Bytes data;for(const auto& item:object)data=concat(std::move(data),value(type,dimensions-1,item,depth+1));return keccak(data);}
  const auto& name=type.base;
  if(types.contains(name))return struct_hash(name,object,depth+1);
  if(name=="string")return keccak(text_bytes(object.get<std::string>()));
  if(name=="bytes")return keccak(unhex(object.get<std::string>()));
  if(name=="bool"){if(!object.is_boolean())throw std::invalid_argument("expected boolean");return bytes(Int(object.get<bool>()?1:0),32);}
  if(name=="address")return bytes(integer(unhex(address(object.get<std::string>()))),32);
  if(auto size=width(name,"bytes",32,1)){auto data=unhex(object.get<std::string>());if(data.size()!=size)throw std::invalid_argument("fixed bytes size mismatch");data.resize(32);return data;}
  bool sign=name.starts_with("int");unsigned bits=width(name,sign?"int":"uint",256,8);if(!bits)throw std::invalid_argument("unsupported atomic type");
  Int number;bool negative=object.is_number_integer()&&!object.is_number_unsigned()&&object.get<int64_t>()<0;
  if(object.is_string())negative=object.get<std::string>().starts_with('-');
  if(negative){auto raw=object.is_string()?object.get<std::string>():object.dump();number=-quantity(Json(raw.substr(1)));}else number=quantity(object);
  Int min=sign?Int(-(Int(1)<<(bits-1))):Int(0),max=sign?Int((Int(1)<<(bits-1))-1):Int((Int(1)<<bits)-1);
  if(number<min||number>max)throw std::invalid_argument("typed integer out of range");return bytes(word(number),32);
 }
};
}
Bytes typed_data_hash(const Json& data){
 Encoder encoder(data.at("types"));auto primary=data.at("primaryType").get<std::string>();
 if(primary=="EIP712Domain")throw std::invalid_argument("domain cannot be primary message type");
 auto domain=encoder.struct_hash("EIP712Domain",data.at("domain"));auto message=encoder.struct_hash(primary,data.at("message"));
 return keccak(concat(concat(Bytes{0x19,0x01},domain),message));
}
Bytes sign_typed_data(const Bytes& key,const Json& data){auto signature=sign_hash(key,typed_data_hash(data));return concat(concat(bytes(signature.r,32),bytes(signature.s,32)),Bytes{uint8_t(signature.v+27)});}
}
