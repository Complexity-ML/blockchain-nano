#include "nano/consensus.hpp"
#include <array>
namespace nano {
namespace {
// CPython's integer-seeded MT19937 and getrandbits/rejection selection.
// std::mt19937(seed) has different seeding and cannot replace this consensus rule.
class PythonRandom {
 std::array<uint32_t,624> state_{};
 size_t index_=624;
 uint32_t next(){
  if(index_>=624){for(size_t i=0;i<624;++i){uint32_t y=(state_[i]&0x80000000U)|(state_[(i+1)%624]&0x7fffffffU);state_[i]=state_[(i+397)%624]^(y>>1)^((y&1)?0x9908b0dfU:0);}index_=0;}
  uint32_t y=state_[index_++];y^=y>>11;y^=(y<<7)&0x9d2c5680U;y^=(y<<15)&0xefc60000U;y^=y>>18;return y;
 }
public:
 explicit PythonRandom(uint64_t seed){
  uint32_t key[2]={uint32_t(seed),uint32_t(seed>>32)};size_t key_size=key[1]?2:1;
  state_[0]=19650218U;for(size_t i=1;i<624;++i)state_[i]=1812433253U*(state_[i-1]^(state_[i-1]>>30))+uint32_t(i);
  size_t i=1,j=0;for(size_t k=624;k;--k){state_[i]=(state_[i]^((state_[i-1]^(state_[i-1]>>30))*1664525U))+key[j]+uint32_t(j);++i;++j;if(i>=624){state_[0]=state_[623];i=1;}if(j>=key_size)j=0;}
  for(size_t k=623;k;--k){state_[i]=(state_[i]^((state_[i-1]^(state_[i-1]>>30))*1566083941U))-uint32_t(i);++i;if(i>=624){state_[0]=state_[623];i=1;}}state_[0]=0x80000000U;
 }
 Int below(const Int& bound){if(bound<=0)throw std::invalid_argument("invalid random bound");size_t bits=mpz_sizeinbase(bound.get_mpz_t(),2);for(;;){Int candidate=0;for(size_t shift=0;shift<bits;shift+=32){uint32_t x=next();size_t n=std::min(size_t(32),bits-shift);if(n<32)x>>=32-n;candidate|=Int(x)<<shift;}if(candidate<bound)return candidate;}}
};
}
const Int ProofOfStake::minimum_stake("10000000000000000000000");
const Int ProofOfStake::block_reward("2000000000000000000");
Validator* ProofOfStake::find(const std::string& addr){auto normalized=address(addr);for(auto& v:validators_)if(address(v.address)==normalized)return &v;return nullptr;}
bool ProofOfStake::register_validator(const std::string& addr,const Int& stake,const Int& block){address(addr);if(stake<minimum_stake||block<0||find(addr))return false;validators_.push_back({addr,stake,block,Int(block+100)});return true;}
bool ProofOfStake::add_stake(const std::string& addr,const Int& amount,const Int& block){auto* v=find(addr);if(!v||amount<=0||block<0)return false;v->stake+=amount;v->locked_until=block+100;return true;}
std::optional<Int> ProofOfStake::withdraw(const std::string& addr,const Int& block){auto normalized=address(addr);for(auto it=validators_.begin();it!=validators_.end();++it)if(address(it->address)==normalized){if(block<it->locked_until)return std::nullopt;Int value=it->stake;validators_.erase(it);return value;}return std::nullopt;}
std::optional<std::string> ProofOfStake::select(const Int& block,const Bytes& hash)const{if(hash.size()!=32)throw std::invalid_argument("block hash width");auto digest=keccak(concat(hash,bytes(block,8)));Int total=0;for(const auto& v:validators_)if(v.active)total+=v.stake;if(total==0)return bootstrap?std::optional<std::string>(bootstrap_validator):std::nullopt;PythonRandom random(u64(integer(Bytes(digest.begin(),digest.begin()+8))));Int selection=random.below(total),cumulative=0;for(const auto& v:validators_)if(v.active){cumulative+=v.stake;if(selection<cumulative)return v.address;}throw std::logic_error("invalid stake distribution");}
bool ProofOfStake::validate(const Header& h,const std::string& addr)const{auto normalized=address(addr);for(const auto& v:validators_)if(address(v.address)==normalized&&v.active){if(h.number==0)return false;auto selected=select(Int(h.number-1),h.parent_hash);return selected&&address(*selected)==normalized;}return false;}
void ProofOfStake::reward(const std::string& addr,const Int& block){auto* v=find(addr);if(!v)return;if(block<0)throw std::invalid_argument("negative block");v->last_block=block;v->blocks_validated+=1;v->stake+=block_reward;}
void ProofOfStake::slash(const std::string& addr){auto* v=find(addr);if(!v)return;v->stake-=v->stake/10;if(v->stake<minimum_stake)v->active=false;}
Int ProofOfStake::stake(const std::string& addr)const{auto normalized=address(addr);for(const auto& v:validators_)if(address(v.address)==normalized)return v.stake;return 0;}
Int ProofOfStake::total_stake()const{Int total=0;for(const auto& v:validators_)total+=v.stake;return total;}
Json ProofOfStake::validators()const{Json result=Json::array();for(const auto& v:validators_)result.push_back({{"address",v.address},{"stake",quantity(v.stake)},{"blocks_validated",quantity(v.blocks_validated)},{"is_active",v.active}});return result;}
Json ProofOfStake::to_json()const{
 Json records=Json::array();
 for(const auto& v:validators_)records.push_back({{"address",v.address},{"stake",quantity(v.stake)},{"registeredAt",quantity(v.registered_at)},{"lockedUntil",quantity(v.locked_until)},{"lastBlock",quantity(v.last_block)},{"blocksValidated",quantity(v.blocks_validated)},{"active",v.active}});
 return {{"version",1},{"bootstrap",bootstrap},{"validators",records}};
}
ProofOfStake ProofOfStake::from_json(const Json& data){
 if(data.at("version")!=1||!data.at("validators").is_array())throw std::invalid_argument("unsupported validator snapshot");
 ProofOfStake result;result.bootstrap=data.at("bootstrap").get<bool>();
 for(const auto& entry:data.at("validators")){
  Validator v;v.address=entry.at("address").get<std::string>();address(v.address);
  v.stake=quantity(entry.at("stake"));v.registered_at=quantity(entry.at("registeredAt"));v.locked_until=quantity(entry.at("lockedUntil"));v.last_block=quantity(entry.at("lastBlock"));v.blocks_validated=quantity(entry.at("blocksValidated"));v.active=entry.at("active").get<bool>();
  for(const Int* n:{&v.stake,&v.registered_at,&v.locked_until,&v.last_block,&v.blocks_validated})bytes(*n,32);
  if(result.find(v.address)||v.locked_until<v.registered_at||(v.active&&v.stake<minimum_stake))throw std::invalid_argument("invalid validator snapshot");
  result.validators_.push_back(std::move(v));
 }
 return result;
}

}
