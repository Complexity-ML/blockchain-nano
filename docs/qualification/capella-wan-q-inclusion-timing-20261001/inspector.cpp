#include "nano/chain.hpp"
#include "nano/consensus_bls.hpp"
#include "nano/consensus_capella_reactor.hpp"
#include "nano/consensus_message_ssz.hpp"
#include <fstream>
#include <iostream>
#include <algorithm>
using namespace nano;
static Json load(const char* p){std::ifstream f(p);if(!f)throw std::runtime_error("input unavailable");return Json::parse(f);}
static std::string raw(const std::string& p){std::ifstream f(p,std::ios::binary);if(!f)throw std::runtime_error("profile unavailable");return {std::istreambuf_iterator<char>(f),{}};}
static uint64_t number(const Json& x){return u64(quantity(x));}
static size_t offset(const Bytes& b,size_t p){if(p>b.size()||b.size()-p<4)throw std::runtime_error("truncated offset");size_t n=0;for(size_t i=0;i<4;++i)n|=size_t(b[p+i])<<(8*i);return n;}
int main(int argc,char** argv){try{
 if(argc!=5)throw std::runtime_error("usage: inspect genesis node stopped-database public-stop-receipt");
 const auto g=Genesis::from_json(load(argv[1]));const auto c=load(argv[2]).at("capella"),l=c.at("limits"),receipt=load(argv[4]);
 CapellaStoreSnapshotLimits limits{number(l.at("envelopeBytes")),number(l.at("blocks")),number(l.at("messages")),number(l.at("equivocations")),number(l.at("stateBytes")),number(l.at("totalStateBytes")),number(l.at("replaySlots")),number(l.at("syncTrials"))};
 auto profile=raw(c.at("profile").at("file"));const auto rules=load_capella_fork_profile(profile,unhex(c.at("profile").at("sha256")),limits.envelope_bytes);
 ForkCheckpointToken expected{number(receipt.at("restart").at("sequence")),unhex(receipt.at("restart").at("digest"))};
 ChainStore disk(argv[3],ChainStore::OpenMode::ReadOnly);disk.bind_chain(g.chain_id,g.block.header.hash(),"beacon");
 const auto record=disk.fork_checkpoint(g.chain_id,g.block.header.hash(),limits.envelope_bytes,expected);if(!record)throw std::runtime_error("checkpoint unavailable");
 CapellaDurableReactor restored(disk,g.chain_id,g.block.header.hash(),rules,limits,expected);const auto current=restored.current();const auto& store=*current.state;
 const auto& head=capella_fork_head(store,rules);const auto& t=rules.transition;const auto context=capella_fork_block_context(head.state,head.header,{t.consensus.epoch,t.execution,t.maximum_state_bytes},t.consensus.state);
 auto root=consensus_beacon_header_root(capella_fork_head(store,rules).header);Json out=Json::array();uint64_t last=UINT64_MAX;size_t bytes_read=0;
 for(size_t count=0;count<1024;++count){
  const auto item=disk.checkpoint_archive(Bytes(root.begin(),root.end()),limits.envelope_bytes);if(!item)break;
  const auto& b=item->payload;if(item->slot>=last||b.size()<184||bytes_read>512*1024*1024-b.size())throw std::runtime_error("archive budget/slot mismatch");last=item->slot;bytes_read+=b.size();
  const auto message=offset(b,0);if(message>b.size()||b.size()-message<84)throw std::runtime_error("message bounds");
  const auto body_offset=offset(b,message+80);if(body_offset>b.size()-message)throw std::runtime_error("body bounds");const auto body=message+body_offset;if(body>b.size()||b.size()-body<216)throw std::runtime_error("body bounds");
  const auto from=offset(b,body+208),to=offset(b,body+212);if(from>to||to>b.size()-body)throw std::runtime_error("attestation bounds");const auto begin=body+from,end=body+to;
  Json votes=Json::array();if(begin<end){const auto first=offset(b,begin);if(!first||first%4||first>end-begin)throw std::runtime_error("vote offsets");const auto n=first/4;if(n>rules.transition.consensus.body.attestations)throw std::runtime_error("vote capacity");
   for(size_t i=0;i<n;++i){const auto a=offset(b,begin+4*i),z=i+1<n?offset(b,begin+4*(i+1)):end-begin;if(a<first||z<=a||z>end-begin)throw std::runtime_error("vote bounds");
    const auto vote=decode_consensus_attestation_ssz(std::span(b).subspan(begin+a,z-a),rules.transition.consensus.body.validators_per_committee);if(!vote)throw std::runtime_error("invalid stored vote");
    const auto& d=vote->data;votes.push_back({{"slot",d.slot},{"committee",d.index},{"sourceEpoch",d.source.epoch},{"targetEpoch",d.target.epoch},{"sourceRoot",hex(Bytes(d.source.root.begin(),d.source.root.end()))},{"targetRoot",hex(Bytes(d.target.root.begin(),d.target.root.end()))},{"headRoot",hex(Bytes(d.beacon_block_root.begin(),d.beacon_block_root.end()))},{"participants",std::count(vote->aggregation_bits.begin(),vote->aggregation_bits.end(),true)}});
    if(d.target.epoch>=12&&d.target.epoch<=15){
     const auto bits=std::count(vote->aggregation_bits.begin(),vote->aggregation_bits.end(),true);
     auto& identified=votes.back();identified["verifiedSignerIndices"]=Json::array();identified["singleSignerIdentificationPerformed"]=bits==1;
     if(bits==1){
      const auto version=d.target.epoch<context.fork.epoch?context.fork.previous_version:context.fork.current_version;
      const auto signing=consensus_signing_root(consensus_attestation_root(d),consensus_domain({1,0,0,0},version,context.genesis_root));
      for(const auto& [index,member]:context.registry.validators)if(verify_consensus_bls(member.public_key,signing,vote->signature))identified["verifiedSignerIndices"].push_back(index);
     }
    }
   }
  }
  out.push_back({{"slot",item->slot},{"root",hex(Bytes(root.begin(),root.end()))},{"votes",votes}});if(message>b.size()||b.size()-message<48)throw std::runtime_error("parent bounds");std::copy_n(b.begin()+message+16,32,root.begin());
 }
 std::cout<<Json{{"readOnly",true},{"externalCheckpointMatched",true},{"historicalForkChoiceVerified",false},{"fullCanonicalReplayPerformed",false},{"archiveBytes",bytes_read},{"blocks",out}}.dump(2)<<'\n';return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
