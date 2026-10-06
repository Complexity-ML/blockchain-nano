from pathlib import Path
p=Path('/Users/boris/Dev/blockchain-nano/tests/test_capella_transition.cpp');s=p.read_text().replace('atomic_proposal=false,proposal_gap=false;','atomic_proposal=false,proposal_gap=false,incoming_alternation=false;')
needle='   auto worker_limits=crash_limits;worker_limits.replay_slots=3;\n'
assert s.count(needle)==1
block='''   if(incoming_alternation){
    // A slashable proposer sends two individually valid siblings. Alternating
    // retries must not erase authenticated target work needed by the honest one.
    auto competing_body=included_body;competing_body[168]^=1;
    const auto competing=preparation.construct(slot,identity->index,competing_body,parent.execution_block.header,parent.execution_state,history,100000);
    check(bool(competing),"alternating sibling construction failed");const auto competing_wire=sign(competing->message_ssz,identity->index);
    check(competing_wire!=included_wire&&prepare_capella_received_block(timed,competing_wire,rules,slot-parent.header.slot,100000).has_value(),"competing fixture is not an individually valid distinct block");
    Blockchain alternating_owner(worker_directory/"alternating-owner",wg);ChainStore alternating_witness(worker_directory/"alternating-witness");
    auto alternating=std::make_unique<CapellaDurableReactor>(alternating_owner,rules,worker_limits,initial,std::function<void()>{},&alternating_witness);alternating->tick(clock);
    const auto before=alternating->current().token;bool accepted=false,sibling_accepted=false;size_t attempts=0;
    const auto maximum=2*(slot-parent.header.slot)/3+12;
    while(!accepted&&attempts<maximum){
     ++attempts;for(const auto* input:{&competing_wire,&included_wire}){
      const auto token=alternating->current().token;const bool admitted=alternating->block(*input);
      if(!admitted)check(alternating->current().token==token,"incomplete alternating preparation published state");
      if(input==&included_wire)accepted=admitted;else sibling_accepted|=admitted;
     }
    }
    check(accepted,"Alternating valid blocks erased incoming target continuation");
    const auto honest_root=consensus_beacon_header_root(capella_fork_head(*expected,rules).header);const auto current=alternating->current();
    check(current.token.sequence>before.sequence&&current.state->blocks.at(honest_root).state==expected->blocks.at(honest_root).state&&current.state->blocks.at(honest_root).execution_state.root()==expected->blocks.at(honest_root).execution_state.root(),"alternating block differs from full-gap oracle");
    check(alternating->signed_block(honest_root)==std::optional<Bytes>(included_wire),"alternating block signed archive differs");
    auto invalid=competing_wire;invalid[4]^=1;check(!alternating->block(invalid)&&alternating->current().token==current.token,"target cache bypassed invalid signature");
    check(alternating->block(included_wire)&&alternating->current().token==current.token,"alternating duplicate rewrote checkpoint");
    alternating.reset();CapellaDurableReactor reopened(alternating_owner,rules,worker_limits,alternating_witness);
    check(reopened.current().token==current.token&&reopened.signed_block(honest_root)==std::optional<Bytes>(included_wire),"alternating witness reopen lost commitment/archive");
    std::cout<<Json{{"scenario","capella-incoming-target-alternation"},{"attemptPairs",attempts},{"maximumPairs",maximum},{"individuallyValidSiblings",true},{"siblingAdmitted",sibling_accepted},{"honestBlockProgress",true},{"fullGapOracleEqual",true},{"invalidSignatureRejected",true},{"witnessReopened",true},{"networkQualified",false}}.dump()<<std::endl;return;
   }
'''
s=s.replace(needle,needle+block)
s=s.replace('if(argc==2&&std::string(argv[1])=="--prepared-block")','if(argc==2&&std::string(argv[1])=="--incoming-alternation"){incoming_alternation=true;prepared_block=true;scenario(false);return 0;}\nif(argc==2&&std::string(argv[1])=="--prepared-block")')
p.write_text(s)
