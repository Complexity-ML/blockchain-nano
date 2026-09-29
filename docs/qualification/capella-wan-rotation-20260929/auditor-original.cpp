// Offline qualification tool. Only stopped, isolated synthetic devnet stores.
#include "nano/consensus_capella_reactor.hpp"
#include "nano/chain.hpp"
#include "nano/consensus_beacon_state.hpp"
#include "nano/consensus_beacon_body.hpp"
#include "nano/consensus_bls.hpp"
#include <fstream>
#include <iostream>
#include <set>
using namespace nano;
namespace {
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
std::string read(const std::string& path,size_t limit){std::ifstream f(path,std::ios::binary);require(bool(f),"missing input");f.seekg(0,std::ios::end);const auto size=f.tellg();require(size>=0&&uint64_t(size)<=limit,"input budget exceeded");f.seekg(0);return {std::istreambuf_iterator<char>(f),{}};}
uint64_t value(const Json& j){return u64(quantity(j));}
uint64_t number(std::span<const uint8_t>b,size_t at,size_t n=8){require(at<=b.size()&&n<=b.size()-at,"SSZ bounds");uint64_t out=0;for(size_t i=0;i<n;++i)out|=uint64_t(b[at+i])<<(8*i);return out;}
ConsensusRoot root(std::span<const uint8_t>b,size_t at=0){require(at<=b.size()&&32<=b.size()-at,"root bounds");ConsensusRoot r{};std::copy_n(b.begin()+at,32,r.begin());return r;}
std::string text(const ConsensusRoot&r){return hex(Bytes(r.begin(),r.end()));}
Bytes cut(const Bytes&b,size_t at,size_t n){require(at<=b.size()&&n<=b.size()-at,"byte bounds");return {b.begin()+at,b.begin()+at+n};}
ConsensusBytes4 version(const Bytes&b,uint64_t epoch){ConsensusBytes4 v{};std::copy_n(b.begin()+(epoch<number(b,56)?48:52),4,v.begin());return v;}
}
int main(int argc,char**argv){try{
 require(argc==6,"usage: rotation-audit genesis node-config stopped-db proof node-name");
 auto g=Genesis::from_json(Json::parse(read(argv[1],1048576)));require(g.chain_id==1337,"synthetic chain only");
 auto c=Json::parse(read(argv[2],1048576)).at("capella");auto p=c.at("profile");
 auto rules=load_capella_fork_profile(read(p.at("file"),value(p.at("maximumBytes"))),unhex(p.at("sha256")),value(p.at("maximumBytes")));
 const auto lim=c.at("limits");CapellaStoreSnapshotLimits limits{size_t(value(lim.at("envelopeBytes"))),size_t(value(lim.at("blocks"))),size_t(value(lim.at("messages"))),size_t(value(lim.at("equivocations"))),size_t(value(lim.at("stateBytes"))),size_t(value(lim.at("totalStateBytes"))),value(lim.at("replaySlots")),value(lim.at("syncTrials"))};
 const auto proof=Json::parse(read(argv[4],64*1024*1024));const auto initial=c.at("initial");
 auto seed=capella_store_from_snapshot(read(initial.at("file"),limits.envelope_bytes),unhex(initial.at("sha256")),rules,limits);
 Blockchain chain(argv[3],g);ChainStore witness(c.at("recoveryWitness").get<std::string>());
 CapellaDurableReactor reactor(chain,rules,limits,witness);const auto snapshot=reactor.current();
 const auto token=proof.at("expectedTokens").at(argv[5]);require(snapshot.token.sequence==value(token.at("sequence"))&&snapshot.token.digest==unhex(token.at("digest")),"stopped checkpoint changed");
 const auto finalized_bytes=unhex(proof.at("finalizedRoot"));require(finalized_bytes.size()==32,"finalized root width");const auto finalized=root(finalized_bytes);const auto found=snapshot.state->blocks.find(finalized);require(found!=snapshot.state->blocks.end(),"finalized state not retained after reopen");
 // The selected checkpoint must remain an ancestor of the store's finality.
 auto tip=snapshot.state->timed.view.finalized.root;bool ancestor=false;
 for(size_t n=0;n<=snapshot.state->blocks.size();++n){if(tip==finalized){ancestor=true;break;}auto i=snapshot.state->blocks.find(tip);if(i==snapshot.state->blocks.end()||tip==snapshot.state->timed.view.anchor)break;tip=i->second.header.parent_root;}
 require(ancestor,"requested checkpoint not finalized");const auto& block=found->second;const auto& b=block.state.ssz;const auto& t=rules.transition;const auto& l=t.consensus.state;
 require(capella_beacon_state_root(b,l,t.execution,t.maximum_state_bytes)==block.header.state_root,"finalized SSZ commitment mismatch");
 const auto slots=t.consensus.epoch.ffg.slots_per_epoch,period=t.consensus.epoch.sync.epochs_per_sync_committee_period;
 require(period&&slots&&block.header.slot/slots/period>=2,"finalized state predates nontrivial rotation");
 const auto committee=272+64*l.slots_per_historical_root+32*l.epochs_per_historical_vector+8*l.epochs_per_slashings_vector+133;
 const auto width=48*(l.sync_committee_size+1);const auto original=cut(seed.blocks.at(seed.timed.view.anchor).state.ssz,committee,width),rotated=cut(b,committee,width);
 require(original!=rotated,"committee composition never changed");
 const auto historical=176+64*l.slots_per_historical_root,registry=number(b,historical+88,4),balances=number(b,historical+92,4),count=(balances-registry)/121;
 const auto owners=proof.at("owners");require(count==64&&owners.size()==count,"expected fixed 64-validator fixture");
 std::set<unsigned> owner_groups;for(const auto&o:owners){auto n=value(o);require(n<3,"invalid owner");owner_groups.insert(unsigned(n));}require(owner_groups.size()==3,"three distinct ownership groups required");
 const std::vector<std::string> names{"mac-native","mac-docker","vps-docker"};const auto node=std::find(names.begin(),names.end(),argv[5]);require(node!=names.end(),"unknown node");const auto local_owner=size_t(node-names.begin());
 std::set<uint64_t> local_indices;for(const auto& key:c.at("validator").at("keys")){const auto index=value(key.at("index"));require(index<count&&value(owners[index])==local_owner&&local_indices.insert(index).second,"configuration ownership mismatch");}
 for(size_t i=0;i<count;++i)require((value(owners[i])==local_owner)==local_indices.contains(i),"configuration ownership incomplete");
 const auto minimum=value(proof.at("minimumInclusionSlot"));require(minimum>=2*period*slots,"recovery cutoff predates rotation");
 const auto& wires=proof.at("blocks");require(wires.is_object()&&wires.size()<=256,"capture budget exceeded");
 Json samples=Json::array();unsigned remote_blocks=0;tip=finalized;
 for(size_t depth=0;depth<64;++depth){
  auto it=wires.find(text(tip));if(it==wires.end())break;const auto wire=unhex(it->get<std::string>());require(wire.size()>=184&&wire.size()<=t.consensus.maximum_block_bytes,"signed block size");
  require(number(wire,0,4)==100&&number(wire,180,4)==84,"noncanonical signed block offsets");
  auto message=std::span<const uint8_t>(wire).subspan(100),body=message.subspan(84);
  ConsensusBeaconHeader h{number(message,0),number(message,8),root(message,16),root(message,48),capella_beacon_body_root(body,t.consensus.body,l.sync_committee_size,t.execution,t.withdrawals.maximum_per_payload,t.maximum_bls_changes,t.consensus.maximum_block_bytes)};
  require(consensus_beacon_header_root(h)==tip,"captured block commitment mismatch");require(h.proposer_index<count,"unknown proposer");
  require(verify_consensus_beacon_header(h,version(b,h.slot/slots),root(b,8),cut(b,registry+121*h.proposer_index,48),cut(wire,4,96)),"proposer signature invalid");
  tip=h.parent_root;if(h.slot<=minimum)break;
  if(h.slot/slots/period!=block.header.slot/slots/period)continue;
  const auto bit_bytes=(l.sync_committee_size+7)/8;require(body.size()>=220+bit_bytes+96,"sync aggregate bounds");
  std::vector<Bytes> keys;unsigned remote=0;
  for(size_t position=0;position<l.sync_committee_size;++position)if(body[220+position/8]&(1u<<(position%8))){
   auto key=cut(b,committee+48*position,48);size_t member=0;for(;member<count;++member)if(cut(b,registry+121*member,48)==key)break;
   require(member<count,"unknown committee member");remote+=owners[member]!=owners[h.proposer_index];keys.push_back(std::move(key));
  }
  const auto signing=consensus_signing_root(h.parent_root,consensus_domain({7,0,0,0},version(b,(h.slot-1)/slots),root(b,8)));
  require(!keys.empty()&&verify_consensus_bls_fast_aggregate(keys,signing,body.subspan(220+bit_bytes,96)),"sync aggregate signature invalid");
  if(remote)++remote_blocks;samples.push_back({{"slot",h.slot},{"participants",keys.size()},{"remotePositions",remote}});
 }
 require(remote_blocks>=3,"fewer than three finalized post-recovery aggregates with remote positions");
 require(reactor.current().token==snapshot.token,"audit mutated checkpoint");
 std::cout<<Json{{"node",argv[5]},{"finalizedRoot",text(finalized)},{"stateRoot",text(block.header.state_root)},{"stateSha256",hex(sha256(b))},{"stateSlot",block.header.slot},{"committeeHash",hex(sha256(rotated))},{"initialCommitteeHash",hex(sha256(original))},{"minimumInclusionSlot",minimum},{"remoteBlocks",remote_blocks},{"samples",samples},{"witnessReopened",true}}.dump(2)<<'\n';
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
