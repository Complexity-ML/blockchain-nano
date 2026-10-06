#include "nano/consensus_sync_message.hpp"
namespace nano {
Bytes encode_capella_sync_message(const CapellaSyncMessage& message){
 Bytes out(144);for(unsigned i=0;i<8;++i){out[i]=uint8_t(message.slot>>(8*i));out[40+i]=uint8_t(message.validator_index>>(8*i));}
 std::copy(message.beacon_block_root.begin(),message.beacon_block_root.end(),out.begin()+8);
 std::copy(message.signature.begin(),message.signature.end(),out.begin()+48);return out;
}
std::optional<CapellaSyncMessage> decode_capella_sync_message(std::span<const uint8_t> bytes){
 if(bytes.size()!=144)return std::nullopt;CapellaSyncMessage out;
 for(unsigned i=0;i<8;++i){out.slot|=uint64_t(bytes[i])<<(8*i);out.validator_index|=uint64_t(bytes[40+i])<<(8*i);}
 std::copy_n(bytes.begin()+8,32,out.beacon_block_root.begin());std::copy_n(bytes.begin()+48,96,out.signature.begin());return out;
}
}
