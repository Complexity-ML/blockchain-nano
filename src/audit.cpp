#include "nano/chain.hpp"
#include <unistd.h>
namespace nano {
Json Blockchain::audit()const{std::lock_guard lock(mutex_);return audit_locked();}
Json Blockchain::audit_locked()const{
 std::string pattern=(std::filesystem::temp_directory_path()/"nano-audit-XXXXXX").string();
 std::vector<char> name(pattern.begin(),pattern.end());name.push_back(0);
 if(!mkdtemp(name.data()))throw std::runtime_error("cannot create audit scratch directory");
 struct Cleanup{std::filesystem::path path;~Cleanup(){std::error_code ignored;std::filesystem::remove_all(path,ignored);}} cleanup{name.data()};
 Blockchain replay(cleanup.path,genesis_);Int transactions=0,certified=0;
 const Int now(std::to_string(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count()));
 for(Int height=0;height<=tip_.header.number;++height){
  try{
   auto block=store_.block_by_number(height);if(!block)throw std::runtime_error("canonical block missing");
   if(height==0){if(block->to_json(true)!=genesis_.block.to_json(true))throw std::runtime_error("genesis differs from configuration");}
   else if(auto certificate=store_.certificate_at(height)){
    if(voting_set_hash(certificate->validators)!=voting_set_hash(replay.voting_set()))throw std::runtime_error("certificate parent validators differ from replay");
    replay.import_certified_block(*block,certificate->round,certificate->votes,now);++certified;
    if(voting_set_hash(certificate->next_validators)!=voting_set_hash(replay.voting_set()))throw std::runtime_error("certificate next validators differ from replay");
   }else replay.import_block(*block,now);
   auto indexed=store_.block_by_hash(block->header.hash());
   if(!indexed||indexed->to_json(true)!=block->to_json(true))throw std::runtime_error("block hash index mismatch");
   auto state=store_.state_at(height);if(!state||state->root()!=replay.state().root())throw std::runtime_error("state differs from replay");
   if(store_.consensus_at(height)!=replay.store().consensus_at(height))throw std::runtime_error("validator registry differs from replay");
   for(const auto& tx:block->transactions){
    auto hash=tx.hash();auto receipt=store_.receipt(hash),expected=replay.store().receipt(hash);
    if(!receipt||!expected||receipt->to_json()!=expected->to_json())throw std::runtime_error("receipt differs from replay");
    if(store_.transaction(hash)!=replay.store().transaction(hash))throw std::runtime_error("transaction index differs from replay");
    ++transactions;
   }
  }catch(const std::exception& e){throw std::runtime_error("chain audit failed at block "+height.get_str()+": "+e.what());}
 }
 if(store_.count()!=replay.store().count()||store_.total_difficulty()!=replay.store().total_difficulty())throw std::runtime_error("chain audit metadata mismatch");
 return {{"verified",true},{"certifiedBlocks",quantity(certified)},{"blocks",store_.count()},{"transactions",quantity(transactions)},{"blockHash",hex(tip_.header.hash())},{"stateRoot",hex(replay.state().root())},{"posSignatureVersion",genesis_.pos_signature_version}};
}
}
