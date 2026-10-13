#pragma once
#include "nano/checkpoint_blob_gc.hpp"
#include <sys/stat.h>
// Synthetic fixture only. This function never reads campaign paths or secrets.
static Json gc_token(const ForkCheckpointToken& t){return {{"sequence",quantity(Int(std::to_string(t.sequence)))},{"digest",hex(t.digest)}};}
static Json gc_limits(const CapellaStoreSnapshotLimits& l){return Json::array({quantity(Int(std::to_string(l.envelope_bytes))),quantity(Int(std::to_string(l.blocks))),quantity(Int(std::to_string(l.messages))),quantity(Int(std::to_string(l.equivocations))),quantity(Int(std::to_string(l.state_bytes))),quantity(Int(std::to_string(l.total_state_bytes))),quantity(Int(std::to_string(l.replay_slots))),quantity(Int(std::to_string(l.sync_trials)))});}
#include "capella_blob_gc_adversarial.hpp"
#include "capella_blob_gc_cli.hpp"
static void qualify_capella_blob_gc_next_publication(const fs::path& incoming_dir,const Genesis& g,
 const CapellaForkBlockRules& rules,const CapellaStoreSnapshotLimits& limits,
 const std::string& prior_checkpoint,const Bytes& child,const Bytes& public_key,
 const ConsensusRoot& validators_root,uint64_t slots){
 const auto dir=fs::canonical(incoming_dir);
 const bool use_cli=std::getenv("NANO_TEST_CHECKPOINT_BLOB_GC_CLI")!=nullptr;
 check(!(use_cli&&std::getenv("NANO_TEST_CHECKPOINT_BLOB_GC_ADVERSARIAL")),"GC CLI and adverse variants are distinct");
 check(!test::isolated_capella::active,"GC test must not operate on a campaign export");
 const auto source=dir/"owner",source_witness=dir/"witness";
 check(::chmod(source.c_str(),0700)==0&&::chmod(source_witness.c_str(),0700)==0,"GC owned synthetic source permissions");
 ForkCheckpointToken primary_pin,witness_pin;
 const CheckpointBlob orphan{sha256(Bytes(prior_checkpoint.begin(),prior_checkpoint.end())),prior_checkpoint};
 // Stage an explicitly synthetic orphan using the real synchronized CAS APIs.
 // Its bytes are a prior validated checkpoint payload; no live root is forged.
 // This is not a measurement of naturally accumulated campaign garbage.
 {
  ChainStore primary(source),witness(source_witness);
  const auto before=primary.fork_checkpoint(g.chain_id,g.block.header.hash(),limits.envelope_bytes);
  const auto saved_witness=witness.fork_checkpoint(g.chain_id,g.block.header.hash(),8*limits.envelope_bytes+2048);
  const auto tip=primary.latest();check(before&&saved_witness&&tip,"GC fixture bindings missing");
  check(!primary.checkpoint_blob(g.chain_id,g.block.header.hash(),orphan.digest,limits.envelope_bytes),"GC orphan collides with current fragment");
  const ForkCheckpointToken next{before->token.sequence+1,before->token.digest};
  const auto intent=Json{{"format","nano-capella-recovery-v1"},{"previous",gc_token(before->token)},{"next",gc_token(next)},{"snapshot",before->snapshot},{"archive",Json::array()}}.dump();
  witness_pin=witness.commit_fork_checkpoint_with_blobs(g.chain_id,g.block.header.hash(),saved_witness->token,intent,8*limits.envelope_bytes+2048,{orphan},limits.blocks);
  primary_pin=primary.commit_execution_checkpoint_with_blobs(g.chain_id,g.block.header.hash(),before->token,tip->header.hash(),tip->header.number,tip->header.hash(),{},before->snapshot,limits.envelope_bytes,limits.blocks,{orphan},limits.blocks);
  check(primary_pin==next&&primary.checkpoint_blob(g.chain_id,g.block.header.hash(),orphan.digest,limits.envelope_bytes)==orphan.payload,"GC staging not committed");
 }
 if(std::getenv("NANO_TEST_CHECKPOINT_BLOB_GC_ADVERSARIAL")){qualify_capella_gc_adversarial(source,source_witness,g,rules,limits,child);return;}
 const auto signer_path=dir/"gc-protected-signer",signer_witness_path=dir/"gc-protected-signer-witness";
 ForkCheckpointToken signer_pin,signer_witness_pin;
 {
  ChainStore signer(signer_path),witness(signer_witness_path);
  BeaconSigningJournal journal(signer,g.chain_id,g.block.header.hash(),validators_root,public_key,slots,std::nullopt,&witness);
  // An imported synthetic durable reservation, with no BLS provider call.
  const auto reserved=hex(sha256(Bytes{'g','c','-','r','e','s','e','r','v','e','d'}));
  const auto interchange=Json{{"metadata",{{"interchange_format_version","5"},{"genesis_validators_root",hex(Bytes(validators_root.begin(),validators_root.end()))}}},{"data",Json::array({Json{{"pubkey",hex(public_key)},{"signed_blocks",Json::array({Json{{"slot","1"},{"signing_root",reserved}}})},{"signed_attestations",Json::array()}}})}}.dump();
  check(journal.import_interchange(interchange,65536)==1&&journal.latest_proposal_slot()==1,"GC signer reservation import");signer_pin=journal.commitment();
  const auto saved=witness.fork_checkpoint(g.chain_id,g.block.header.hash(),limits.envelope_bytes);check(bool(saved),"GC signer witness missing");signer_witness_pin=saved->token;
 }
 check(::chmod(signer_path.c_str(),0700)==0&&::chmod(signer_witness_path.c_str(),0700)==0,"GC protected directory permissions");
 const CheckpointBlobGcBudget budget{1024,128*1024*1024,16384,128*1024*1024,1024,64*1024*1024,1024*1024,60000};
 const auto inventory=[&](const fs::path& p){return checkpoint_blob_gc_copy_inventory(p,budget.inventory_files,budget.inventory_bytes);};
 const auto original=inventory(source),original_witness=inventory(source_witness),original_signer=inventory(signer_path),original_signer_witness=inventory(signer_witness_path);
 const auto parent=dir/"gc-maintained-copies";check(fs::create_directory(parent)&&::chmod(parent.c_str(),0700)==0,"GC new owned copy root");
 const auto pc=parent/"primary",wc=parent/"witness";fs::copy(source,pc,fs::copy_options::recursive);fs::copy(source_witness,wc,fs::copy_options::recursive);
 check(::chmod(pc.c_str(),0700)==0&&::chmod(wc.c_str(),0700)==0,"GC copy permissions");
 {std::ofstream marker(parent/".nano-owned-checkpoint-gc-copy");marker<<"owned-offline-checkpoint-gc-v1\n";check(bool(marker),"GC marker write");}
 const auto pi=inventory(pc),wi=inventory(wc);check(pi==original&&wi==original_witness,"GC source/copy inventory differs");
 const auto stopped=Json{{"format","nano-stopped-checkpoint-copy-receipt-v1"},{"scope","isolated-owned-copies"},{"allWritersStopped",true},{"allReadersStopped",true},{"stoppedAtMs",quantity(Int(std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count())))},{"copies",Json::array({Json{{"role","primary"},{"path",pc.string()},{"inventorySha256",pi},{"sourcePath",source.string()},{"sourceInventorySha256",original}},Json{{"role","witness"},{"path",wc.string()},{"inventorySha256",wi},{"sourcePath",source_witness.string()},{"sourceInventorySha256",original_witness}}})}}.dump();
 const auto stop_sha=sha256(Bytes(stopped.begin(),stopped.end()));const auto profile=capella_fork_profile(rules).dump();
 const auto request=Json{{"format","nano-owned-stopped-checkpoint-gc-v1"},{"chainId",quantity(g.chain_id)},{"genesisHash",hex(g.block.header.hash())},{"profileSha256",hex(sha256(Bytes(profile.begin(),profile.end()))).substr(2)},
  {"primary",{{"path",pc.string()},{"inventorySha256",pi},{"token",gc_token(primary_pin)},{"sourcePath",source.string()},{"sourceInventorySha256",original}}},{"witness",{{"path",wc.string()},{"inventorySha256",wi},{"token",gc_token(witness_pin)},{"sourcePath",source_witness.string()},{"sourceInventorySha256",original_witness}}},{"next",gc_token(primary_pin)},{"stopReceiptSha256",hex(stop_sha).substr(2)},{"limits",gc_limits(limits)},{"budget",Json::array({"0x400","0x8000000","0x4000","0x8000000","0x400","0x4000000","0x100000","0xea60"})}}.dump();
 if(use_cli){gc_cli_collect_closed(dir,request,stopped,profile,budget,primary_pin,witness_pin,orphan.digest,{source,source_witness,signer_path,signer_witness_path});}
 else{
  auto maintenance=CheckpointBlobGcCopies::open(request,sha256(Bytes(request.begin(),request.end())),stopped,stop_sha,budget);
  const auto plan=maintenance->plan(rules,limits,budget);const auto description=plan.description();
  check(description.at("deletePrimary").size()==1&&description.at("deleteWitness").size()==1&&description.at("deletePrimary")[0][1]==hex(orphan.digest)&&description.at("deleteWitness")[0][1]==hex(orphan.digest),"GC exact two orphan keys not isolated");
  const auto result=maintenance->collect(plan);check(result.at("noWork")==false&&result.at("allProtectedReferencesReopened")==true&&result.at("nonGcNamespacesUnchanged")==true,"GC deletion/cold verification did not complete");
 }
 ForkCheckpointToken advanced,advanced_witness;std::string advanced_snapshot;
 {
  Blockchain owner(pc,g);ChainStore witness(wc);CapellaDurableReactor reactor(owner,rules,limits,witness,{},primary_pin);
  check(reactor.current().token==primary_pin&&!owner.store().checkpoint_blob(g.chain_id,g.block.header.hash(),orphan.digest,limits.envelope_bytes)&&!witness.checkpoint_blob(g.chain_id,g.block.header.hash(),orphan.digest,limits.envelope_bytes),"GC cold reopen changed commitment or retained orphan");
  const auto identity=reactor.network_identity();bool accepted=false;
  for(unsigned attempt=0;attempt<64&&!accepted;++attempt)accepted=reactor.block(child);
  check(accepted,"GC next child not normally admitted within finite bound");advanced=reactor.current().token;check(advanced.sequence>primary_pin.sequence&&reactor.network_identity()==identity,"GC next publication failed/domain changed");
  const auto snapshot=owner.store().fork_checkpoint(g.chain_id,g.block.header.hash(),limits.envelope_bytes,advanced);const auto recovery=witness.fork_checkpoint(g.chain_id,g.block.header.hash(),8*limits.envelope_bytes+2048);
  check(snapshot&&recovery&&Json::parse(recovery->snapshot).at("next")==gc_token(advanced)&&Json::parse(recovery->snapshot).at("snapshot")==snapshot->snapshot,"GC next witness/primary binding failed");advanced_snapshot=snapshot->snapshot;advanced_witness=recovery->token;
  check(reactor.block(child)&&reactor.current().token==advanced,"GC replay changed next commitment");
 }
 {
  Blockchain owner(pc,g);ChainStore witness(wc);CapellaDurableReactor reactor(owner,rules,limits,witness,{},advanced);
  const auto snapshot=owner.store().fork_checkpoint(g.chain_id,g.block.header.hash(),limits.envelope_bytes,advanced);
  check(snapshot&&reactor.current().token==advanced&&snapshot->snapshot==advanced_snapshot&&witness.fork_checkpoint(g.chain_id,g.block.header.hash(),8*limits.envelope_bytes+2048,advanced_witness).has_value(),"GC second cold reopen differs");
 }
 {ChainStore signer(signer_path,ChainStore::OpenMode::ReadOnly),witness(signer_witness_path,ChainStore::OpenMode::ReadOnly);check(signer.fork_checkpoint(g.chain_id,g.block.header.hash(),limits.envelope_bytes,signer_pin).has_value()&&witness.fork_checkpoint(g.chain_id,g.block.header.hash(),limits.envelope_bytes,signer_witness_pin).has_value(),"GC separate signer/witness pins changed");}
 check(inventory(source)==original&&inventory(source_witness)==original_witness&&inventory(signer_path)==original_signer&&inventory(signer_witness_path)==original_signer_witness,"GC original/protected bytes changed");
 std::cout<<Json{{"scenario","gc-cold-reopen-and-next-admission"},{"collectorInterface",use_cli?"actual-cli":"cpp-api"},{"syntheticOrphanStagedViaCas",true},{"naturallyObservedCampaignLeak",false},{"deletedPrimaryRecords",1},{"deletedWitnessRecords",1},{"sameCheckpointColdReopened",true},{"nextChildNormallyAdmitted",true},{"nextPublicationColdReopened",true},{"originalAndSignerWitnessBytesUnchanged",true},{"localSigningProviderCalls",0},{"networkQualified",false},{"runtimeDutiesResumed",false},{"productionQualified",false}}.dump()<<std::endl;
}
struct NaturalGcCopies {fs::path primary,witness;Json result;std::string original_inventory,witness_inventory;};
// Called only after the real synthetic publication/pruning loop closes its
// reactor, owners and witness handles. No artificial blob is injected here.
static NaturalGcCopies collect_naturally_pruned_capella_copy(const fs::path& incoming_source,const fs::path& incoming_witness,
 const fs::path& incoming_destination,const Genesis& g,const CapellaForkBlockRules& rules,
 const CapellaStoreSnapshotLimits& limits,const ForkCheckpointToken& primary_pin,const ForkCheckpointToken& witness_pin){
 const auto source=fs::canonical(incoming_source),source_witness=fs::canonical(incoming_witness);
 const auto destination=fs::canonical(incoming_destination.parent_path())/incoming_destination.filename();
 check(!test::isolated_capella::active,"natural GC cannot operate on campaign data");
 const CheckpointBlobGcBudget budget{2048,256*1024*1024,32768,256*1024*1024,4096,128*1024*1024,1024*1024,60000};
 check(::chmod(source.c_str(),0700)==0&&::chmod(source_witness.c_str(),0700)==0,"natural GC closed source permissions");
 const auto inventory=[&](const fs::path& p){return checkpoint_blob_gc_copy_inventory(p,budget.inventory_files,budget.inventory_bytes);};
 const auto original=inventory(source),original_witness=inventory(source_witness);
 check(fs::create_directory(destination)&&::chmod(destination.c_str(),0700)==0,"natural GC fresh destination required");
 const auto pc=destination/"primary",wc=destination/"witness";fs::copy(source,pc,fs::copy_options::recursive);fs::copy(source_witness,wc,fs::copy_options::recursive);
 check(::chmod(pc.c_str(),0700)==0&&::chmod(wc.c_str(),0700)==0,"natural GC private copies");
 {std::ofstream marker(destination/".nano-owned-checkpoint-gc-copy");marker<<"owned-offline-checkpoint-gc-v1\n";check(bool(marker),"natural GC marker");}
 const auto pi=inventory(pc),wi=inventory(wc);check(pi==original&&wi==original_witness,"natural GC copies not physically exact");
 const auto stopped=Json{{"format","nano-stopped-checkpoint-copy-receipt-v1"},{"scope","isolated-owned-copies"},{"allWritersStopped",true},{"allReadersStopped",true},{"stoppedAtMs",quantity(Int(std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count())))},{"copies",Json::array({Json{{"role","primary"},{"path",pc.string()},{"inventorySha256",pi},{"sourcePath",source.string()},{"sourceInventorySha256",original}},Json{{"role","witness"},{"path",wc.string()},{"inventorySha256",wi},{"sourcePath",source_witness.string()},{"sourceInventorySha256",original_witness}}})}}.dump();
 const auto stop_sha=sha256(Bytes(stopped.begin(),stopped.end()));const auto profile=capella_fork_profile(rules).dump();
 const auto request=Json{{"format","nano-owned-stopped-checkpoint-gc-v1"},{"chainId",quantity(g.chain_id)},{"genesisHash",hex(g.block.header.hash())},{"profileSha256",hex(sha256(Bytes(profile.begin(),profile.end()))).substr(2)},
  {"primary",{{"path",pc.string()},{"inventorySha256",pi},{"token",gc_token(primary_pin)},{"sourcePath",source.string()},{"sourceInventorySha256",original}}},{"witness",{{"path",wc.string()},{"inventorySha256",wi},{"token",gc_token(witness_pin)},{"sourcePath",source_witness.string()},{"sourceInventorySha256",original_witness}}},{"next",gc_token(primary_pin)},{"stopReceiptSha256",hex(stop_sha).substr(2)},{"limits",gc_limits(limits)},{"budget",Json::array({"0x800","0x10000000","0x8000","0x10000000","0x1000","0x8000000","0x100000","0xea60"})}}.dump();
 Json result;
 {auto maintenance=CheckpointBlobGcCopies::open(request,sha256(Bytes(request.begin(),request.end())),stopped,stop_sha,budget);const auto plan=maintenance->plan(rules,limits,budget);const auto description=plan.description();
  // A fixture failing to create a natural orphan is a qualification refusal.
  // Do not inject a digest, report NO_WORK as cleanup, or silently skip.
  check(!description.at("deletePrimary").empty()&&!description.at("deleteWitness").empty(),"natural cold publication/pruning did not produce an orphan; natural leak gate remains open");
  result=maintenance->collect(plan);check(result.at("noWork")==false&&result.at("allProtectedReferencesReopened")==true&&result.at("nonGcNamespacesUnchanged")==true,"natural GC did not cold-verify deletion");
 }
 check(inventory(source)==original&&inventory(source_witness)==original_witness,"natural GC modified original publications");
 return {pc,wc,result,original,original_witness};
}
