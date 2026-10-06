from pathlib import Path
import json,subprocess,shutil,hashlib
old=Path('/private/tmp/nano-v12-frontier-oracle-20261006');out=Path('/private/tmp/nano-parent-interference-20261006');out.mkdir(exist_ok=True)
s=(old/'diagnose.cpp').read_text();at=s.index(' Json results=Json::array()');s=s[:at]+r'''
 auto& source=sources[0];auto& recipient=sources[1];const auto baseline=recipient.reactor->current();
 std::optional<ConsensusRoot> honest;
 for(const auto& [id,b]:source.reactor->current().state->blocks)if(!baseline.state->blocks.contains(id)&&baseline.state->blocks.contains(b.header.parent_root)){honest=id;break;}
 need(bool(honest),"no missing frontier");const auto h=source.reactor->current().state->blocks.at(*honest).header;
 need(h.slot-baseline.state->blocks.at(h.parent_root).header.slot<=256,"frontier gap beyond diagnostic bound");
 const auto valid=source.reactor->signed_block(*honest);need(bool(valid),"frontier wire missing");
 const auto oracle=prepare_capella_received_block(*baseline.state,*valid,recipient.rules,256,recipient.limits.sync_trials);need(bool(oracle),"valid frontier oracle rejected");
 const CapellaForkBlock* alternate=nullptr;ConsensusRoot alternate_root{};
 for(const auto& [id,b]:baseline.state->blocks)if(b.header.parent_root!=h.parent_root&&baseline.state->blocks.contains(b.header.parent_root)&&b.header.slot>baseline.state->blocks.at(b.header.parent_root).header.slot&&b.header.slot-baseline.state->blocks.at(b.header.parent_root).header.slot<=16&&(!alternate||b.header.slot>alternate->header.slot)){alternate=&b;alternate_root=id;}
 need(alternate,"no short alternate parent fixture");auto noise=recipient.reactor->signed_block(alternate_root);need(bool(noise)&&noise->size()>352,"alternate wire missing");(*noise)[352]^=1;
 need(!prepare_capella_received_block(*baseline.state,*noise,recipient.rules,256,recipient.limits.sync_trials),"altered block unexpectedly valid");
 const auto run=[&](const std::string& mode,bool interfere){
  ChainStore disk(request.at(mode+"Database").get<std::string>());disk.bind_chain(genesis.chain_id,genesis.block.header.hash(),"beacon");
  CapellaDurableReactor worker(disk,genesis.chain_id,genesis.block.header.hash(),recipient.rules,recipient.limits,recipient.expected);
  const auto before=worker.current().token;bool admitted=false;size_t rounds=0;
  for(;rounds<100&&!admitted;++rounds){if(interfere)need(!worker.block(*noise),"invalid block admitted");admitted=worker.block(*valid);if(!admitted)need(worker.current().token==before,"incomplete work published state");}
  const bool under_interference=admitted;size_t quiet=0;
  while(!admitted&&quiet++<100)admitted=worker.block(*valid);
  need(admitted,"frontier never progresses even without interference");const auto after=worker.current();
  need(after.state->blocks.at(*honest).state==oracle->blocks.at(*honest).state&&after.state->blocks.at(*honest).execution_state.root()==oracle->blocks.at(*honest).execution_state.root(),"incremental transition differs from full oracle");
  need(worker.signed_block(*honest)==valid,"archive differs from authenticated wire");need(worker.block(*valid)&&worker.current().token==after.token,"duplicate changed checkpoint");
  return Json{{"mode",mode},{"rounds",rounds},{"admittedDuringInterference",under_interference},{"quietRecoveryAttempts",quiet},{"fullOracleEqual",true},{"invalidBlockRejected",true},{"duplicateCheckpointStable",true}};
 };
 const auto control=run("control",false),interference=run("interference",true);
 std::cout<<Json{{"scenario","actual-durable-parent-preparation-interference"},{"honestSlot",h.slot},{"honestParentSlot",baseline.state->blocks.at(h.parent_root).header.slot},{"noiseSlot",alternate->header.slot},{"noiseParentSlot",baseline.state->blocks.at(alternate->header.parent_root).header.slot},{"control",control},{"interference",interference},{"counterexampleObserved",!interference.at("admittedDuringInterference").get<bool>()},{"originalStoresReadOnly",true},{"copiesSameMac",true},{"signingKeysOrJournalsCopied",false},{"activeNetworkCauseProven",false},{"productionQualified",false}}.dump(2)<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
'''
(out/'diagnose.cpp').write_text(s)
req=json.loads((old/'request.json').read_text());src=Path(req['sources'][1]['database']);expected=json.loads(Path('/private/tmp/nano-v12-vote-cache-post-stop-native-diagnostic-20261006/mac-docker-before.json').read_text());expected={p[len('chain/'):]:sha for p,sha in expected.items() if p.startswith('chain/')}
def hashes(d):return {str(p.relative_to(d)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(d.rglob('*')) if p.is_file()}
assert hashes(src)==expected
for mode in ['control','interference']:
 target=out/(mode+'-chain');assert not target.exists();subprocess.run(['/bin/cp','-cR',str(src),str(target)],check=True);assert hashes(target)==expected;req[mode+'Database']=str(target)
(out/'request.json').write_text(json.dumps(req,indent=2)+'\n');(out/'source-before.json').write_text(json.dumps(expected,indent=2)+'\n')
for name in ['compile','link']:
 args=json.loads((old/(name+'-command.json')).read_text());args=[str(out/Path(x).name) if x.startswith(str(old)+'/') else x for x in args]
 (out/(name+'-command.json')).write_text(json.dumps(args,indent=2)+'\n')
 with (out/(name+'.log')).open('w') as f:r=subprocess.run(args,cwd='/tmp/nano-pruning-wan-native-build-20261004',stdout=f,stderr=subprocess.STDOUT)
 (out/(name+'-exit.json')).write_text(json.dumps({'exitCode':r.returncode})+'\n');assert r.returncode==0,name
print('Two same-Mac chain-only clones verified; no signing journal or key copied.')
with (out/'result.json').open('w') as f,(out/'error.log').open('w') as e:r=subprocess.run([str(out/'diagnose'),str(out/'request.json')],stdout=f,stderr=e)
(out/'exit.json').write_text(json.dumps({'exitCode':r.returncode})+'\n');assert hashes(src)==expected;assert r.returncode==0
print((out/'result.json').read_text())
