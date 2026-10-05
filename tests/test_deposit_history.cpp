#include "nano/consensus_deposit_history.hpp"
#include "blst.h"
#include "nano/consensus_capella_deposits.hpp"
#include "nano/consensus_capella_reactor.hpp"
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
 // A duty deadline is administrative cancellation, never a contract failure.
 // Interrupt both deposit views inside their EVM checkpoint paths, then retry.
 size_t view_checkpoints=0;ha.validate_execution(ba.header,ra.state,code_hash,[&]{++view_checkpoints;});
 check(view_checkpoints>5,"deposit view did not execute checkpoints");
 for(const auto stop_at:{size_t(4),view_checkpoints-1}){
  size_t visits=0;bool expired=false;try{ha.validate_execution(ba.header,ra.state,code_hash,[&]{if(++visits==stop_at)throw ExpiredCapellaDuty("deposit view duty expired");});}
  catch(const ExpiredCapellaDuty& e){expired=std::string(e.what())=="deposit view duty expired";}
  catch(const std::exception&){/* Wrong exception type means a fatal validator error. */}
  check(expired,"deposit view swallowed typed duty deadline");
  ha.validate_execution(ba.header,ra.state,code_hash);check(ha.count()==1&&ra.state.root()==ba.header.state_root,"deadline changed authenticated execution/history");
 }
 Phase0DepositRules rules{1000000000,32000000000,100,{}};
 Phase0DepositState registry{};registry.deposit_root=ha.root();registry.deposit_count=1;
 check(process_phase0_deposit(registry,rules,ha.proof(0))==Phase0DepositOutcome::Admitted,"funded validator not admitted");
 check(registry.registry.validators.at(0).public_key==a.message.public_key&&registry.registry.validators.at(0).balance==32000000000,"wrong funded registry");
 Phase0DepositState other{};other.deposit_root=hb.root();other.deposit_count=1;const auto before=other;
 check(!process_phase0_deposit(other,rules,ha.proof(0))&&other==before,"foreign branch proof changed registry");
 check(process_phase0_deposit(other,rules,hb.proof(0))==Phase0DepositOutcome::Admitted,"alternative branch deposit failed");
 // Competing funded branches contain the same keys in opposite orders.
 // The execution nonce and authenticated history, not the public key, determine
 // the index. Never reuse the registry or a proof from the sibling branch.
 const auto bab=block(ra.state,ba.header,1,&b),bba=block(rb.state,bb.header,1,&a);
 const auto [hab,rab]=ha.extend(engine,ra.state,ba.header,bab,bab.header.hash(),code_hash,4);
 const auto [hba,rba]=hb.extend(engine,rb.state,bb.header,bba,bba.header.hash(),code_hash,4);
 auto consume=[&](const ExecutionDepositHistory& history){Phase0DepositState out{};out.deposit_root=history.root();out.deposit_count=history.count();
  for(uint64_t i=0;i<history.count();++i)check(process_phase0_deposit(out,rules,history.proof(i))==Phase0DepositOutcome::Admitted,"ordered branch deposit rejected");return out;};
 const auto registry_ab=consume(hab),registry_ba=consume(hba);
 check(registry_ab.registry.validators.at(0).public_key==a.message.public_key&&registry_ab.registry.validators.at(1).public_key==b.message.public_key&&registry_ba.registry.validators.at(0).public_key==b.message.public_key&&registry_ba.registry.validators.at(1).public_key==a.message.public_key,"competing order did not change validator indices");
 check(hab.root()!=hba.root()&&hab.execution_hash()!=hba.execution_hash(),"opposite orders share deposit commitment");
 const Int backing("64000000000000000000");
 for(const auto* post:{&rab.state,&rba.state})check(post->get(ethereum_deposit_contract).balance-state.get(ethereum_deposit_contract).balance==backing,"two-branch deposits not backed by execution");
 unsigned foreign_rejections=0,replayed_positions=0;
 for(const auto& pair:{std::pair{&hab,&hba},std::pair{&hba,&hab}}){
  Phase0DepositState local{};local.deposit_root=pair.first->root();local.deposit_count=2;
  for(uint64_t i=0;i<2;++i){const auto unchanged=local;
   check(!process_phase0_deposit(local,rules,pair.second->proof(i))&&local==unchanged,"foreign ordered proof changed cursor or registry");++foreign_rejections;
   check(process_phase0_deposit(local,rules,pair.first->proof(i))==Phase0DepositOutcome::Admitted,"valid proof failed after foreign rejection");
  }
  const auto completed=local;
  for(uint64_t i=0;i<2;++i){check(!process_phase0_deposit(local,rules,pair.first->proof(i))&&local==completed,"completed branch double credited");++replayed_positions;}
 }
 const auto restored_ab=ExecutionDepositHistory::from_snapshot(hab.snapshot(),bab.header,rab.state,bab.header.hash(),code_hash,2);
 const auto restored_ba=ExecutionDepositHistory::from_snapshot(hba.snapshot(),bba.header,rba.state,bba.header.hash(),code_hash,2);
 check(consume(restored_ab)==registry_ab&&consume(restored_ba)==registry_ba,"restored branches changed validator indices");
 refuses([&]{ExecutionDepositHistory::from_snapshot(hab.snapshot(),bba.header,rba.state,bba.header.hash(),code_hash,2);});
 refuses([&]{ExecutionDepositHistory::from_snapshot(hba.snapshot(),bab.header,rab.state,bab.header.hash(),code_hash,2);});
 check(consume(hab)==registry_ab&&consume(hba)==registry_ba&&ha.count()==1&&hb.count()==1,"branch rejection mutated shared prefix");
 std::cout<<Json{{"scenario","competing-deposit-order"},{"fundedBranches",2},{"depositsPerBranch",2},{"sameKeyIndices",Json::array({0,1})},{"foreignProofsRejected",foreign_rejections},{"replayedPositionsRejected",replayed_positions},{"bothHistoriesRestored",true},{"beaconForkChoice",false},{"runtimeConnected",false}}.dump()<<'\n';
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
 // Select mandatory operations from each parent in a store retaining both
 // branches. Equal counts must not make the sibling's root/anchor acceptable.
 CapellaForkStore competing;competing.blocks.emplace(ids[0],full.blocks.at(ids[0]));competing.timed.view.anchor=ids[0];
 auto add_branch=[&](const ExecutionDepositHistory& first,const Header& first_execution,const ExecutionDepositHistory& second,const Header& second_execution){
  ConsensusRoot parent=ids[0];uint64_t slot=1;
  for(const auto& item:{std::pair{&first,&first_execution},std::pair{&second,&second_execution}}){CapellaForkBlock record{};record.header.slot=slot++;record.header.parent_root=parent;record.header.state_root=item.first->root();record.execution_block.header=*item.second;record.deposit_history=std::make_shared<const ExecutionDepositHistory>(*item.first);parent=consensus_beacon_header_root(record.header);check(competing.blocks.emplace(parent,std::move(record)).second,"competing deposit record collision");}return parent;
 };
 const auto parent_ab=add_branch(ha,ba.header,hab,bab.header),parent_ba=add_branch(hb,bb.header,hba,bba.header);
 unsigned selected_operations=0;
 for(const auto& side:{std::pair{parent_ab,&hab},std::pair{parent_ba,&hba}}){
  const auto own=vote_bytes(*side.second),foreign=vote_bytes(side.second==&hab?hba:hab);
  check(capella_native_deposit_vote_known(competing,side.first,own)&&!capella_native_deposit_vote_known(competing,side.first,foreign),"native ancestry accepted sibling at equal count");
  Bytes view(272);auto set4=[&](size_t at,uint64_t n){for(unsigned k=0;k<4;++k)view[at+k]=uint8_t(n>>(8*k));};set4(252,272);set4(264,272);std::copy(own.begin(),own.end(),view.begin()+180);
  const auto operations=capella_native_deposit_operations(competing,side.first,view,own,native);check(operations.size()==2480,"native selection mixed competing histories");
  for(size_t i=0;i<2;++i){const auto proof=side.second->proof(i);const auto at=i*1240;
   for(size_t j=0;j<33;++j)check(std::equal(proof.proof[j].begin(),proof.proof[j].end(),operations.begin()+at+32*j),"native sibling proof differs");
   check(std::equal(proof.message.public_key.begin(),proof.message.public_key.end(),operations.begin()+at+1056),"native operation assigned wrong public key");++selected_operations;
  }
  refuses([&]{capella_native_deposit_operations(competing,side.first,view,foreign,native);});
  std::copy(foreign.begin(),foreign.end(),view.begin()+180);refuses([&]{capella_native_deposit_operations(competing,side.first,view,own,native);});
 }
 std::cout<<Json{{"scenario","competing-native-deposit-selection"},{"retainedBranches",2},{"mandatoryOperationsChecked",selected_operations},{"siblingVotesRejected",true},{"foreignSelectedHistoryRejected",true},{"syntheticBeaconEnvelope",true},{"runtimeConnected",false}}.dump()<<'\n';
 for(const auto* history:histories){const auto vote=vote_bytes(*history);check(capella_native_deposit_vote_known(pruned,ids[4],vote),"pruning lost historical native vote");}
 check(!capella_native_deposit_vote_known(pruned,ids[4],vote_bytes(hb)),"pruned history accepted foreign branch vote");
 Bytes advanced(272);const auto selected_vote=vote_bytes(ha);std::copy(selected_vote.begin(),selected_vote.end(),advanced.begin()+180);auto put=[&](size_t at,uint64_t value){for(unsigned n=0;n<4;++n)advanced[at+n]=uint8_t(value>>(8*n));};put(252,272);put(264,272);
 const auto operation=capella_native_deposit_operations(full,ids[4],advanced,selected_vote,native);check(operation.size()==1240&&operation==capella_native_deposit_operations(pruned,ids[4],advanced,selected_vote,native),"pruning changed mandatory historical deposit proof");
 auto twice=pruned;twice.deposit_ancestors=capella_deposit_ancestors_before(pruned,ids[4]);twice.timed.view.anchor=ids[4];twice.blocks.erase(ids[3]);validate_capella_deposit_ancestors(twice);check(twice.deposit_ancestors.size()==4&&capella_native_deposit_operations(twice,ids[4],advanced,selected_vote,native)==operation,"repeated pruning lost deposit references");
 auto forged_pins=pruned;forged_pins.deposit_ancestors[0].root[0]^=1;refuses([&]{validate_capella_deposit_ancestors(forged_pins);});forged_pins=pruned;forged_pins.deposit_ancestors.pop_back();refuses([&]{validate_capella_deposit_ancestors(forged_pins);});forged_pins=pruned;std::swap(forged_pins.deposit_ancestors[0],forged_pins.deposit_ancestors[1]);refuses([&]{validate_capella_deposit_ancestors(forged_pins);});
 std::cout<<Json{{"scenario","execution-deposit-history"},{"profile","Paris"},{"branches",2},{"fundedAdmission",true},{"topup",true},{"invalidPoPIgnored",true},{"crossBranchProofRejected",true},{"duplicateCreditRejected",true},{"authenticatedRestore",true},{"runtimeConnected",false},{"authenticatedPrefixes",4},{"prefixProofsCompared",prefix_proofs},{"nativePeriodVotesCompared",vote_comparisons},{"nativeMandatoryProofPreserved",true}}.dump()<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
