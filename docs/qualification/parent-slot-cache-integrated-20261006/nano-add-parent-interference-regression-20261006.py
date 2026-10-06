from pathlib import Path
p=Path('/Users/boris/Dev/blockchain-nano/tests/test_capella_transition.cpp');s=p.read_text();s=s.replace('incoming_alternation=false,vote_target_alternation=false;', 'incoming_alternation=false,vote_target_alternation=false,incoming_parent_interference=false;')
anchor='   if(incoming_alternation){';assert anchor in s
s=s.replace(anchor,'''   if(incoming_parent_interference){
    auto sibling_body=included_body;sibling_body[168]^=1;
    const auto sibling=preparation.construct(slot,identity->index,sibling_body,parent.execution_block.header,parent.execution_state,history,100000);
    check(bool(sibling),"parent interference sibling construction failed");const auto sibling_wire=sign(sibling->message_ssz,identity->index);
    const auto seeded=prepare_capella_received_block(timed,sibling_wire,rules,slot-parent.header.slot,100000);
    check(bool(seeded),"parent interference seed is not valid");
    const auto sibling_root=consensus_beacon_header_root(capella_fork_head(*seeded,rules).header);
    auto interleaved=prepare_capella_store_tick(*seeded,clock+rules.slot_duration_ms,rules,slot-parent.header.slot+1,100000);
    auto noise=sibling_wire;put(noise,100,slot+1,8);std::copy(sibling_root.begin(),sibling_root.end(),noise.begin()+116);noise[352]^=1;
    check(!prepare_capella_received_block(interleaved,noise,rules,slot-parent.header.slot+1,100000),"parent interference noise unexpectedly valid");
    const auto oracle=prepare_capella_received_block(interleaved,included_wire,rules,slot-parent.header.slot+1,100000);
    check(bool(oracle),"parent interference honest block not valid");
    Blockchain owner(worker_directory/"parent-interference-owner",wg);ChainStore witness(worker_directory/"parent-interference-witness");
    auto worker=std::make_unique<CapellaDurableReactor>(owner,rules,worker_limits,interleaved,std::function<void()>{},&witness);
    const auto before=worker->current().token;bool accepted=false;size_t attempts=0;const auto maximum=2*(slot-parent.header.slot)/3+12;
    while(!accepted&&attempts<maximum){++attempts;check(!worker->block(noise),"parent interference admitted invalid block");const auto prior=worker->current().token;accepted=worker->block(included_wire);if(!accepted)check(worker->current().token==prior,"parent interference published incomplete work");}
    check(accepted,"Alternating parents erased incoming block continuation");const auto current=worker->current();
    const auto honest_root=consensus_beacon_header_root({little(included_wire,100,8),little(included_wire,108,8),root_at(included_wire,116),root_at(included_wire,148),capella_beacon_body_root(std::span(included_wire).subspan(184),r.consensus.body,l.sync_committee_size,r.execution,r.withdrawals.maximum_per_payload,r.maximum_bls_changes,r.consensus.maximum_block_bytes)});
    check(current.token.sequence>before.sequence&&current.state->blocks.at(honest_root).state==oracle->blocks.at(honest_root).state&&current.state->blocks.at(honest_root).execution_state.root()==oracle->blocks.at(honest_root).execution_state.root(),"parent interference differs from full oracle");
    check(worker->signed_block(honest_root)==std::optional<Bytes>(included_wire),"parent interference signed archive differs");
    check(worker->block(included_wire)&&worker->current().token==current.token,"parent interference duplicate rewrote checkpoint");
    worker.reset();CapellaDurableReactor reopened(owner,rules,worker_limits,witness);
    check(reopened.current().token==current.token&&reopened.signed_block(honest_root)==std::optional<Bytes>(included_wire),"parent interference witness reopen lost archive");
    std::cout<<Json{{"scenario","capella-incoming-parent-interference"},{"attemptPairs",attempts},{"maximumPairs",maximum},{"honestBlockProgress",true},{"invalidAlternateParentRejected",true},{"fullGapOracleEqual",true},{"duplicateCheckpointStable",true},{"witnessReopened",true},{"networkQualified",false}}.dump()<<std::endl;return;
   }
''' +anchor)
needle='if(argc==2&&std::string(argv[1])=="--incoming-alternation")';i=s.index(needle);s=s[:i]+'if(argc==2&&std::string(argv[1])=="--incoming-parent-interference"){incoming_parent_interference=true;prepared_block=true;scenario(false);return 0;}\n'+s[i:];p.write_text(s)
p=Path('/Users/boris/Dev/blockchain-nano/CMakeLists.txt');s=p.read_text();s+='\nadd_test(NAME consensus_capella_incoming_parent_interference COMMAND nano-capella-transition-tests --incoming-parent-interference)\nset_tests_properties(consensus_capella_incoming_parent_interference PROPERTIES TIMEOUT 180 LABELS qualification_consensus_recovery)\n';p.write_text(s)
print('Added portable synthetic regression for interleaved parents, with oracle and witness checks.')
