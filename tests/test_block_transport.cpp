#include "support/rocksdb.hpp"
#include "nano/consensus_round.hpp"
#include "nano/block_auth.hpp"
#include "support/process.hpp"
#include <rocksdb/db.h>
using namespace test;
int main(){try{
 Temp root;auto signing_key=bytes(Int(1),32);auto signer=public_address(public_key(signing_key));
 auto genesis=Genesis::from_json({{"config",{{"chainId",1337},{"consensus","pos"},{"posSignatureVersion",1},{"bftVersion",1},{"bootstrapValidator",signer},{"evmFork","Paris"}}},{"alloc",{{signer,{{"balance","0x100000000000000000"}}}}}});
 // Codec coverage is independent of which transaction types are activated on
 // the live Nano network. Preserve the exact signed bytes for all five types.
 auto synthetic=genesis.block;synthetic.header.number=1;synthetic.header.coinbase=address(signer);
 for(unsigned type=0;type<=4;++type){
  Transaction tx;tx.type=type;tx.chain_id=1337;tx.nonce=type;tx.to=signer;tx.data={0,1,2};tx.gas=100000;tx.gas_price=2000000000;tx.max_fee=2000000000;tx.priority_fee=1000000000;
  if(type==3){tx.max_fee_per_blob_gas=1;tx.blob_versioned_hashes={Bytes(32,1)};}
  if(type==4){Authorization a;a.chain_id=1337;a.target=signer;a.nonce=0;auto signature=sign_hash(signing_key,a.signing_hash());a.r=signature.r;a.s=signature.s;a.y_parity=signature.v;tx.authorization_list={a};}
  synthetic.transactions.push_back(SignedTransaction::sign(tx,signing_key));
 }
 synthetic.header.transactions_root=transactions_root(synthetic.transactions);sign_block(synthetic,signing_key,1337,genesis.block.header.hash());
 auto wire=synthetic.to_transport_json();auto decoded=Block::from_json(wire);
 if(decoded.to_json(true)!=synthetic.to_json(true))std::cout<<Json{{"stage","codec-roundtrip-difference"},{"diff",Json::diff(synthetic.to_json(true),decoded.to_json(true))}}.dump()<<std::endl;
 check(decoded.to_json(true)==synthetic.to_json(true),"compact block preserves the complete block representation");
 check(verify_block_signature(decoded,1337,genesis.block.header.hash()),"compact block preserves producer authentication");
 for(size_t i=0;i<5;++i)check(decoded.transactions[i].raw()==synthetic.transactions[i].raw(),"typed signed transaction bytes changed");
 check(Block::from_json(synthetic.to_json(true)).to_json(true)==synthetic.to_json(true),"legacy expanded block remains readable");
 for(unsigned mode=0;mode<4;++mode){auto bad=wire;
  if(mode==0)bad["transactionEncoding"]="signed-rlp-v2";
  if(mode==1)bad["transactions"][0]=synthetic.transactions[0].to_json();
  if(mode==2)bad.erase("transactionEncoding");
  if(mode==3)bad["transactions"][0]=bad["transactions"][0].get<std::string>()+"00";
  bool rejected=false;try{Block::from_json(bad);}catch(const std::exception&){rejected=true;}check(rejected,"malformed or unversioned compact block accepted");
 }
 Blockchain chain(root/"chain",genesis);Transaction tx;tx.chain_id=1337;tx.to=signer;tx.gas_price=2000000000;check(chain.submit(SignedTransaction::sign(tx,signing_key)),"legacy journal fixture transaction admitted");
 auto proposal=chain.block_template(signer,12,0);sign_block(proposal,signing_key,1337,genesis.block.header.hash());FinalityVote reserved;
 {ChainStore journal(root/"journal");reserved=chain.vote_for_block(journal,signing_key,proposal,0,VoteStep::prevote,{},12);}
 const auto proposal_key="voting/v1/"+hex(bytes(Int(1337),32))+"/"+hex(genesis.block.header.hash())+"/"+address(signer)+"/proposal/"+hex(bytes(Int(1),8))+"/"+hex(proposal.header.hash());
 auto raw_value=[&](std::optional<std::string> replacement=std::nullopt){
  std::unique_ptr<rocksdb::DB> db;rocksdb::Options options;auto opened=test::open_rocksdb(options,(root/"journal").string(),db);check(opened.ok(),"open journal for legacy-format fixture");
  if(replacement){rocksdb::WriteOptions writes;writes.sync=true;check(db->Put(writes,proposal_key,*replacement).ok(),"write legacy-format fixture");}
  std::string value;check(db->Get(rocksdb::ReadOptions(),proposal_key,&value).ok(),"read proposal fixture bytes");return value;
 };
 check(Json::parse(raw_value()).at("transactionEncoding")=="signed-rlp-v1","new journal proposal uses versioned compact encoding");
 const auto legacy=proposal.to_json(true).dump();raw_value(legacy);
 {ChainStore journal(root/"journal");auto start=ConsensusRound::Clock::time_point{};ConsensusRound round(chain,journal,signing_key,0,start,1s,1s,1s);auto progress=round.advance(start,12);
  check(progress.committed&&!progress.broadcast.empty()&&progress.broadcast.front().signature==reserved.signature,"legacy journal does not resume the exact reserved vote");
  check(chain.tip().header.hash()==proposal.header.hash()&&chain.audit().at("verified")==true,"legacy recovery changes certified canonical history");
 }
 check(raw_value()==legacy,"legacy proposal bytes were silently rewritten");
 std::cout<<Json{{"scenario","block-transport-codec"},{"signedTypes",Json::array({0,1,2,3,4})},{"malformedCasesRejected",4},{"legacyJournalExactVoteResumed",true},{"legacyProposalBytesPreserved",true},{"canonicalAuditVerified",true}}.dump()<<std::endl;
 return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<std::endl;return 1;}}
