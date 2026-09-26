#include "nano/consensus_deposit_history.hpp"
#include <fstream>
#include <iostream>
using namespace nano;
namespace {
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
Header header(Json j){for(const auto& [a,b]:std::vector<std::pair<std::string,std::string>>{{"uncleHash","sha3Uncles"},{"coinbase","miner"},{"transactionsTrie","transactionsRoot"},{"receiptTrie","receiptsRoot"},{"bloom","logsBloom"}})if(j.contains(a)){j[b]=j.at(a);j.erase(a);}return Header::from_json(j);}
Bytes call(State state,const std::string& signature){ExecutionContext c;c.revision=EvmRevision::Prague;c.address=ethereum_deposit_contract;c.code=static_cast<const State&>(state).get(c.address).code;c.data=keccak(Bytes(signature.begin(),signature.end()));c.data.resize(4);c.is_static=true;auto result=EVM(state).execute(c);check(result.success,"deposit contract view failed");return result.return_data;}
template<class F> void refuses(F f){bool rejected=false;try{f();}catch(const std::invalid_argument&){rejected=true;}catch(const std::length_error&){rejected=true;}check(rejected,"invalid deposit source accepted");}
}
int main(){try{
 std::ifstream file(std::string(NANO_TEST_SOURCE)+"/tests/fixtures/ethereum-blocks/for_prague--prague--eip6110_deposits--deposits--deposit.json");Json fixtures;file>>fixtures;
 size_t cases=0,deposits=0,empty=0;
 for(const auto& [name,test]:fixtures.items()){
  const auto pre=State::from_json(test.at("pre"));const auto parent=header(test.at("genesisBlockHeader"));
  const auto& input=test.at("blocks").at(0);Block block;block.header=header(input.at("blockHeader"));block.withdrawals=input.at("withdrawals");
  const auto raw=Rlp::decode(unhex(input.at("rlp")));for(const auto& tx:raw[1].items)block.transactions.push_back(SignedTransaction::decode(tx.list?tx.encode():tx.blob()));
  const auto code_hash=keccak(pre.get(ethereum_deposit_contract).code);
  ExecutionEngine engine(1,[&](const Int& n){return n==parent.number?parent.hash():Bytes(32);},EvmRevision::Prague,ReplayPolicy::AllowLegacyUnprotected,ConsensusInputPolicy::EthereumValidated);
  const auto before=pre.root();const auto expected_hash=unhex(input.at("blockHeader").at("hash"));
  const ExecutionDepositHistory initial_history(parent.hash());
  const auto [history,batch]=initial_history.extend(engine,pre,parent,block,expected_hash,code_hash,1000);
  check(pre.root()==before,"replay mutated parent state");
  check(batch.state.root()==State::from_json(test.at("postState")).root(),"official post-state mismatch");
  const auto count=call(batch.state,"get_deposit_count()");check(count.size()==96,"contract count ABI");uint64_t n=0;for(unsigned i=0;i<8;++i)n|=uint64_t(count[64+i])<<(8*i);
  check(n==batch.next_deposit_index&&n==batch.deposits.size(),"executed deposit count differs");
  // Independently compare reconstructed SSZ tree with the actual Solidity view.
  std::vector<ConsensusRoot> layer;Int amount=0;for(const auto& d:batch.deposits){layer.push_back(consensus_deposit_data_root(d));amount+=Int(std::to_string(d.message.amount_gwei))*1000000000;}
  ConsensusRoot zero{};if(layer.empty())layer.push_back(zero);
  for(unsigned depth=0;depth<32;++depth){if(layer.size()%2)layer.push_back(zero);std::vector<ConsensusRoot> next;for(size_t i=0;i<layer.size();i+=2)next.push_back(consensus_signing_root(layer[i],layer[i+1]));layer=std::move(next);zero=consensus_signing_root(zero,zero);}
  ConsensusRoot length{};for(unsigned i=0;i<8;++i)length[i]=uint8_t(n>>(8*i));const auto root=consensus_signing_root(layer.at(0),length);
  check(history.root()==root,"history tree differs from independent reconstruction");
  for(uint64_t i=0;i<n;++i){const auto proof=history.proof(i);auto proven=consensus_deposit_data_root(proof);for(unsigned depth=0;depth<33;++depth)proven=(i>>depth)&1?consensus_signing_root(proof.proof[depth],proven):consensus_signing_root(proven,proof.proof[depth]);check(proven==root,"official deposit Merkle proof differs");}
  check(call(batch.state,"get_deposit_root()")==Bytes(root.begin(),root.end()),"executed deposit tree differs");
  check(batch.state.get(ethereum_deposit_contract).balance-pre.get(ethereum_deposit_contract).balance==amount,"deposit amount not backed by executed balance");
  if(!n)++empty;deposits+=n;++cases;
  // Cheap failures precede EVM replay; all preserve the supplied parent.
  auto wrong_hash=expected_hash;wrong_hash[0]^=1;refuses([&]{replay_execution_deposits(engine,pre,parent,block,wrong_hash,code_hash,0,1000);});
  auto wrong_code=code_hash;wrong_code[0]^=1;refuses([&]{replay_execution_deposits(engine,pre,parent,block,expected_hash,wrong_code,0,1000);});
  if(name.find("single_deposit_from_eoa]")!=std::string::npos){
   refuses([&]{replay_execution_deposits(engine,pre,parent,block,expected_hash,code_hash,1,1000);});
   refuses([&]{replay_execution_deposits(engine,pre,parent,block,expected_hash,code_hash,0,0);});
   auto broken=block;broken.header.receipts_root[0]^=1;refuses([&]{replay_execution_deposits(engine,pre,parent,broken,broken.header.hash(),code_hash,0,1000);});
   auto wrong_parent=parent;wrong_parent.state_root[0]^=1;refuses([&]{replay_execution_deposits(engine,pre,wrong_parent,block,expected_hash,code_hash,0,1000);});
   bool cancelled=false;try{replay_execution_deposits(engine,pre,parent,block,expected_hash,code_hash,0,1000,[]{throw ExecutionCancelled("cancelled");});}catch(const ExecutionCancelled&){cancelled=true;}check(cancelled,"deposit replay ignored cancellation");
  }
 }
 check(cases==35&&empty>0&&deposits>200,"incomplete deposit fixture coverage");
 std::cout<<Json{{"scenario","execution-deposit-source"},{"officialCases",cases},{"deposits",deposits},{"zeroDepositCases",empty},{"allMerkleProofsVerified",true},{"beaconAdmission",false}}.dump()<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
