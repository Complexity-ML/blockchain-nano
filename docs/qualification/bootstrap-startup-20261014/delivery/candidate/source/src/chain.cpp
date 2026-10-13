#include "nano/chain.hpp"
#include "nano/staking.hpp"
#include "nano/block_auth.hpp"
#include "nano/crypto.hpp"
#include <set>
#include <openssl/rand.h>
namespace nano {
Genesis Genesis::from_json(const Json& data){
 Genesis g;auto config=data.value("config",Json::object());
 g.chain_id=quantity(config.value("chainId",Json(1337)));bytes(g.chain_id,32);
 if(config.contains("evmFork")){if(config.at("evmFork")=="Paris")g.evm_revision=EvmRevision::Paris;else if(config.at("evmFork")=="Shanghai")g.evm_revision=EvmRevision::Shanghai;else throw std::invalid_argument("unsupported chain evmFork; expected Paris or Shanghai");}
 g.consensus=config.value("consensus",std::string("pow"));
 if(g.consensus!="pow"&&g.consensus!="pos"&&g.consensus!="beacon")throw std::invalid_argument("unknown consensus");
 if(g.consensus=="beacon"){
  if((g.evm_revision!=EvmRevision::Paris&&g.evm_revision!=EvmRevision::Shanghai)||g.chain_id<=0)throw std::invalid_argument("Beacon execution requires explicit Paris or Shanghai profile and positive chain ID");
  for(const char* field:{"stakingContract","posSignatureVersion","bootstrapValidator","bftVersion","consensusRules","stakingQueryRules"})
   if(config.contains(field))throw std::invalid_argument("Beacon genesis cannot contain legacy consensus parameters");
 }
 g.staking_contract=config.value("stakingContract",std::string());if(!g.staking_contract.empty()){g.staking_contract=address(g.staking_contract);if(g.consensus!="pos")throw std::invalid_argument("staking contract requires PoS");}
 auto version=config.value("posSignatureVersion",Json(0));
 if(!version.is_number_integer()||(version!=0&&version!=1))throw std::invalid_argument("unsupported PoS signature version");
 g.pos_signature_version=version.get<unsigned>();
 if(g.pos_signature_version){
  if(g.consensus!="pos"||g.chain_id<=0)throw std::invalid_argument("signed blocks require PoS and positive chain ID");
  g.bootstrap_validator=address(config.at("bootstrapValidator").get<std::string>());
  if(g.bootstrap_validator==address("0x0000000000000000000000000000000000000000"))throw std::invalid_argument("bootstrap validator cannot be zero");
 }
 if(config.contains("bftVersion")){
  const auto& version=config.at("bftVersion");
  if(!version.is_number_integer()||version!=1||g.consensus!="pos"||g.pos_signature_version!=1)throw std::invalid_argument("bftVersion 1 requires signed PoS");
  g.bft_version=1;
 }
 if(config.contains("consensusRules")){
  const auto& rules=config.at("consensusRules");
  if(!rules.is_object()||rules.size()!=4||!rules.contains("version")||!rules.at("version").is_number_integer()||rules.at("version")!=1||!g.pos_signature_version)throw std::invalid_argument("consensusRules version 1 requires signed PoS and four explicit fields");
  for(const char* field:{"minimumBlockInterval","maximumFutureDrift","minimumGasLimit"})if(!rules.contains(field)||!rules.at(field).is_number_integer())throw std::invalid_argument("consensusRules parameters must be integers");
  g.consensus_rules_version=1;g.minimum_block_interval=quantity(rules.at("minimumBlockInterval"));g.maximum_future_drift=quantity(rules.at("maximumFutureDrift"));g.minimum_gas_limit=quantity(rules.at("minimumGasLimit"));
  if(g.minimum_block_interval<1||g.minimum_block_interval>3600||g.maximum_future_drift<0||g.maximum_future_drift>3600||g.minimum_gas_limit<5000)throw std::invalid_argument("invalid consensusRules bounds");
  u64(g.minimum_gas_limit);
 }
 if(config.contains("stakingQueryRules")){
  const auto& rules=config.at("stakingQueryRules");
  if(!rules.is_object()||!rules.contains("version")||!rules.at("version").is_number_integer()||(rules.at("version")!=1&&rules.at("version")!=2)||!g.pos_signature_version||g.staking_contract.empty())throw std::invalid_argument("stakingQueryRules requires supported version and signed staking");
  if(rules.size()!=(rules.at("version")==1?3:4))throw std::invalid_argument("unexpected staking query fields");
  if(rules.at("version")==2){if(!rules.contains("pageSize")||!rules.at("pageSize").is_number_integer())throw std::invalid_argument("pageSize must be integer");g.staking_query_limits.page_size=u64(quantity(rules.at("pageSize")));if(!g.staking_query_limits.page_size)throw std::invalid_argument("pageSize must be positive");}
  for(const char* field:{"perCallGas","syncGas"})if(!rules.contains(field)||!rules.at(field).is_number_integer())throw std::invalid_argument("staking query gas must be integer");
  g.staking_query_limits.per_call_gas=u64(quantity(rules.at("perCallGas")));g.staking_query_limits.sync_gas=u64(quantity(rules.at("syncGas")));g.staking_query_limits.validate();g.staking_query_version=rules.at("version").get<unsigned>();
 }
 g.state=State::from_json(data.value("alloc",Json::object()));
 auto& h=g.block.header;h.state_root=g.state.root();
 h.difficulty=quantity(data.value("difficulty",Json(g.consensus=="beacon"?"0x0":"0x1")));
 h.gas_limit=quantity(data.value("gasLimit",Json("0x1c9c380")));
 if(data.contains("baseFeePerGas"))h.base_fee=quantity(data.at("baseFeePerGas"));
 if(data.contains("evm_rules")){
  const auto& rules=data.at("evm_rules");
  if(!rules.is_object()||!rules.contains("initial_base_fee")||!rules.contains("target_gas_per_block")||rules.size()!=(rules.contains("version")?3:2))throw std::invalid_argument("evm_rules requires initial_base_fee and target_gas_per_block");
  if(rules.contains("version")&&(!rules.at("version").is_number_integer()||rules.at("version")!=1))throw std::invalid_argument("unsupported evm_rules version");
  auto initial=quantity(rules.at("initial_base_fee"));auto target=quantity(rules.at("target_gas_per_block"));
  if(initial<0||initial>=(Int(1)<<256)||target<=0||target*2!=h.gas_limit)throw std::invalid_argument("invalid EIP-1559 genesis fee or target; gasLimit must equal twice target_gas_per_block");
  if(data.contains("baseFeePerGas")&&h.base_fee!=std::optional<Int>(initial))throw std::invalid_argument("conflicting initial base fees in genesis");
  h.base_fee=initial;
 }
 if(*h.base_fee<0||*h.base_fee>=(Int(1)<<256))throw std::invalid_argument("invalid genesis base fee");
 h.timestamp=quantity(data.value("timestamp",Json("0x0")));
 h.nonce=bytes(quantity(data.value("nonce",Json("0x0"))),8);
 h.extra_data=unhex(data.value("extraData",std::string("0x4e616e6f5079")));
 if(h.extra_data.size()>32)h.extra_data.resize(32); // Source genesis convention.
 if((g.consensus=="beacon"?h.difficulty!=0:h.difficulty<1)||h.gas_limit<g.minimum_gas_limit)throw std::invalid_argument("invalid genesis limits");
 if(g.consensus=="beacon"){
  if(h.nonce!=Bytes(8))throw std::invalid_argument("Beacon genesis nonce must be zero");
  const std::string domain=g.evm_revision==EvmRevision::Shanghai?"Nano/Beacon/Capella/v1":"Nano/Beacon/Bellatrix/v1";
  if(g.evm_revision==EvmRevision::Shanghai){
   h.withdrawals_root=withdrawals_root(g.block.withdrawals);
   if(data.contains("withdrawals")&&data.at("withdrawals")!=Json::array())throw std::invalid_argument("Capella genesis withdrawals must be empty");
   if(data.contains("withdrawalsRoot")&&data.at("withdrawalsRoot")!=hex(*h.withdrawals_root))throw std::invalid_argument("Capella genesis withdrawals root mismatch");
  }
  h.extra_data=keccak(Rlp(std::vector<Rlp>{Rlp(Bytes(domain.begin(),domain.end())),Rlp(g.chain_id),Rlp(h.extra_data)}).encode());
 }
 if(g.pos_signature_version){const std::string domain="Nano/PoS/Genesis";h.extra_data=keccak(Rlp(std::vector<Rlp>{Rlp(Bytes(domain.begin(),domain.end())),Rlp(Int(g.pos_signature_version)),Rlp(g.chain_id),Rlp(unhex(g.bootstrap_validator)),Rlp(g.staking_contract.empty()?Bytes():unhex(g.staking_contract)),Rlp(h.extra_data)}).encode());}
 if(g.consensus_rules_version){const std::string domain="Nano/Consensus/Rules";h.extra_data=keccak(Rlp(std::vector<Rlp>{Rlp(Bytes(domain.begin(),domain.end())),Rlp(Int(1)),Rlp(g.minimum_block_interval),Rlp(g.maximum_future_drift),Rlp(g.minimum_gas_limit),Rlp(h.extra_data)}).encode());}
 if(g.staking_query_version){const std::string domain="Nano/Staking/QueryRules";h.extra_data=keccak(Rlp(std::vector<Rlp>{Rlp(Bytes(domain.begin(),domain.end())),Rlp(Int(g.staking_query_version)),Rlp(Int(std::to_string(g.staking_query_limits.per_call_gas))),Rlp(Int(std::to_string(g.staking_query_limits.sync_gas))),Rlp(h.extra_data)}).encode());}
 if(g.staking_query_version==2){const std::string domain="Nano/Staking/Pagination";h.extra_data=keccak(Rlp(std::vector<Rlp>{Rlp(Bytes(domain.begin(),domain.end())),Rlp(Int(std::to_string(g.staking_query_limits.page_size))),Rlp(h.extra_data)}).encode());}
 if(g.bft_version){const std::string domain="Nano/BFT/Rules";h.extra_data=keccak(Rlp(std::vector<Rlp>{Rlp(Bytes(domain.begin(),domain.end())),Rlp(Int(g.bft_version)),Rlp(h.extra_data)}).encode());}
 if(g.evm_revision==EvmRevision::Paris||g.evm_revision==EvmRevision::Shanghai){const std::string domain=g.evm_revision==EvmRevision::Paris?"Nano/EVM/Paris/v1":"Nano/EVM/Shanghai/v1";h.extra_data=keccak(Rlp(std::vector<Rlp>{Rlp(Bytes(domain.begin(),domain.end())),Rlp(h.extra_data)}).encode());}
 h.encode();return g;
}
namespace {
ChainStore::OpenMode blockchain_open_mode(ChainStore::OpenMode mode){
 if(mode!=ChainStore::OpenMode::ReadWrite&&mode!=ChainStore::OpenMode::ReadWriteExisting)
  throw std::invalid_argument("Blockchain owner requires ReadWrite or ReadWriteExisting");
 return mode;
}
}
Blockchain::Blockchain(const std::filesystem::path& database,Genesis genesis)
 :Blockchain(database,std::move(genesis),ChainStore::OpenMode::ReadWrite){}
Blockchain::Blockchain(const std::filesystem::path& database,Genesis genesis,ChainStore::OpenMode mode)
 :genesis_(std::move(genesis)),store_(database,blockchain_open_mode(mode)),engine_(genesis_.chain_id,[this](const Int& number){auto b=store_.block_by_number(number);if(!b)throw std::runtime_error("missing ancestor block");return b->header.hash();},genesis_.evm_revision){
 Bytes epoch(16);if(RAND_bytes(epoch.data(),int(epoch.size()))!=1)throw std::runtime_error("gossip session randomness failed");event_epoch_=hex(epoch);
 if(mode==ChainStore::OpenMode::ReadWriteExisting)
  store_.require_existing_chain_binding(genesis_.chain_id,genesis_.block.header.hash(),genesis_.consensus,genesis_.staking_contract);
 else store_.bind_chain(genesis_.chain_id,genesis_.block.header.hash(),genesis_.consensus,genesis_.staking_contract);
 auto latest=store_.latest();
 if(!latest){
  if(mode==ChainStore::OpenMode::ReadWriteExisting)throw std::runtime_error("existing Blockchain database has no canonical tip");
  pos_=sync_consensus(genesis_.state,genesis_.block.header,pos_);store_.commit(genesis_.block,genesis_.state,{},genesis_.consensus=="pos"?std::optional<Json>(pos_.to_json()):std::nullopt);tip_=genesis_.block;state_=genesis_.state;}
 else{
  auto first=store_.block_by_number(0);
  if(!first||first->header.hash()!=genesis_.block.header.hash())throw std::invalid_argument("genesis does not match database");
  auto persisted=store_.state_at(latest->header.number);
  if(!persisted)throw std::runtime_error("missing canonical state");
  if(latest->header.number!=0&&genesis_.consensus!="beacon")verify_authentication(*latest);
  if(genesis_.bft_version&&latest->header.number!=0&&!store_.certificate_at(latest->header.number))throw std::runtime_error("BFT chain tip lacks certificate");
  tip_=std::move(*latest);state_=std::move(*persisted);
  if(genesis_.consensus=="pos"){
   auto consensus=store_.consensus_at(tip_.header.number);
   if(consensus)pos_=ProofOfStake::from_json(*consensus);
   else if(tip_.header.number!=0)throw std::runtime_error("missing persisted validator registry");
  }
 }
}
ProofOfStake Blockchain::sync_consensus(const State& state,const Header& header,const ProofOfStake& previous)const{
 return sync_staking_contract(state,header,genesis_.staking_contract,previous,genesis_.chain_id,[this](const Int& number){auto block=store_.block_by_number(number);if(!block)throw std::runtime_error("missing staking ancestor");return block->header.hash();},genesis_.evm_revision,genesis_.staking_query_limits);
}
void Blockchain::emit(const std::string& type,Json value){events_.push_back({++event_sequence_,type,std::move(value)});if(events_.size()>10000)events_.pop_front();}
uint64_t Blockchain::event_cursor()const{std::lock_guard lock(mutex_);return event_sequence_;}
std::vector<ChainEvent> Blockchain::events_since(uint64_t cursor)const{
 std::lock_guard lock(mutex_);if(!events_.empty()&&cursor<events_.front().sequence-1)throw std::runtime_error("event history expired");
 std::vector<ChainEvent> result;for(const auto& event:events_)if(event.sequence>cursor)result.push_back(event);return result;
}
Json Blockchain::block_reference_page(uint64_t cursor,const std::string& epoch)const{
 std::lock_guard lock(mutex_);if(epoch.size()>34)throw std::invalid_argument("oversized event epoch");
 const bool reset=epoch!=event_epoch_||cursor>event_sequence_||(!events_.empty()&&(cursor<events_.front().sequence-1||(events_.front().type=="reset"&&cursor<events_.front().sequence)));
 auto reference=[](const Json& header){return Json{{"type","block"},{"value",{{"version",1},{"hash",header.at("hash")},{"number",header.at("number")}}}};};
 Json result=Json::array();
 if(reset){result.push_back(reference(tip_.header.to_json()));cursor=event_sequence_;}
 else for(const auto& event:events_)if(event.sequence>cursor){
  if(result.size()>=50)break;
  if(event.type=="block")result.push_back(reference(event.value));
  cursor=event.sequence;
 }
 return {{"version",1},{"epoch",event_epoch_},{"reset",reset},{"cursor",cursor},{"events",std::move(result)}};
}
Json Blockchain::gossip_page(uint64_t cursor,const std::string& epoch,const std::string& after,bool snapshot)const{
 if(epoch.size()>34||after.size()>66)throw std::invalid_argument("oversized gossip cursor");
 std::lock_guard lock(mutex_);
 bool reset=epoch!=event_epoch_||cursor>event_sequence_||(!snapshot&&!events_.empty()&&(cursor<events_.front().sequence-1||(events_.front().type=="reset"&&cursor<events_.front().sequence)));
 if(reset||snapshot){
  auto page=pool_.page(reset?"":after);page["mode"]="inventory";page["epoch"]=event_epoch_;
  // Capture the event barrier only on the first page. Inserts behind the hash
  // cursor during pagination must remain visible in the subsequent event tail.
  page["cursor"]=(reset||after.empty())?event_sequence_:cursor;return page;
 }
 Json transactions=Json::array();size_t size=0,scanned=0;
 for(const auto& event:events_){if(event.sequence<=cursor)continue;if(scanned>=50)break;
  if(event.type=="pending"){auto tx=pool_.get(event.value.get<std::string>());if(tx){auto raw=hex(tx->raw());if(size+raw.size()>8*1024*1024)break;size+=raw.size();transactions.push_back(std::move(raw));}}
  cursor=event.sequence;++scanned;
 }
 return {{"mode","events"},{"epoch",event_epoch_},{"cursor",cursor},{"next",nullptr},{"transactions",transactions}};
}
bool Blockchain::restore(const Int& number,const Bytes& expected_hash){
 if(genesis_.consensus=="beacon")throw std::invalid_argument("Beacon rewind requires consensus fork choice");
 std::lock_guard lock(mutex_);auto block=store_.block_by_number(number);if(!block||block->header.hash()!=expected_hash)return false;
 auto state=store_.state_at(number);if(!state)throw std::runtime_error("snapshot state unavailable");
 auto registry=pos_;if(genesis_.consensus=="pos"){auto saved=store_.consensus_at(number);if(!saved)throw std::runtime_error("snapshot validator registry unavailable");registry=ProofOfStake::from_json(*saved);}
 audited_certified_tip_.reset();store_.rewind(number);state_=std::move(*state);tip_=std::move(*block);pos_=std::move(registry);pool_.clear();events_.clear();emit("reset",nullptr);return true;
}
Block Blockchain::tip()const{std::lock_guard lock(mutex_);require_execution_ready_locked();return tip_;}
Header Blockchain::tip_header()const{std::lock_guard lock(mutex_);require_execution_ready_locked();return tip_.header;}
State Blockchain::state()const{std::lock_guard lock(mutex_);require_execution_ready_locked();return state_;}
bool Blockchain::submit(const SignedTransaction& tx){
 std::lock_guard lock(mutex_);require_execution_ready_locked();
 // Local admission policy, separate from consensus decoding of signed envelopes.
 if(tx.raw().size()>1024*1024)throw std::invalid_argument("transaction exceeds Nano pool byte limit");
 const auto& t=tx.tx;if(t.type>2)throw std::invalid_argument("transaction type unavailable on Nano network");auto sender=tx.sender();
 if(t.chain_id!=genesis_.chain_id)throw std::invalid_argument("wrong chain ID");
 const auto& account=static_cast<const State&>(state_).get(sender);
 if((genesis_.evm_revision==EvmRevision::Paris||genesis_.evm_revision==EvmRevision::Shanghai)&&!account.code.empty())throw std::invalid_argument("transaction sender has deployed code");
 if(t.nonce>=Int("18446744073709551615"))throw std::invalid_argument("transaction nonce at or above maximum");
 if(t.nonce<account.nonce)throw std::invalid_argument("transaction nonce already mined");
 Int price=t.type==2?t.max_fee:t.gas_price;
 if(t.gas<transaction_intrinsic_gas(t,genesis_.evm_revision)||t.gas>tip_.header.gas_limit||t.value<0||price<0||account.balance<t.value+t.gas*price)throw std::invalid_argument("transaction exceeds funds or gas limits");
 if(t.type==2&&(t.priority_fee<0||t.priority_fee>t.max_fee))throw std::invalid_argument("invalid priority fee");
 if(account.balance<pool_.reserved_cost(sender,t.nonce,account.nonce)+t.value+t.gas*price)throw std::invalid_argument("transaction exceeds cumulative pending funds");
 if(store_.transaction(tx.hash()))return false;
 if(!pool_.add(tx))return false;emit("pending",hex(tx.hash()));return true;
}
size_t Blockchain::pending_count()const{std::lock_guard lock(mutex_);return pool_.size();}
Int Blockchain::pending_nonce(const std::string& addr)const{std::lock_guard lock(mutex_);Int nonce=state_.get(addr).nonce;for(const auto& tx:pool_.by_sender(addr)){if(tx.tx.nonce==nonce)++nonce;else if(tx.tx.nonce>nonce)break;}return nonce;}
std::optional<SignedTransaction> Blockchain::pending_transaction(const Bytes& hash)const{std::lock_guard lock(mutex_);return pool_.get(hex(hash));}
Json Blockchain::pool_info(bool content)const{std::lock_guard lock(mutex_);return pool_.classified(content,state_);}

void Blockchain::validate_uncles(const Block& block,const Int& now)const{
 if(block.uncles.size()>2)throw std::invalid_argument("too many uncles");
 if(block.uncles.empty())return;
 std::map<Bytes,Header> ancestors;std::set<Bytes> rewarded;
 Int number=tip_.header.number;
 for(unsigned depth=0;depth<7;++depth){
  auto ancestor=store_.block_by_number(number);if(!ancestor)throw std::runtime_error("missing uncle ancestry");
  ancestors.emplace(ancestor->header.hash(),ancestor->header);
  for(const auto& uncle:ancestor->uncles)rewarded.insert(uncle.hash());
  if(number==0)break;--number;
 }
 for(const auto& uncle:block.uncles){
  auto hash=uncle.hash();Int depth=block.header.number-uncle.number;
  if(depth<1||depth>6)throw std::invalid_argument("invalid uncle depth");
  if(!rewarded.insert(hash).second)throw std::invalid_argument("uncle already included");
  if(ancestors.contains(hash))throw std::invalid_argument("uncle is canonical ancestor");
  auto parent=ancestors.find(uncle.parent_hash);
  if(parent==ancestors.end()||uncle.parent_hash==tip_.header.hash())throw std::invalid_argument("uncle parent is not eligible ancestor");
  auto validation=validate_pow_header(uncle,parent->second,now);
  if(!validation.valid)throw std::invalid_argument("invalid uncle header: "+validation.error);
  if(uncle.base_fee!=std::optional<Int>(next_base_fee(parent->second)))throw std::invalid_argument("invalid uncle base fee");
 }
}
std::optional<std::string> Blockchain::selected_validator(uint64_t round)const{
 if(round&&!genesis_.bft_version)throw std::invalid_argument("round rotation requires BFT mode");
 auto selected=genesis_.pos_signature_version&&pos_.bootstrap&&pos_.validators().empty()?std::optional<std::string>(genesis_.bootstrap_validator):pos_.select(tip_.header.number,tip_.header.hash());
 if(!round||!selected)return selected;
 auto members=voting_set(pos_);std::sort(members.begin(),members.end(),[](const auto& a,const auto& b){return address(a.first)<address(b.first);});
 auto first=std::find_if(members.begin(),members.end(),[&](const auto& member){return address(member.first)==address(*selected);});
 if(first==members.end())throw std::runtime_error("initial proposer outside validator set");
 auto index=size_t(first-members.begin());return members[(index+round%members.size())%members.size()].first;
}
void Blockchain::verify_authentication(const Block& block)const{
 if(genesis_.consensus=="beacon")throw std::invalid_argument("Beacon block authentication requires signed consensus envelope");
 if(genesis_.pos_signature_version){if(!verify_block_signature(block,genesis_.chain_id,genesis_.block.header.hash()))throw std::invalid_argument("invalid or missing PoS producer signature");}
 else if(!block.producer_signature.empty())throw std::invalid_argument("producer signature is not enabled on this chain");
}
void Blockchain::reserve_signature(const Block& block,const std::string& signer){
 std::lock_guard lock(mutex_);
 const Int now(std::to_string(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count()));
 auto checked=verify_contents(block,now);
 auto next_pos=pos_;next_pos.reward(block.header.coinbase,block.header.number);sync_consensus(checked.state,block.header,next_pos);
 if(!genesis_.pos_signature_version||address(signer)!=address(block.header.coinbase))throw std::invalid_argument("invalid signed producer");
 store_.reserve_signature(genesis_.chain_id,genesis_.block.header.hash(),signer,block.header.number,block_signing_hash(block,genesis_.chain_id,genesis_.block.header.hash()),block,genesis_.bft_version?std::optional<uint64_t>(bft_proposal_round(block)):std::nullopt);
}
Block Blockchain::resume_signing_proposal(const Block& candidate,const std::string& signer)const{
 auto saved=store_.signing_proposal(genesis_.chain_id,genesis_.block.header.hash(),signer,candidate.header.number,genesis_.bft_version?std::optional<uint64_t>(bft_proposal_round(candidate)):std::nullopt);
 return saved?*saved:candidate;
}
Block Blockchain::validate_signed(const Bytes& key,const Int& timestamp){
 if(genesis_.bft_version)throw std::invalid_argument("BFT requires quorum certification, not direct production");
 if(genesis_.consensus!="pos"||!genesis_.pos_signature_version)throw std::invalid_argument("signed PoS is not enabled");
 auto block=resume_signing_proposal(block_template(public_address(public_key(key)),timestamp),public_address(public_key(key)));
 reserve_signature(block,public_address(public_key(key)));sign_block(block,key,genesis_.chain_id,genesis_.block.header.hash());import_block(block,timestamp);return block;
}
Blockchain::Prepared Blockchain::verify_contents(const Block& block,const Int& now)const{
 if(genesis_.consensus=="beacon")throw std::invalid_argument("Beacon block import requires consensus admission");
 auto validation=genesis_.consensus=="pow"?validate_pow_header(block.header,tip_.header,now):HeaderValidation{true,{}};
 if(genesis_.consensus=="pos"){
  // PoS uses the same structural bounds but has no PoW seal.
  auto h=block.header;h.encode();
  if(h.number!=tip_.header.number+1||h.parent_hash!=tip_.header.hash())throw std::invalid_argument("block does not extend chain");
  if(h.timestamp<tip_.header.timestamp+genesis_.minimum_block_interval||h.timestamp>now+genesis_.maximum_future_drift)throw std::invalid_argument("invalid PoS timestamp");
  validate_execution_header(h,tip_.header,genesis_.minimum_gas_limit);
  if(h.difficulty!=1||h.nonce!=Bytes(8)||h.mix_hash!=Bytes(32))throw std::invalid_argument("invalid PoS seal");
  if(!block.uncles.empty())throw std::invalid_argument("PoS blocks cannot contain uncles");
  auto selected=selected_validator(genesis_.bft_version?bft_proposal_round(block):0);
  if(!selected||address(*selected)!=address(h.coinbase))throw std::invalid_argument("unexpected PoS validator");
  validation={true,{}};
 }
 if(!validation.valid)throw std::invalid_argument(validation.error);
 if(genesis_.consensus=="pow")validate_uncles(block,now);
 if(genesis_.consensus=="pow")validate_execution_header(block.header,tip_.header);
 auto candidate=state_;auto receipts=engine_.verify_block(candidate,block,tip_.header);
 return {block,std::move(candidate),std::move(receipts)};
}
void Blockchain::verify_proposal(const Block& block,const Int& now)const{
 std::lock_guard lock(mutex_);
 if(genesis_.consensus!="pos"||!genesis_.pos_signature_version)throw std::invalid_argument("signed PoS is not enabled");
 auto checked=verify_contents(block,now);
 auto next_pos=pos_;next_pos.reward(block.header.coinbase,block.header.number);sync_consensus(checked.state,block.header,next_pos);
}
void Blockchain::import_block(const Block& block,const Int& now){
 std::lock_guard lock(mutex_);
 if(genesis_.bft_version)throw std::invalid_argument("BFT block requires quorum certificate");
 verify_authentication(block);
 auto verified=verify_contents(block,now);auto& candidate=verified.state;auto& receipts=verified.receipts;
 auto next_pos=pos_;if(genesis_.consensus=="pos"){next_pos.reward(block.header.coinbase,block.header.number);next_pos=sync_consensus(candidate,block.header,next_pos);}
 audited_certified_tip_.reset();store_.commit(block,candidate,receipts,genesis_.consensus=="pos"?std::optional<Json>(next_pos.to_json()):std::nullopt);
 state_=std::move(candidate);tip_=block;pos_=std::move(next_pos);pool_.remove_mined(block.transactions);pool_.remove_stale(state_);emit("block",block.header.to_json());
}
void Blockchain::import_certified_block(const Block& block,uint64_t round,const std::vector<FinalityVote>& certificate,const Int& now){
 std::lock_guard lock(mutex_);import_certified_locked(block,round,certificate,now);
}
void Blockchain::commit_certified_round(ChainStore& journal,const std::string& signer,const Block& block,uint64_t round,const std::vector<FinalityVote>& certificate,const Int& now){
 std::lock_guard lock(mutex_);auto current=voting_set(pos_);
 auto position=journal.voting_position(signer,genesis_.chain_id,genesis_.block.header.hash());
 if(position){if(position->height!=u64(block.header.number)||position->validator_set_hash!=voting_set_hash(current))throw std::runtime_error("journal must match canonical parent before certified commit");}
 else if(block.header.number!=1)throw std::runtime_error("journal recovery required before certified commit");
 auto proof=import_certified_locked(block,round,certificate,now);
 // This is deliberately after canonical durability. On journal failure the
 // block stays committed; recover_voting_progress must run before further votes.
 journal.advance_voting_height(signer,u64(block.header.number),round,block.header.hash(),certificate,proof.validators,proof.next_validators,genesis_.chain_id,genesis_.block.header.hash());
}
BlockCertificate Blockchain::import_certified_locked(const Block& block,uint64_t round,const std::vector<FinalityVote>& certificate,const Int& now){
 auto current=voting_set(pos_);verify_authentication(block);
 if(genesis_.bft_version&&bft_proposal_round(block)>round)throw std::invalid_argument("certificate predates BFT proposal");
 if(!verify_commit_certificate(certificate,current,u64(block.header.number),round,block.header.hash(),genesis_.chain_id,genesis_.block.header.hash()))throw std::invalid_argument("certificate differs from verified parent validator set");
 auto verified=verify_contents(block,now);auto next_pos=pos_;next_pos.reward(block.header.coinbase,block.header.number);
 next_pos=sync_consensus(verified.state,block.header,next_pos);
 BlockCertificate proof{round,current,voting_set(next_pos),certificate};
 const bool extends_audited=audited_certified_tip_&&*audited_certified_tip_==tip_.header.hash();
 audited_certified_tip_.reset(); // A failed durable write must not retain a proof of the new tip.
 store_.commit_certified(block,verified.state,verified.receipts,next_pos.to_json(),proof);
 state_=std::move(verified.state);tip_=block;pos_=std::move(next_pos);pool_.remove_mined(block.transactions);pool_.remove_stale(state_);emit("block",block.header.to_json());
 if(extends_audited)audited_certified_tip_=tip_.header.hash();
 return proof;
}
Int Blockchain::stake(const std::string& addr)const{
 std::lock_guard lock(mutex_);auto normalized=address(addr);
 if(genesis_.staking_contract.empty())return pos_.stake(normalized);
 return staking_contract_stake(state_,tip_.header,genesis_.staking_contract,normalized,genesis_.chain_id,[this](const Int& number){auto block=store_.block_by_number(number);if(!block)throw std::runtime_error("missing ancestor block");return block->header.hash();},genesis_.evm_revision,genesis_.staking_query_limits);
}
VotingSet Blockchain::voting_set(const ProofOfStake& registry)const{
 if(genesis_.consensus!="pos"||!genesis_.pos_signature_version)throw std::invalid_argument("voting set requires signed PoS");
 VotingSet result;auto validators=registry.validators();
 if(registry.bootstrap&&validators.empty())result.emplace_back(genesis_.bootstrap_validator,Int(1));
 else for(const auto& member:validators)if(member.at("is_active").get<bool>())result.emplace_back(address(member.at("address").get<std::string>()),quantity(member.at("stake")));
 voting_set_hash(result);return result;
}
VotingSet Blockchain::voting_set()const{std::lock_guard lock(mutex_);return voting_set(pos_);}
void Blockchain::recover_voting_progress(ChainStore& journal,const std::string& signer)const{
 std::lock_guard lock(mutex_);auto members=voting_set(pos_);auto target=u64(Int(tip_.header.number+1));
 auto position=journal.voting_position(signer,genesis_.chain_id,genesis_.block.header.hash());
 if(position&&position->height>target)throw std::runtime_error("voting journal is ahead of canonical chain");
 // Reopened or otherwise unverified prefixes require a complete audit. Within
 // this process only verified, durably committed certificates extend that proof.
 if(!audited_certified_tip_||*audited_certified_tip_!=tip_.header.hash()){
  audited_certified_tip_.reset();auto verified=audit_locked();
  if(quantity(verified.at("certifiedBlocks"))!=tip_.header.number)throw std::runtime_error("voting recovery requires fully certified canonical history");
  audited_certified_tip_=tip_.header.hash();
 }
 // Keep validating the journal's entire certificate sequence: its provenance is
 // independent of the in-memory canonical-chain proof.
 for(Int height=1;height<=tip_.header.number;++height){
  auto block=store_.block_by_number(height);auto certificate=store_.certificate_at(height);
  if(!block||!certificate)throw std::runtime_error("missing certified recovery block");
  journal.advance_voting_height(signer,u64(height),certificate->round,block->header.hash(),certificate->votes,certificate->validators,certificate->next_validators,genesis_.chain_id,genesis_.block.header.hash());
 }
 position=journal.voting_position(signer,genesis_.chain_id,genesis_.block.header.hash());
 if(position&&(position->height!=target||position->validator_set_hash!=voting_set_hash(members)))throw std::runtime_error("recovered voting position differs from canonical state");
 if(!position&&target!=1)throw std::runtime_error("missing recovered voting position");
}
std::optional<FinalityVote> Blockchain::resume_consensus_vote(ChainStore& journal,const Bytes& key,const Int& now)const{
 std::lock_guard guard(mutex_);auto signer=public_address(public_key(key));auto members=voting_set(pos_);auto genesis_hash=genesis_.block.header.hash();
 if(std::none_of(members.begin(),members.end(),[&](const auto& member){return address(member.first)==address(signer);}))throw std::invalid_argument("resuming signer not in parent validator set");
 auto saved=journal.latest_consensus_vote(signer,genesis_.chain_id,genesis_hash);if(!saved)return std::nullopt;
 auto height=u64(Int(tip_.header.number+1));if(saved->height<height)return std::nullopt;
 auto active=journal.voting_lock(signer,genesis_.chain_id,genesis_hash);auto set_hash=voting_set_hash(members);
 if(saved->height!=height||saved->validator_set_hash!=set_hash||!active||active->height!=height||active->validator_set_hash!=set_hash)throw std::runtime_error("reserved vote differs from canonical parent");
 if(saved->block_hash){
  if(active->block_hash&&active->block_hash!=saved->block_hash)throw std::runtime_error("reserved vote contradicts durable lock");
  if(saved->step==VoteStep::precommit&&(!active->round||*active->round!=saved->round||active->block_hash!=saved->block_hash))throw std::runtime_error("reserved precommit has no matching lock");
  auto proposal=journal.voting_proposal(signer,height,*saved->block_hash,genesis_.chain_id,genesis_hash);
  if(!proposal)throw std::runtime_error("reserved vote proposal unavailable");
  if(genesis_.bft_version&&bft_proposal_round(*proposal)>saved->round)throw std::runtime_error("reserved vote predates proposal");
  verify_authentication(*proposal);auto checked=verify_contents(*proposal,now);auto next_pos=pos_;next_pos.reward(proposal->header.coinbase,proposal->header.number);next_pos=sync_consensus(checked.state,proposal->header,next_pos);voting_set(next_pos);
 }else if(saved->step==VoteStep::prevote&&active->block_hash)throw std::runtime_error("reserved nil prevote contradicts lock");
 // latest_consensus_vote verified canonical fields against the durable digest.
 // This reproduces that signature; it does not reserve a new vote or alter locks.
 sign_vote(*saved,key,genesis_.chain_id,genesis_hash);return saved;
}
FinalityVote Blockchain::vote_nil(ChainStore& journal,const Bytes& key,uint64_t round,VoteStep step,const std::vector<FinalityVote>& justification)const{
 std::lock_guard lock(mutex_);auto members=voting_set(pos_);auto signer=public_address(public_key(key));
 if(std::none_of(members.begin(),members.end(),[&](const auto& member){return address(member.first)==address(signer);}))throw std::invalid_argument("vote signer is not in parent validator set");
 FinalityVote vote;vote.height=u64(Int(tip_.header.number+1));vote.round=round;vote.step=step;
 vote.validator_set_hash=voting_set_hash(members);vote.validator=signer;
 sign_consensus_vote(journal,vote,key,members,justification,genesis_.chain_id,genesis_.block.header.hash());return vote;
}
FinalityVote Blockchain::vote_for_block(ChainStore& journal,const Bytes& key,const Block& block,uint64_t round,VoteStep step,const std::vector<FinalityVote>& justification,const Int& now)const{
 std::lock_guard lock(mutex_);auto members=voting_set(pos_);
 auto signer=public_address(public_key(key));
 if(std::none_of(members.begin(),members.end(),[&](const auto& member){return address(member.first)==address(signer);}))throw std::invalid_argument("vote signer is not in parent validator set");
 if(genesis_.bft_version&&bft_proposal_round(block)>round)throw std::invalid_argument("vote predates BFT proposal");
 verify_authentication(block);auto checked=verify_contents(block,now);
 auto next_pos=pos_;next_pos.reward(block.header.coinbase,block.header.number);
 next_pos=sync_consensus(checked.state,block.header,next_pos);voting_set(next_pos);
 FinalityVote vote;vote.height=u64(block.header.number);vote.round=round;vote.step=step;
 vote.validator_set_hash=voting_set_hash(members);vote.block_hash=block.header.hash();vote.validator=signer;
 sign_consensus_vote(journal,vote,key,members,justification,genesis_.chain_id,genesis_.block.header.hash(),&block);
 return vote;
}
VotingSet Blockchain::certify_voting_progress(ChainStore& journal,const std::string& signer,const Block& block,uint64_t round,const std::vector<FinalityVote>& certificate,const Int& now)const{
 std::lock_guard lock(mutex_);auto current=voting_set(pos_);
 if(genesis_.bft_version&&bft_proposal_round(block)>round)throw std::invalid_argument("certificate predates BFT proposal");
 verify_authentication(block);
 if(!verify_commit_certificate(certificate,current,u64(block.header.number),round,block.header.hash(),genesis_.chain_id,genesis_.block.header.hash()))throw std::invalid_argument("certificate differs from verified parent validator set");
 auto checked=verify_contents(block,now);auto next_pos=pos_;next_pos.reward(block.header.coinbase,block.header.number);
 next_pos=sync_consensus(checked.state,block.header,next_pos);auto next=voting_set(next_pos);
 journal.advance_voting_height(signer,u64(block.header.number),round,block.header.hash(),certificate,current,next,genesis_.chain_id,genesis_.block.header.hash());
 return next;
}
Json Blockchain::validators()const{std::lock_guard lock(mutex_);return pos_.validators();}
std::optional<std::string> Blockchain::next_validator(uint64_t round)const{std::lock_guard lock(mutex_);return genesis_.consensus=="pos"?selected_validator(round):std::nullopt;}
Block Blockchain::validate(const std::string& coinbase,const Int& timestamp){return produce(coinbase,timestamp,false,std::chrono::milliseconds(0));}
Block Blockchain::mine(const std::string& coinbase,const Int& timestamp,std::chrono::milliseconds timeout){return produce(coinbase,timestamp,true,timeout);}
Block Blockchain::block_template(const std::string& coinbase,const Int& timestamp,uint64_t round)const{std::lock_guard lock(mutex_);if(genesis_.consensus=="beacon")throw std::invalid_argument("Beacon templates require authenticated slot context");return prepare(coinbase,timestamp,round).block;}
Blockchain::PendingSnapshot Blockchain::pending_snapshot(const std::string& coinbase,const Int& now,const std::function<void()>& checkpoint)const{
 std::lock_guard lock(mutex_);require_execution_ready_locked();if(checkpoint)checkpoint();
 std::string producer=coinbase;
 if(genesis_.consensus=="pos"){
  auto selected=selected_validator();if(!selected)throw std::invalid_argument("no active validator for pending execution");producer=*selected;
 }
 Block block;block.header=prepare_header(producer,std::max(now,Int(tip_.header.timestamp+genesis_.minimum_block_interval)),0);
 std::map<Int,Bytes> ancestors;
 for(Int n=std::max(Int(0),Int(block.header.number-256));n<block.header.number;++n){
  if(checkpoint)checkpoint();auto ancestor=store_.block_by_number(n);if(!ancestor)throw std::runtime_error("missing pending ancestor block");ancestors.emplace(n,ancestor->header.hash());
 }
 // No live chain or store access after capture: execution can overlap imports,
 // pool changes or a development rewind without mixing block contexts.
 ExecutionEngine engine(genesis_.chain_id,[ancestors](const Int& number){auto it=ancestors.find(number);if(it==ancestors.end())throw std::runtime_error("missing captured pending ancestor");return it->second;},genesis_.evm_revision);
 engine=engine.with_checkpoint(checkpoint);
 auto execute=[block,initial=state_,pending=pool_.candidates_for_block(Int(1)<<256),engine,checkpoint,reward=genesis_.consensus!="beacon"]{
  auto prepared=execute_candidate(block,initial,pending,engine,checkpoint,reward);
  return std::make_pair(std::move(prepared.block.header),std::move(prepared.state));
 };
 return {block.header,std::move(ancestors),std::move(execute)};
}
std::pair<Header,State> Blockchain::pending_execution(const std::string& coinbase,const Int& now,const std::function<void()>& checkpoint)const{
 return pending_snapshot(coinbase,now,checkpoint).execute();
}
std::vector<SignedTransaction> Blockchain::pending_transactions(const Int& gas_limit)const{std::lock_guard lock(mutex_);return pool_.pending_for_block(gas_limit);}
Header Blockchain::prepare_header(const std::string& coinbase,const Int& timestamp,uint64_t round)const{
 bool mining=genesis_.consensus=="pow";
 if(genesis_.consensus=="pos"){auto selected=selected_validator(round);if(!selected||address(*selected)!=address(coinbase))throw std::invalid_argument("unexpected PoS validator");}
 if(timestamp<tip_.header.timestamp+genesis_.minimum_block_interval)throw std::invalid_argument("timestamp violates minimum block interval");
 Header h;h.parent_hash=tip_.header.hash();h.number=tip_.header.number+1;h.coinbase=address(coinbase);h.timestamp=timestamp;h.gas_limit=tip_.header.gas_limit;h.base_fee=next_base_fee(tip_.header);h.difficulty=mining?ProofOfWork::adjust_difficulty(tip_.header,timestamp):Int(genesis_.consensus=="beacon"?0:1);
 // Pending simulation uses the latest known randomness; an actual proposal
 // obtains its slot-specific RANDAO from the authenticated Beacon transition.
 if(genesis_.consensus=="beacon"){if(round)throw std::invalid_argument("Beacon has no legacy BFT round");h.mix_hash=tip_.header.mix_hash;}
 if(genesis_.bft_version)h.extra_data=bft_round_extra(round);
 return h;
}
Blockchain::Prepared Blockchain::prepare(const std::string& coinbase,const Int& timestamp,uint64_t round,const std::function<void()>& checkpoint)const{
 if(checkpoint)checkpoint();Block block;block.header=prepare_header(coinbase,timestamp,round);
 return execute_candidate(std::move(block),state_,pool_.candidates_for_block(Int(1)<<256),engine_.with_checkpoint(checkpoint),checkpoint);
}
Blockchain::Prepared Blockchain::execute_candidate(Block block,const State& initial,std::vector<TxPool::Candidate> pending,const ExecutionEngine& engine,const std::function<void()>& checkpoint,bool reward){
 if(checkpoint)checkpoint();
 // Select by sender nonce using a trial state; future nonces stay in the pool.
 auto trial=initial;Int cumulative=0;auto& h=block.header;
 bool progress=true;
 while(progress){progress=false;for(auto it=pending.begin();it!=pending.end();){
  if(checkpoint)checkpoint();
  // Every supported transaction needs at least 21000 intrinsic gas.
  if(h.gas_limit-cumulative<21000){progress=false;break;}
  const auto& account=static_cast<const State&>(trial).get(it->sender);
  if(it->transaction.tx.nonce!=account.nonce){++it;continue;}
  try{auto receipt=engine.apply_transaction(trial,it->transaction,h,Int(block.transactions.size()),cumulative);cumulative=receipt.cumulative_gas_used;block.transactions.push_back(it->transaction);it=pending.erase(it);progress=true;}
  catch(const std::invalid_argument&){it=pending.erase(it);}
 }}
 auto candidate=initial;auto receipts=engine.execute_block(candidate,block,reward);
 return {std::move(block),std::move(candidate),std::move(receipts)};
}
Block Blockchain::produce(const std::string& coinbase,const Int& timestamp,bool mining,std::chrono::milliseconds timeout){
 if(genesis_.consensus=="beacon")throw std::invalid_argument("Beacon production requires a signed slot proposal");
 std::lock_guard lock(mutex_);
 if((genesis_.consensus=="pow")!=mining)throw std::invalid_argument("production mode does not match consensus");
 if(!mining&&genesis_.pos_signature_version)throw std::invalid_argument("signed PoS production requires a signing key");
 auto prepared=prepare(coinbase,timestamp);auto& block=prepared.block;auto& candidate=prepared.state;auto& receipts=prepared.receipts;auto& h=block.header;
 if(mining){ProofOfWork pow;auto solution=pow.mine(h,timeout);
  if(!solution)throw std::runtime_error("mining timed out");
  h.nonce=solution->nonce;h.mix_hash=solution->mix_hash;
 }
 for(auto& receipt:receipts)receipt.block_hash=h.hash();
 auto next_pos=pos_;if(!mining){next_pos.reward(h.coinbase,h.number);next_pos=sync_consensus(candidate,h,next_pos);}
 store_.commit(block,candidate,receipts,mining?std::nullopt:std::optional<Json>(next_pos.to_json()));
 state_=std::move(candidate);tip_=block;pos_=std::move(next_pos);pool_.remove_mined(block.transactions);pool_.remove_stale(state_);emit("block",block.header.to_json());return block;
}
}
