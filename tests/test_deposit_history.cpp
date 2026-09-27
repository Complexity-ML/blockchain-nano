#include "nano/consensus_deposit_history.hpp"
#include "blst.h"
#include "nano/consensus_capella_deposits.hpp"
#include <fstream>
#include <iostream>
using namespace nano;
namespace {
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
template<class F>void refuses(F f){bool failed=false;try{f();}catch(const std::invalid_argument&){failed=true;}catch(const std::out_of_range&){failed=true;}check(failed,"invalid history accepted");}
Phase0Deposit deposit(unsigned seed,uint64_t amount){
 Phase0Deposit d{};std::array<uint8_t,32> entropy{};entropy[0]=uint8_t(seed);blst_scalar sk;blst_keygen(&sk,entropy.data(),entropy.size(),nullptr,0);blst_p1 pk;blst_sk_to_pk_in_g1(&pk,&sk);blst_p1_compress(d.message.public_key.data(),&pk);d.message.withdrawal_credentials[0]=1;d.message.withdrawal_credentials[31]=uint8_t(seed);d.message.amount_gwei=amount;
 const auto root=consensus_signing_root(consensus_deposit_root(d.message),consensus_domain({3,0,0,0},{},{}));static constexpr char dst[]="BLS_SIG_BLS12381G2_XMD:SHA-256_SSWU_RO_POP_";blst_p2 h,s;blst_hash_to_g2(&h,root.data(),root.size(),reinterpret_cast<const uint8_t*>(dst),sizeof(dst)-1,nullptr,0);blst_sign_pk_in_g1(&s,&h,&sk);blst_p2_compress(d.signature.data(),&s);return d;
}
Bytes calldata(const Phase0Deposit& d){
 const std::string name="deposit(bytes,bytes,bytes,bytes32)";auto out=keccak(Bytes(name.begin(),name.end()));out.resize(4);Bytes args(416);
 auto word=[&](size_t at,size_t n){const auto b=bytes(Int(n),32);std::copy(b.begin(),b.end(),args.begin()+at);};
 word(0,128);word(32,224);word(64,288);const auto root=consensus_deposit_data_root(d);std::copy(root.begin(),root.end(),args.begin()+96);
 word(128,48);std::copy(d.message.public_key.begin(),d.message.public_key.end(),args.begin()+160);
 word(224,32);std::copy(d.message.withdrawal_credentials.begin(),d.message.withdrawal_credentials.end(),args.begin()+256);
 word(288,96);std::copy(d.signature.begin(),d.signature.end(),args.begin()+320);out.insert(out.end(),args.begin(),args.end());return out;
}
}
int main(){try{
 std::ifstream f(std::string(NANO_TEST_SOURCE)+"/tests/fixtures/ethereum-blocks/for_prague--prague--eip6110_deposits--deposits--deposit.json");Json fixtures;f>>fixtures;
 State state;state.get(ethereum_deposit_contract)=State::from_json(fixtures.begin().value().at("pre")).get(ethereum_deposit_contract);
 const auto code_hash=keccak(state.get(ethereum_deposit_contract).code);const auto secret=bytes(Int(1),32);
 Transaction sample;sample.chain_id=1337;sample.to=ethereum_deposit_contract;sample.gas=1000000;sample.gas_price=10;
 const auto sender=SignedTransaction::sign(sample,secret).sender();state.get(sender).balance=Int("1000000000000000000000");
 Header anchor;anchor.difficulty=0;anchor.base_fee=1;anchor.state_root=state.root();
 ExecutionEngine engine(1337,{},EvmRevision::Paris);
 auto block=[&](const State& parent_state,const Header& parent,uint64_t nonce,const Phase0Deposit* d){
  Block out;out.header=parent;out.header.parent_hash=parent.hash();out.header.number=parent.number+1;out.header.timestamp=parent.timestamp+12;out.header.base_fee=next_base_fee(parent);
  if(d){auto tx=sample;tx.nonce=Int(std::to_string(nonce));tx.value=Int(std::to_string(d->message.amount_gwei))*1000000000;tx.data=calldata(*d);out.transactions.push_back(SignedTransaction::sign(tx,secret));}
  auto candidate=parent_state;engine.execute_block(candidate,out,false);return out;
 };
 ExecutionDepositHistory empty(anchor.hash());const auto empty_root=empty.root();
 const auto a=deposit(1,32000000000),b=deposit(2,32000000000);
 const auto ba=block(state,anchor,0,&a),bb=block(state,anchor,0,&b);
 const auto [ha,ra]=empty.extend(engine,state,anchor,ba,ba.header.hash(),code_hash,4);
 const auto [hb,rb]=empty.extend(engine,state,anchor,bb,bb.header.hash(),code_hash,4);
 check(empty.count()==0&&empty.root()==empty_root&&ha.count()==1&&hb.count()==1&&ha.root()!=hb.root(),"branch histories not isolated");
 Phase0DepositRules rules{1000000000,32000000000,100,{}};
 Phase0DepositState registry{};registry.deposit_root=ha.root();registry.deposit_count=1;
 check(process_phase0_deposit(registry,rules,ha.proof(0))==Phase0DepositOutcome::Admitted,"funded validator not admitted");
 check(registry.registry.validators.at(0).public_key==a.message.public_key&&registry.registry.validators.at(0).balance==32000000000,"wrong funded registry");
 Phase0DepositState other{};other.deposit_root=hb.root();other.deposit_count=1;const auto before=other;
 check(!process_phase0_deposit(other,rules,ha.proof(0))&&other==before,"foreign branch proof changed registry");
 check(process_phase0_deposit(other,rules,hb.proof(0))==Phase0DepositOutcome::Admitted,"alternative branch deposit failed");
 auto top=a;top.message.amount_gwei=8000000000;top.message.withdrawal_credentials[31]=99;top.signature.fill(0);
 const auto bt=block(ra.state,ba.header,1,&top);const auto [ht,rt]=ha.extend(engine,ra.state,ba.header,bt,bt.header.hash(),code_hash,4);
 registry.deposit_root=ht.root();registry.deposit_count=ht.count();check(process_phase0_deposit(registry,rules,ht.proof(1))==Phase0DepositOutcome::ToppedUp,"funded topup failed");
 check(registry.registry.validators.at(0).balance==40000000000&&registry.registry.validators.at(0).withdrawal_credentials==a.message.withdrawal_credentials,"topup replaced credentials");
 auto invalid=deposit(3,32000000000);invalid.signature.fill(0);const auto bi=block(rt.state,bt.header,2,&invalid);const auto [hi,ri]=ht.extend(engine,rt.state,bt.header,bi,bi.header.hash(),code_hash,4);
 registry.deposit_root=hi.root();registry.deposit_count=hi.count();check(process_phase0_deposit(registry,rules,hi.proof(2))==Phase0DepositOutcome::IgnoredInvalidSignature,"bad PoP admitted");check(registry.registry.validators.size()==1&&registry.next_deposit_index==3,"bad PoP index handling");
 // Earlier immutable views remain usable after later insertions, including proofs
 // for leaves whose sibling subtree was replaced by a subsequent deposit.
 Phase0DepositState replay{};replay.deposit_root=hi.root();replay.deposit_count=3;for(unsigned i=0;i<3;++i)check(process_phase0_deposit(replay,rules,hi.proof(i)).has_value(),"historical leaf proof failed");check(replay==registry,"sequential replay differs");
 check(ha.count()==1&&ha.proof(0).message.amount_gwei==32000000000&&hb.count()==1,"append mutated earlier branch");
 const auto duplicate=registry;check(!process_phase0_deposit(registry,rules,hi.proof(2))&&registry==duplicate,"deposit credited twice");
 refuses([&]{ha.extend(engine,rb.state,bb.header,bt,bt.header.hash(),code_hash,4);});
 refuses([&]{hi.proof(3);});
 auto tampered=hi.proof(0);tampered.proof[0][0]^=1;Phase0DepositState fresh{};fresh.deposit_root=hi.root();fresh.deposit_count=3;check(!process_phase0_deposit(fresh,rules,tampered),"tampered Merkle sibling accepted");
 // Advance an empty block: same tree, distinct execution anchor.
 const auto be=block(ri.state,bi.header,3,nullptr);const auto [he,re]=hi.extend(engine,ri.state,bi.header,be,be.header.hash(),code_hash,0);check(he.root()==hi.root()&&he.execution_hash()!=hi.execution_hash(),"empty block lost branch anchor");
 bool cancelled=false;try{hi.proof(0,[]{throw ExecutionCancelled("cancel");});}catch(const ExecutionCancelled&){cancelled=true;}check(cancelled&&hi.count()==3,"proof cancellation mutated history");
 // Durable contents cannot replace or reorder deposits under an authenticated
 // execution anchor, even if an attacker recomputes an outer file checksum.
 const auto snapshot=hi.snapshot();const auto restored=ExecutionDepositHistory::from_snapshot(Json::parse(snapshot.dump()),bi.header,ri.state,bi.header.hash(),code_hash,3);
 check(restored.snapshot()==snapshot&&restored.root()==hi.root(),"deposit history restore differs");
 for(unsigned i=0;i<3;++i)check(restored.proof(i).proof==hi.proof(i).proof,"restored deposit proof differs");
 auto forged=snapshot;auto text=forged["deposits"][0].get<std::string>();text[2]=text[2]=='0'?'1':'0';forged["deposits"][0]=text;
 refuses([&]{ExecutionDepositHistory::from_snapshot(forged,bi.header,ri.state,bi.header.hash(),code_hash,3);});
 forged=snapshot;std::swap(forged["deposits"][0],forged["deposits"][1]);refuses([&]{ExecutionDepositHistory::from_snapshot(forged,bi.header,ri.state,bi.header.hash(),code_hash,3);});
 refuses([&]{ExecutionDepositHistory::from_snapshot(snapshot,bi.header,ri.state,bi.header.hash(),code_hash,2);});
 refuses([&]{ExecutionDepositHistory::from_snapshot(snapshot,bb.header,rb.state,bb.header.hash(),code_hash,3);});
 check(hi.snapshot()==snapshot,"rejected restore changed source history");
 // Historical prefixes recover the original proofs without keeping old EVM states.
 const std::array<const ExecutionDepositHistory*,4> originals{&empty,&ha,&ht,&hi};
 size_t prefix_proofs=0;for(const auto* original:originals){size_t visits=0;const auto prefix=restored.authenticated_prefix(original->count(),original->execution_hash(),original->root(),[&]{++visits;});check(visits<=35,"prefix extraction traversed more than one tree boundary");check(prefix.snapshot()==original->snapshot(),"historical prefix contents differ");for(uint64_t i=0;i<prefix.count();++i){check(prefix.proof(i).proof==original->proof(i).proof,"historical prefix proof differs");++prefix_proofs;}}
 const auto same_tree=he.authenticated_prefix(hi.count(),hi.execution_hash(),hi.root());check(same_tree.snapshot()==hi.snapshot(),"empty-block prefix recovery differs");
 refuses([&]{restored.authenticated_prefix(hb.count(),hb.execution_hash(),hb.root());});
 refuses([&]{restored.authenticated_prefix(4,hi.execution_hash(),hi.root());});
 refuses([&]{restored.authenticated_prefix(1,Bytes(31),ha.root());});
 auto bad_root=ha.root();bad_root[0]^=1;refuses([&]{restored.authenticated_prefix(1,ha.execution_hash(),bad_root);});
 bool prefix_cancelled=false;size_t visits=0;try{restored.authenticated_prefix(1,ha.execution_hash(),ha.root(),[&]{if(++visits==10)throw ExecutionCancelled("prefix interrupted");});}catch(const ExecutionCancelled&){prefix_cancelled=true;}check(prefix_cancelled&&restored.snapshot()==snapshot,"prefix failure mutated history");
 // Native voting keeps historical commitments even when old full records disappear.
 CapellaForkStore full;std::vector<ConsensusRoot> ids;const std::array<const ExecutionDepositHistory*,5> histories{&empty,&ha,&ht,&hi,&he};const std::array<Header,5> headers{anchor,ba.header,bt.header,bi.header,be.header};
 for(size_t i=0;i<histories.size();++i){ConsensusBeaconHeader header{};header.slot=i;if(i)header.parent_root=ids.back();const auto id=consensus_beacon_header_root(header);CapellaForkBlock record{};record.header=header;record.execution_block.header=headers[i];record.deposit_history=std::make_shared<const ExecutionDepositHistory>(*histories[i]);full.blocks.emplace(id,std::move(record));ids.push_back(id);}full.timed.view.anchor=ids[0];
 auto pruned=full;pruned.network_anchor=ids[0];pruned.deposit_ancestors=capella_deposit_ancestors_before(full,ids[3]);pruned.timed.view.anchor=ids[3];for(size_t i=0;i<3;++i)pruned.blocks.erase(ids[i]);validate_capella_deposit_ancestors(pruned);
 CapellaForkBlockRules native{};native.native_deposit_votes=true;native.transition.consensus.state.eth1_votes_limit=64;native.transition.consensus.body.deposits=16;native.transition.consensus.maximum_block_bytes=100000;
 size_t vote_comparisons=0;for(uint64_t period:{2,64}){native.transition.consensus.state.eth1_votes_limit=period;for(uint64_t slot:{4,5,63,64,128}){check(capella_native_deposit_vote(full,ids[4],slot,native)==capella_native_deposit_vote(pruned,ids[4],slot,native),"pruning changed native period vote");++vote_comparisons;}}
 native.transition.consensus.state.eth1_votes_limit=64;
 auto vote_bytes=[](const ExecutionDepositHistory& history){const auto root=history.root();Bytes value(root.begin(),root.end());for(unsigned n=0;n<8;++n)value.push_back(uint8_t(history.count()>>(8*n)));value.insert(value.end(),history.execution_hash().begin(),history.execution_hash().end());return value;};
 for(const auto* history:histories){const auto vote=vote_bytes(*history);check(capella_native_deposit_vote_known(pruned,ids[4],vote),"pruning lost historical native vote");}
 check(!capella_native_deposit_vote_known(pruned,ids[4],vote_bytes(hb)),"pruned history accepted foreign branch vote");
 Bytes advanced(272);const auto selected_vote=vote_bytes(ha);std::copy(selected_vote.begin(),selected_vote.end(),advanced.begin()+180);auto put=[&](size_t at,uint64_t value){for(unsigned n=0;n<4;++n)advanced[at+n]=uint8_t(value>>(8*n));};put(252,272);put(264,272);
 const auto operation=capella_native_deposit_operations(full,ids[4],advanced,selected_vote,native);check(operation.size()==1240&&operation==capella_native_deposit_operations(pruned,ids[4],advanced,selected_vote,native),"pruning changed mandatory historical deposit proof");
 auto twice=pruned;twice.deposit_ancestors=capella_deposit_ancestors_before(pruned,ids[4]);twice.timed.view.anchor=ids[4];twice.blocks.erase(ids[3]);validate_capella_deposit_ancestors(twice);check(twice.deposit_ancestors.size()==4&&capella_native_deposit_operations(twice,ids[4],advanced,selected_vote,native)==operation,"repeated pruning lost deposit references");
 auto forged_pins=pruned;forged_pins.deposit_ancestors[0].root[0]^=1;refuses([&]{validate_capella_deposit_ancestors(forged_pins);});forged_pins=pruned;forged_pins.deposit_ancestors.pop_back();refuses([&]{validate_capella_deposit_ancestors(forged_pins);});forged_pins=pruned;std::swap(forged_pins.deposit_ancestors[0],forged_pins.deposit_ancestors[1]);refuses([&]{validate_capella_deposit_ancestors(forged_pins);});
 std::cout<<Json{{"scenario","execution-deposit-history"},{"profile","Paris"},{"branches",2},{"fundedAdmission",true},{"topup",true},{"invalidPoPIgnored",true},{"crossBranchProofRejected",true},{"duplicateCreditRejected",true},{"authenticatedRestore",true},{"runtimeConnected",false},{"authenticatedPrefixes",4},{"prefixProofsCompared",prefix_proofs},{"nativePeriodVotesCompared",vote_comparisons},{"nativeMandatoryProofPreserved",true}}.dump()<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
