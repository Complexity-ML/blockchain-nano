// Private WAN qualification helper. Uses only known public synthetic key0.
// Never part of nano's runtime, and refuses non-fixture chain/profile/registry.
#include "nano/chain.hpp"
#include "nano/consensus_capella_reactor.hpp"
#include "nano/consensus_capella_fork_state.hpp"
#include "nano/consensus_message_ssz.hpp"
#include "nano/consensus_bls.hpp"
#include "../support/persistent_rpc.hpp"
#include "blst.h"
#include <fstream>
#include <iostream>
#include <thread>
#include <unistd.h>
using namespace nano;
namespace {
void need(bool b,const char* s){if(!b)throw std::runtime_error(s);}
std::string read(const std::string& p,size_t max){need(std::filesystem::is_regular_file(p)&&std::filesystem::file_size(p)<=max,"file size/type");std::ifstream f(p,std::ios::binary);std::string s((std::istreambuf_iterator<char>(f)),{});need(s.size()<=max&&!f.bad(),"file read budget");return s;}
Json json(const std::string& p){return Json::parse(read(p,1024*1024));}
uint64_t num(const Json& x){return u64(quantity(x));}
struct PublicKey0 {
 blst_scalar secret;Bytes key;
 PublicKey0(){uint64_t words[4]{1,0,0,0};blst_scalar_from_uint64(&secret,words);blst_p1 p;blst_sk_to_pk_in_g1(&p,&secret);key.resize(48);blst_p1_compress(key.data(),&p);}
 Bytes sign(const ConsensusRoot& root)const{constexpr char dst[]="BLS_SIG_BLS12381G2_XMD:SHA-256_SSWU_RO_POP_";blst_p2 h,s;blst_hash_to_g2(&h,root.data(),root.size(),reinterpret_cast<const uint8_t*>(dst),sizeof(dst)-1,nullptr,0);blst_sign_pk_in_g1(&s,&h,&secret);Bytes out(96);blst_p2_compress(out.data(),&s);return out;}
};
struct Fixture {
 Genesis genesis;Json c;CapellaForkBlockRules rules;CapellaStoreSnapshotLimits limits;CapellaForkStore seed;
 Fixture(const std::string& g,const std::string& config):genesis(Genesis::from_json(json(g))),c(json(config).at("capella")){
  need(genesis.chain_id==1337&&genesis.consensus=="beacon"&&genesis.evm_revision==EvmRevision::Shanghai,"synthetic Beacon Shanghai only");
  const auto& l=c.at("limits");limits={size_t(num(l.at("envelopeBytes"))),size_t(num(l.at("blocks"))),size_t(num(l.at("messages"))),size_t(num(l.at("equivocations"))),size_t(num(l.at("stateBytes"))),size_t(num(l.at("totalStateBytes"))),num(l.at("replaySlots")),num(l.at("syncTrials"))};
  need(limits.envelope_bytes<=128*1024*1024&&limits.blocks<=1024,"fixture snapshot budget");
  const auto& p=c.at("profile");rules=load_capella_fork_profile(read(p.at("file"),1024*1024),unhex(p.at("sha256")),1024*1024);
  need(rules.transition.chain_id==genesis.chain_id,"genesis/profile chain mismatch");
  const auto& i=c.at("initial");seed=capella_store_from_snapshot(read(i.at("file"),limits.envelope_bytes),unhex(i.at("sha256")),rules,limits);
  const auto& head=capella_fork_head(seed,rules);auto context=capella_fork_block_context(head.state,head.header,{rules.transition.consensus.epoch,rules.transition.execution,rules.transition.maximum_state_bytes},rules.transition.consensus.state);
  need(context.registry.validators.size()==64&&std::ranges::equal(context.registry.validators.at(0).public_key,PublicKey0{}.key),"known 64-key fixture required");
 }
};
std::pair<IndexedConsensusAttestation,IndexedConsensusAttestation> proof(Fixture& f){
 const auto& h=capella_fork_head(f.seed,f.rules);const auto ctx=capella_fork_block_context(h.state,h.header,{f.rules.transition.consensus.epoch,f.rules.transition.execution,f.rules.transition.maximum_state_bytes},f.rules.transition.consensus.state);
 IndexedConsensusAttestation a{};a.indices={0};a.data.slot=f.seed.timed.view.justified.epoch*f.rules.transition.consensus.epoch.ffg.slots_per_epoch;a.data.source=f.seed.timed.view.finalized;a.data.target=f.seed.timed.view.justified;a.data.beacon_block_root=f.seed.timed.view.justified.root;auto b=a;b.data.beacon_block_root[0]^=1;
 const auto version=a.data.target.epoch<ctx.fork.epoch?ctx.fork.previous_version:ctx.fork.current_version;
 for(auto* v:{&a,&b})v->signature=PublicKey0{}.sign(consensus_signing_root(consensus_attestation_root(v->data),consensus_domain({1,0,0,0},version,ctx.genesis_root)));
 auto view=f.seed.timed.view;need(process_phase0_fork_slashing(view,ctx,a,b,f.rules.transition.consensus.body.validators_per_committee)&&view.equivocating_indices.contains(0),"valid proof failed local admission");
 auto bad=a;bad.signature[0]^=1;view=f.seed.timed.view;need(!process_phase0_fork_slashing(view,ctx,bad,b,f.rules.transition.consensus.body.validators_per_committee),"forged signature accepted");return {a,b};
}
Json generate(Fixture& f){
 auto [a,b]=proof(f);const auto max=f.rules.transition.consensus.body.validators_per_committee;const auto wire=encode_consensus_attester_slashing_ssz(a,b,max);need(bool(decode_consensus_attester_slashing_ssz(wire,max)),"proof codec");
 const auto& head=capella_fork_head(f.seed,f.rules);const auto context=capella_fork_block_context(head.state,head.header,{f.rules.transition.consensus.epoch,f.rules.transition.execution,f.rules.transition.maximum_state_bytes},f.rules.transition.consensus.state);
 Json invalid=Json::array();
 for(const std::string mode:{"bad-signature","wrong-known-key","unknown-index","duplicate-index"}){
  auto x=a,y=b;if(mode=="bad-signature")x.signature[0]^=1;else if(mode=="wrong-known-key"){x.indices={1};y.indices={1};}else if(mode=="unknown-index"){x.indices={64};y.indices={64};}else{x.indices={0,0};y.indices={0,0};}
  auto view=f.seed.timed.view;bool admitted=false;try{admitted=process_phase0_fork_slashing(view,context,x,y,max);}catch(const std::invalid_argument&){}
  need(!admitted&&view.equivocating_indices==f.seed.timed.view.equivocating_indices,"forged proof changed fixture admission");
  invalid.push_back({{"case",mode},{"ssz",hex(encode_consensus_attester_slashing_ssz(x,y,max))}});
 }
 return {{"format","nano-private-slashing-fixture-v1"},{"chainId","0x539"},{"genesisHash",hex(f.genesis.block.header.hash())},{"offender",0},{"syntheticValidators",64},{"ssz",hex(wire)},{"reverseSsz",hex(encode_consensus_attester_slashing_ssz(b,a,max))},{"invalid",invalid},{"productionQualified",false}};
}
void bootstrap(int argc,char** argv){
 const char* path=std::getenv("NANO_TEST_SLASHING_FILE"),*daemon=std::getenv("NANO_TEST_SLASHING_DAEMON"),*token=std::getenv("NANO_NETWORK_TOKEN");need(path&&daemon&&token,"private supervisor fixture context required");
 const auto p=json(path);need(p.at("format")=="nano-private-slashing-fixture-v1"&&p.at("chainId")=="0x539"&&p.at("offender")==0&&p.at("syntheticValidators")==64&&p.at("productionQualified")==false,"invalid synthetic proof manifest");
 const auto port=std::stoul(argv[1]);need(port>1024&&port<65536,"fixture IPC port");PersistentRpc client{uint16_t(port)};
 auto status=client.call("nano_capellaStatus",Json::array({token}));need(status.at("chainId")==p.at("chainId")&&status.at("genesisHash")==p.at("genesisHash"),"private proof chain identity mismatch");
 const auto session=status.at("admissionSession");uint64_t id=1;
 auto submit=[&](const Json& wire,bool accepted){Json envelope;for(const char* k:{"chainId","genesisHash","forkDigest","consensusIdentity"})envelope[k]=status.at(k);envelope["kind"]="attester_slashing";envelope["ssz"]=wire;const auto job=quantity(Int(std::to_string(id++)));
  client.call("nano_capellaSubmit",Json::array({token,session,job,envelope.dump()}));const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(10);Json receipt;
  do{receipt=client.call("nano_capellaJob",Json::array({token,session,job}));if(receipt.at("state")=="completed")break;std::this_thread::sleep_for(std::chrono::milliseconds(1));}while(std::chrono::steady_clock::now()<deadline);
  need(receipt.at("state")=="completed","proof admission deadline");need(receipt.value("decision",std::string())==(accepted?"accepted":"rejected"),"proof admission decision mismatch");client.call("nano_capellaForget",Json::array({token,session,job}));
 };
 need(p.at("invalid").size()==4,"four forged proof cases required");for(const auto& v:p.at("invalid"))submit(v.at("ssz"),false);submit(p.at("ssz"),true);submit(p.at("reverseSsz"),true);
 std::cout<<Json{{"event","private-slashing-bootstrap-passed"},{"forgedRefusals",4},{"validAndReverseReplayAccepted",true},{"offender",0},{"publicKeysOnly",true}}.dump()<<std::endl;
 const std::string program=daemon;unsetenv("NANO_TEST_SLASHING_FILE");unsetenv("NANO_TEST_SLASHING_DAEMON");std::vector<char*> args{const_cast<char*>(program.c_str())};for(int i=1;i<argc;++i)args.push_back(argv[i]);args.push_back(nullptr);execv(program.c_str(),args.data());throw std::runtime_error("cannot exec selected P2P daemon");
}
}
int main(int argc,char** argv){try{
 if(argc==5&&std::string(argv[1])=="--generate"){
  need(!std::filesystem::exists(argv[4]),"fresh output required");Fixture f(argv[2],argv[3]);auto result=generate(f);std::ofstream out(argv[4]);need(bool(out),"create proof output");out<<result.dump(2)<<'\n';out.close();need(bool(out),"write proof output");std::cout<<Json{{"event","private-slashing-proof-generated"},{"offender",0},{"forgedCases",4},{"productionQualified",false}}.dump()<<'\n';return 0;
 }
 if(argc>=4&&std::getenv("NANO_TEST_SLASHING_FILE")){bootstrap(argc,argv);return 1;}
 throw std::runtime_error("usage: --generate synthetic-genesis node-config NEW-proof; private supervisor bootstrap only");
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
