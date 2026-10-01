#include "support/rocksdb.hpp"
#include "nano/block_auth.hpp"
#include "nano/crypto.hpp"
#include "nano/rpc.hpp"
#include <unistd.h>
#include <rocksdb/db.h>
#include <iostream>
using namespace nano;
void check(bool valid,const char* message){if(!valid)throw std::runtime_error(message);}
int main(){try{
 auto key=bytes(Int(1),32);Block block;block.header.number=1;block.header.coinbase=public_address(public_key(key));Bytes genesis=keccak(Bytes{1});
 check(!verify_block_signature(block,1337,genesis),"unsigned block rejected");
 sign_block(block,key,1337,genesis);check(verify_block_signature(block,1337,genesis),"valid block signature");
 auto roundtrip=Block::from_json(block.to_json(true));check(roundtrip.producer_signature==block.producer_signature&&verify_block_signature(roundtrip,1337,genesis),"proof survives JSON roundtrip");
 check(!verify_block_signature(block,1338,genesis)&&!verify_block_signature(block,1337,keccak(Bytes{2})),"chain ID and genesis prevent cross-chain replay");
 auto changed=block;changed.header.timestamp+=1;check(!verify_block_signature(changed,1337,genesis),"timestamp tampering rejected");
 changed=block;changed.header.state_root[0]^=1;check(!verify_block_signature(changed,1337,genesis),"state commitment tampering rejected");
 changed=block;Transaction tx;changed.transactions.push_back(SignedTransaction::sign(tx,key));check(!verify_block_signature(changed,1337,genesis),"transaction body tampering rejected");
 changed=block;changed.header.coinbase=public_address(public_key(bytes(Int(2),32)));check(!verify_block_signature(changed,1337,genesis),"producer tampering rejected");
 bool rejected=false;try{sign_block(changed,key,1337,genesis);}catch(const std::invalid_argument&){rejected=true;}check(rejected,"wrong signing key refused");
 const Int order("0xfffffffffffffffffffffffffffffffebaaedce6af48a03bbfd25e8cd0364141");
 changed=block;auto s=integer(Bytes(changed.producer_signature.begin()+32,changed.producer_signature.begin()+64));auto high=bytes(Int(order-s),32);std::copy(high.begin(),high.end(),changed.producer_signature.begin()+32);changed.producer_signature[64]^=1;
 check(!verify_block_signature(changed,1337,genesis),"high-s malleable proof rejected");
 for(unsigned n:{0,1,64,66}){changed=block;changed.producer_signature.resize(n);check(!verify_block_signature(changed,1337,genesis),"malformed signature size rejected");}
 auto json=block.to_json(true);json["producerProof"]["version"]=2;rejected=false;try{Block::from_json(json);}catch(const std::exception&){rejected=true;}check(rejected,"unknown proof version rejected");
 changed=block;changed.withdrawals.push_back(Json::object());check(!verify_block_signature(changed,1337,genesis),"unsupported withdrawals rejected");
 auto root=std::filesystem::temp_directory_path()/("nano-block-auth-"+std::to_string(getpid()));
 Json config={{"chainId",1337},{"consensus","pos"},{"posSignatureVersion",1},{"bootstrapValidator",public_address(public_key(key))}};
 auto genesis_config=Genesis::from_json({{"config",config}});Block accepted;
 {Blockchain producer(root/"producer",genesis_config),peer(root/"peer",genesis_config);Rpc rpc(peer);
  auto template_block=producer.block_template(public_address(public_key(key)),12);auto initial=peer.state().root();
  auto reject=[&](const Block& candidate){bool failed=false;try{peer.import_block(candidate,12);}catch(const std::exception&){failed=true;}check(failed&&peer.state().root()==initial&&peer.tip().header.number==0&&peer.store().count()==1,"bad proof cannot mutate canonical storage");};
  peer.verify_proposal(template_block,12);
  auto unauthorized_withdrawal=template_block;unauthorized_withdrawal.withdrawals=Json::array({{{"index","0x0"},{"validatorIndex","0x0"},{"address",block.header.coinbase},{"amount","0x1"}}});
  unauthorized_withdrawal.header.withdrawals_root=withdrawals_root(unauthorized_withdrawal.withdrawals);
  bool withdrawal_rejected=false;try{peer.verify_proposal(unauthorized_withdrawal,12);}catch(const std::invalid_argument& e){withdrawal_rejected=std::string(e.what())=="withdrawals not authorized by Nano consensus";}
  check(withdrawal_rejected&&peer.state().root()==initial&&peer.store().count()==1,"Nano proposal must not authorize Ethereum withdrawal credits");
  check(peer.state().root()==initial&&peer.tip().header.number==0,"proposal check does not publish state");
  auto invalid_proposal=template_block;invalid_proposal.header.state_root[0]^=1;
  bool proposal_rejected=false;try{peer.reserve_signature(invalid_proposal,block.header.coinbase);}catch(const std::exception&){proposal_rejected=true;}
  check(proposal_rejected&&!peer.store().signing_proposal(1337,genesis_config.block.header.hash(),block.header.coinbase,1),"invalid state commitment cannot reserve a signature");
  invalid_proposal=template_block;invalid_proposal.header.coinbase=public_address(public_key(bytes(Int(2),32)));
  proposal_rejected=false;try{peer.verify_proposal(invalid_proposal,12);}catch(const std::exception&){proposal_rejected=true;}
  check(proposal_rejected,"pre-sign verification checks validator selection");
  reject(template_block);
  auto forged=template_block;forged.header.coinbase=public_address(public_key(bytes(Int(2),32)));sign_block(forged,bytes(Int(2),32),1337,genesis_config.block.header.hash());reject(forged);
  sign_block(template_block,key,1337,genesis_config.block.header.hash());forged=template_block;forged.header.gas_limit+=1;reject(forged);
  bool refused=false;try{producer.validate(public_address(public_key(key)),12);}catch(const std::exception&){refused=true;}check(refused&&producer.tip().header.number==0,"unsigned local producer cannot bypass rule");
  accepted=producer.validate_signed(key,12);peer.import_block(Block::from_json(accepted.to_json(true)),12);
  check(peer.state().root()==producer.state().root()&&peer.tip().producer_signature==accepted.producer_signature,"peer executes and persists signed block");
  check(producer.restore(0,genesis_config.block.header.hash()),"signed chain rewind fixture");
  auto conflicting=producer.block_template(public_address(public_key(key)),13);bool conflict=false;
  try{producer.reserve_signature(conflicting,public_address(public_key(key)));}catch(const std::exception&){conflict=true;}
  check(conflict&&producer.tip().header.number==0,"rewind cannot erase signing reservation");
  check(producer.validate_signed(key,13).producer_signature==accepted.producer_signature&&producer.tip().header.timestamp==12,"producer resumes exact reserved block instead of changed timestamp");
  auto submit=[&](const Block& candidate){return rpc.handle(Json{{"jsonrpc","2.0"},{"id",1},{"method","validator_submitBlock"},{"params",Json::array({candidate.to_json(true)})}}).at("result");};
  check(submit(accepted).at("success")==true,"valid duplicate signed submission accepted");
  forged=accepted;forged.producer_signature.clear();check(submit(forged).at("success")==false,"unsigned duplicate cannot bypass authentication");
  forged=accepted;forged.producer_signature[0]^=1;check(submit(forged).at("success")==false,"forged duplicate cannot bypass authentication");
 }
 {Blockchain reopened(root/"peer",genesis_config);check(reopened.tip().producer_signature==accepted.producer_signature&&verify_block_signature(reopened.tip(),1337,genesis_config.block.header.hash()),"proof survives database reopen");}
 {Blockchain interrupted(root/"interrupted",genesis_config);auto candidate=interrupted.block_template(public_address(public_key(key)),12);interrupted.reserve_signature(candidate,public_address(public_key(key)));}
 {Blockchain resumed(root/"interrupted",genesis_config);auto recovered=resumed.validate_signed(key,14);check(recovered.header.hash()==accepted.header.hash()&&recovered.producer_signature==accepted.producer_signature,"reservation survives process-style close before signing");}
 config["posSignatureVersion"]=0;auto legacy=Genesis::from_json({{"config",config}});check(legacy.block.header.hash()!=genesis_config.block.header.hash(),"proof activation changes network genesis identity");
 bool mismatch=false;try{Blockchain downgrade(root/"peer",legacy);}catch(const std::exception&){mismatch=true;}check(mismatch,"database cannot silently downgrade signature rule");
 {ChainStore journal(root/"journal");journal.reserve_signature(1337,genesis_config.block.header.hash(),block.header.coinbase,1,Bytes(32,1));}
 {ChainStore journal(root/"journal");journal.reserve_signature(1337,genesis_config.block.header.hash(),block.header.coinbase,1,Bytes(32,1));
  bool conflict=false;try{journal.reserve_signature(1337,genesis_config.block.header.hash(),block.header.coinbase,1,Bytes(32,2));}catch(const std::exception&){conflict=true;}check(conflict,"journal rejects conflicting digest after reopen");
  journal.reserve_signature(1337,genesis_config.block.header.hash(),block.header.coinbase,2,Bytes(32,2));
  bool locked=false;try{ChainStore concurrent(root/"journal");}catch(const std::exception&){locked=true;}check(locked,"journal excludes concurrent independent writers");
 }
 {Blockchain clean(root/"corrupt-history",genesis_config);clean.validate_signed(key,12);clean.validate_signed(key,24);
  auto before=clean.tip().header.hash();auto result=clean.audit();check(result.at("verified")==true&&result.at("blocks")==3&&clean.tip().header.hash()==before,"audit replays full signed chain without changing its tip");}
 {rocksdb::Options options;std::unique_ptr<rocksdb::DB> db;check(test::open_rocksdb(options,(root/"corrupt-history").string(),db).ok(),"open corruption fixture");
  auto storage_key="block/"+hex(bytes(Int(1),8));std::string value;check(db->Get(rocksdb::ReadOptions(),storage_key,&value).ok(),"read historical corruption fixture");
  auto historical=Json::parse(value);auto signature=unhex(historical["producerProof"]["signature"]);signature[0]^=1;historical["producerProof"]["signature"]=hex(signature);
  rocksdb::WriteOptions write;write.sync=true;check(db->Put(write,storage_key,historical.dump()).ok(),"persist historical proof corruption");}
 {Blockchain historical(root/"corrupt-history",genesis_config);check(historical.tip().header.number==2,"valid tip does not imply valid history");bool detected=false;
  try{historical.audit();}catch(const std::exception& e){detected=std::string(e.what()).find("block 1")!=std::string::npos;}
  check(detected&&historical.store().count()==3,"audit detects old forged proof without repairing source");}
 auto funded=Genesis::from_json({{"config",{{"chainId",1337},{"consensus","pos"},{"posSignatureVersion",1},{"bootstrapValidator",block.header.coinbase}}},{"alloc",{{block.header.coinbase,{{"balance","0x100000000000000000"}}}}}});
 Transaction transfer;transfer.to=public_address(public_key(bytes(Int(2),32)));transfer.value=7;transfer.gas_price=2000000000;auto signed_transfer=SignedTransaction::sign(transfer,key);
 {Blockchain clean(root/"receipt-audit",funded);check(clean.submit(signed_transfer),"audit transaction fixture accepted");clean.validate_signed(key,12);check(clean.audit().at("transactions")=="0x1","audit verifies transaction and receipt indices");}
 {rocksdb::Options options;std::unique_ptr<rocksdb::DB> db;check(test::open_rocksdb(options,(root/"receipt-audit").string(),db).ok(),"open receipt corruption fixture");auto storage_key="receipt/"+hex(signed_transfer.hash());std::string value;check(db->Get(rocksdb::ReadOptions(),storage_key,&value).ok(),"read audit receipt");auto receipt=Json::parse(value);receipt["gasUsed"]="0x1";rocksdb::WriteOptions write;write.sync=true;check(db->Put(write,storage_key,receipt.dump()).ok(),"persist receipt corruption");}
 {Blockchain corrupted(root/"receipt-audit",funded);bool detected=false;try{corrupted.audit();}catch(const std::exception& e){detected=std::string(e.what()).find("receipt differs")!=std::string::npos;}check(detected,"audit detects receipt inconsistent with reexecution");}
 {Blockchain clean(root/"index-audit",genesis_config);clean.validate_signed(key,12);clean.validate_signed(key,24);}
 {rocksdb::Options options;std::unique_ptr<rocksdb::DB> db;check(test::open_rocksdb(options,(root/"index-audit").string(),db).ok(),"open hash index fixture");
  rocksdb::WriteOptions write;write.sync=true;check(db->Put(write,"hash/"+hex(accepted.header.hash()),hex(bytes(Int(0),8))).ok(),"redirect hash index to another valid block");}
 {ChainStore store(root/"index-audit");bool detected=false;try{store.block_by_hash(accepted.header.hash());}catch(const std::exception& e){detected=std::string(e.what()).find("block hash index mismatch")!=std::string::npos;}
  check(detected&&store.block_by_number(1)->header.hash()==accepted.header.hash(),"hash lookup rejects redirected index while canonical block remains readable");}
 {rocksdb::Options options;std::unique_ptr<rocksdb::DB> db;check(test::open_rocksdb(options,(root/"index-audit").string(),db).ok(),"open height index fixture");
  std::string first;check(db->Get(rocksdb::ReadOptions(),"block/"+hex(bytes(Int(1),8)),&first).ok(),"read original height fixture");
  rocksdb::WriteOptions write;write.sync=true;check(db->Put(write,"block/"+hex(bytes(Int(2),8)),first).ok(),"redirect height to a valid earlier signed block");}
 {ChainStore store(root/"index-audit");bool height_rejected=false,tip_rejected=false;
  try{store.block_by_number(2);}catch(const std::exception& e){height_rejected=std::string(e.what()).find("block height index mismatch")!=std::string::npos;}
  try{store.latest();}catch(const std::exception& e){tip_rejected=std::string(e.what()).find("chain tip height mismatch")!=std::string::npos;}
  check(height_rejected&&tip_rejected,"height and tip lookups reject correctly signed block under wrong key");}
 Json rules_config={{"chainId",1337},{"consensus","pos"},{"posSignatureVersion",1},{"bootstrapValidator",block.header.coinbase},
  {"consensusRules",{{"version",1},{"minimumBlockInterval",12},{"maximumFutureDrift",3},{"minimumGasLimit",10000}}}};
 auto rules_genesis=Genesis::from_json({{"config",rules_config}});
 check(rules_genesis.block.header.hash()!=genesis_config.block.header.hash(),"consensus rules bind genesis identity");
 for(const char* field:{"minimumBlockInterval","maximumFutureDrift","minimumGasLimit"}){auto altered=rules_config;altered["consensusRules"][field]=altered["consensusRules"][field].get<unsigned>()+1;check(Genesis::from_json({{"config",altered}}).block.header.hash()!=rules_genesis.block.header.hash(),"each consensus parameter changes genesis identity");}
 for(unsigned bad=0;bad<5;++bad){auto invalid=rules_config;
  if(bad==0)invalid["consensusRules"]["minimumBlockInterval"]=0;
  if(bad==1)invalid["consensusRules"]["maximumFutureDrift"]=-1;
  if(bad==2)invalid["consensusRules"]["minimumGasLimit"]=4999;
  if(bad==3)invalid["consensusRules"]["unknown"]=1;
  if(bad==4)invalid["consensusRules"]["version"]=2;
  bool refused=false;try{Genesis::from_json({{"config",invalid}});}catch(const std::exception&){refused=true;}check(refused,"invalid genesis consensus rules rejected");}
 {Blockchain bounds(root/"gas-boundaries",rules_genesis);
  const Int parent_limit=bounds.tip().header.gas_limit, delta=parent_limit/1024;
  for(int direction:{-1,1})for(int offset:{-1,0,1}){
   auto candidate=bounds.block_template(block.header.coinbase,12);
   candidate.header.gas_limit=parent_limit+direction*(delta+offset);
   bool refused=false;try{bounds.verify_proposal(candidate,12);}catch(const std::invalid_argument& e){
    if(std::string(e.what())!="invalid block gas limits")throw;refused=true;}
   check(refused==(offset>=0),"PoS gas adjustment strictly excludes both EIP-1559 boundaries");
   sign_block(candidate,key,1337,rules_genesis.block.header.hash());
   if(offset>=0){bool rejected=false;try{bounds.import_block(candidate,12);}catch(const std::invalid_argument& e){
    if(std::string(e.what())!="invalid block gas limits")throw;rejected=true;}
    check(rejected&&bounds.tip().header.number==0,"signed boundary block rejected without canonical mutation");}
  }
 }
 auto gas_genesis=Genesis::from_json({{"config",rules_config},{"gasLimit",10000}});
 {Blockchain gas_chain(root/"gas-rules",gas_genesis);auto candidate=gas_chain.block_template(block.header.coinbase,12);candidate.header.gas_limit=9999;bool refused=false;
  try{gas_chain.verify_proposal(candidate,12);}catch(const std::exception& e){refused=std::string(e.what()).find("invalid block gas limits")!=std::string::npos;}
  check(refused,"genesis minimum gas limit enforced even within parent gas adjustment bound");}
 {Blockchain constrained(root/"rules",rules_genesis);bool refused=false;try{constrained.block_template(block.header.coinbase,11);}catch(const std::exception&){refused=true;}check(refused,"minimum interval enforced during production");
  auto valid=constrained.block_template(block.header.coinbase,12);constrained.verify_proposal(valid,9);
  refused=false;try{constrained.verify_proposal(valid,8);}catch(const std::exception&){refused=true;}check(refused,"future drift from genesis enforced at verification");
  auto too_early=valid;too_early.header.timestamp=11;refused=false;try{constrained.verify_proposal(too_early,12);}catch(const std::exception&){refused=true;}check(refused,"minimum interval enforced independently at import verification");
  constrained.validate_signed(key,12);check(constrained.audit().at("verified")==true,"versioned rules replay consistently");}
 Json fee_rules={{"version",1},{"initial_base_fee",25000000000ULL},{"target_gas_per_block",15000000}};
 auto fee_genesis=Genesis::from_json({{"config",rules_config},{"evm_rules",fee_rules}});
 check(fee_genesis.block.header.base_fee==Int("25000000000")&&next_base_fee(fee_genesis.block.header)==Int("21875000000"),"genesis fee config drives first produced block");
 auto fee_parent=fee_genesis.block.header;fee_parent.gas_used=15000000;check(next_base_fee(fee_parent)==Int("25000000000"),"target gas leaves fee unchanged");
 fee_parent.gas_used=30000000;check(next_base_fee(fee_parent)==Int("28125000000"),"full block increases fee by one eighth");
 fee_parent.base_fee=1;fee_parent.gas_used=15000001;check(next_base_fee(fee_parent)==2,"congestion increases fee by at least one wei");
 auto changed_fee=fee_rules;changed_fee["initial_base_fee"]=25000000001ULL;
 check(Genesis::from_json({{"config",rules_config},{"evm_rules",changed_fee}}).block.header.hash()!=fee_genesis.block.header.hash(),"initial fee binds chain identity");
 for(unsigned bad=0;bad<4;++bad){auto data=Json{{"config",rules_config},{"evm_rules",fee_rules}};
  if(bad==0)data["evm_rules"]["target_gas_per_block"]=14000000;
  if(bad==1)data["evm_rules"]["initial_base_fee"]=-1;
  if(bad==2)data["evm_rules"]["version"]=2;
  if(bad==3)data["baseFeePerGas"]=1;
  bool refused=false;try{Genesis::from_json(data);}catch(const std::exception&){refused=true;}check(refused,"inconsistent EIP-1559 genesis rejected");}
 {Blockchain producer(root/"fee-rules",fee_genesis),peer(root/"fee-peer",fee_genesis);auto candidate=producer.block_template(block.header.coinbase,12);
  check(candidate.header.base_fee==Int("21875000000"),"template uses configured genesis fee");
  auto forged=candidate;forged.header.base_fee=Int("25000000000");bool refused=false;try{peer.verify_proposal(forged,12);}catch(const std::exception& e){refused=std::string(e.what()).find("invalid base fee")!=std::string::npos;}check(refused,"peer rejects fixed initial fee reused in block one");
  auto produced=producer.validate_signed(key,12);peer.import_block(produced,12);check(peer.audit().at("verified")==true,"configured fee replays consistently across peers");}
 std::filesystem::remove_all(root);
 std::cout<<"Block signature domain, tampering, encoding and malleability checks passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
