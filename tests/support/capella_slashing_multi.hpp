// Included inside the synthetic finality loop; no production code.
std::optional<std::pair<IndexedConsensusAttestation,IndexedConsensusAttestation>> multi_proof;
Bytes multi_wire;
bool multi_partial=false,multi_complete=false;
if(slashing_multi){
 multi_proof=*retirement_proof;
 const auto anchor_pin=initial.blocks.at(initial.timed.view.anchor).lease();const auto& anchor=*anchor_pin;
 const auto context=capella_fork_block_context(anchor.state,anchor.header,{t.consensus.epoch,t.execution,t.maximum_state_bytes},t.consensus.state);
 for(auto* att:{&multi_proof->first,&multi_proof->second}){
  att->indices={0,1};const auto version=att->data.target.epoch<context.fork.epoch?context.fork.previous_version:context.fork.current_version;
  const auto root=consensus_signing_root(consensus_attestation_root(att->data),consensus_domain({1,0,0,0},version,context.genesis_root));
  blst_p2 sum{};bool first=true;
  for(auto index:att->indices){const auto signature=Key(index).sign(root);blst_p2_affine affine;check(blst_p2_uncompress(&affine,signature.data())==BLST_SUCCESS,"multi proof signature encoding");blst_p2 point;blst_p2_from_affine(&point,&affine);if(first){sum=point;first=false;}else blst_p2_add_or_double(&sum,&sum,&point);}
  att->signature.resize(96);blst_p2_compress(att->signature.data(),&sum);
 }
 multi_wire=encode_consensus_attester_slashing_ssz(multi_proof->first,multi_proof->second,t.consensus.body.validators_per_committee);
 for(auto& node:nodes){check(node->slashing(multi_proof->first,multi_proof->second),"multi proof admission rejected");check(node->signed_slashings().size()==2,"overlapping proof lost uncovered offender");const auto token=node->current().token;check(node->slashing(multi_proof->second,multi_proof->first)&&node->current().token==token,"reversed aggregate duplicated archive");}
}
// Delay offender 1's inclusion until offender 0 is actually finalized slashed.
// This is a valid proposer choice, not an altered transition or forged state.
auto filter_slashing_body=[&](Bytes& body){
 if(multi_partial)return;
 const auto begin=little(body,204,4),end=little(body,208,4);
 const auto snapshot=nodes[0]->current();const auto h_pin=capella_fork_head(*snapshot.state,rules);const auto& h=*h_pin;
 const auto context=capella_fork_block_context(h.state,h.header,{t.consensus.epoch,t.execution,t.maximum_state_bytes},t.consensus.state);
 Bytes selected;
 if(!context.registry.validators.at(0).slashed){const auto wire=encode_consensus_attester_slashing_ssz(retirement_proof->first,retirement_proof->second,t.consensus.body.validators_per_committee);selected.resize(4);put(selected,0,4,4);selected.insert(selected.end(),wire.begin(),wire.end());}
 body.erase(body.begin()+begin,body.begin()+end);body.insert(body.begin()+begin,selected.begin(),selected.end());
 const auto sync=(t.consensus.state.sync_committee_size+7)/8+96;
 for(const auto at:std::array<size_t,5>{208,212,216,220+sync,224+sync})put(body,at,little(body,at,4)-(end-begin)+selected.size(),4);
};
auto verify_multi_slashing=[&]{
 for(auto& node:nodes){
  const auto snapshot=node->current();const auto final_pin=snapshot.state->blocks.at(snapshot.state->timed.view.finalized.root).lease();const auto& final=*final_pin;
  const auto context=capella_fork_block_context(final.state,final.header,{t.consensus.epoch,t.execution,t.maximum_state_bytes},t.consensus.state);
  const bool zero=context.registry.validators.at(0).slashed,one=context.registry.validators.at(1).slashed;
  const auto archive=node->signed_slashings();bool aggregate=false;for(const auto& [id,wire]:archive)if(wire==multi_wire)aggregate=true;
  check(aggregate==!(zero&&one),"aggregate proof retired with unfinalized offender or retained after both");
  check(archive.size()==(zero?one?0:1:2),"multi slashing archive count differs from finalized registry");
  const auto token=snapshot.token;check(node->slashing(multi_proof->second,multi_proof->first)&&node->current().token==token,"aggregate replay changed archive checkpoint");
  if(zero&&!one)multi_partial=true;
  if(zero&&one)multi_complete=true;
 }
};
