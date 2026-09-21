#pragma once
#include "types.hpp"
namespace nano {
struct Signature { unsigned v=0; Int r=0,s=0; };
Bytes generate_private_key();
Bytes public_key(const Bytes& private_key);
std::string public_address(const Bytes& public_key);
Signature sign_hash(const Bytes& key,const Bytes& hash);
Bytes recover_public_key(const Bytes& hash,const Signature& signature);
Bytes typed_data_hash(const Json& data);
Bytes sign_typed_data(const Bytes& key,const Json& data);
Bytes personal_hash(const Bytes& message);
Bytes sign_message(const Bytes& key,const Bytes& message);
std::string recover_message(const Bytes& message,const Bytes& signature);
Json encrypt_key(const Bytes& key,const std::string& password);
Bytes decrypt_key(const Json& keystore,const std::string& password);
}
