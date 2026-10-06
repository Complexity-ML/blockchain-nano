from pathlib import Path
repo=Path('/Users/boris/Dev/blockchain-nano');prototype=repo/'docs/qualification/vote-target-alternation-prototype-20261006'
original=(repo/'src/consensus_capella_reactor.cpp').read_text();candidate=(prototype/'candidate-reactor.cpp').read_text()
a=original.index('CapellaTargetPreparation* CapellaDurableReactor::prepare_vote_target_locked(');b=original.index('bool CapellaDurableReactor::vote(',a)
c=candidate.index('CapellaTargetPreparation* CapellaDurableReactor::prepare_vote_target_locked(');d=candidate.index('bool CapellaDurableReactor::vote(',c)
assert original[:a]==candidate[:c] and original[b:]==candidate[d:]
(repo/'src/consensus_capella_reactor.cpp').write_text(original[:a]+candidate[c:d]+original[b:])
p=repo/'include/nano/consensus_capella_reactor.hpp';s=p.read_text().replace('Last requested block does not invalidate authenticated continuation work.','Blocks and votes share this cache; request identity never discards prefixes.');p.write_text(s)
p=repo/'tests/test_capella_transition.cpp';s=p.read_text();s=s.replace('proposal_gap=false,incoming_alternation=false;','proposal_gap=false,incoming_alternation=false,vote_target_alternation=false;')
proto=(prototype/'test-qualified.cpp').read_text();a=proto.index('   if(incoming_alternation){');b=proto.index('   {\n    Blockchain warm_owner',a)
part=proto[a:b].replace('if(incoming_alternation)', 'if(vote_target_alternation)');part='\n'.join(line for line in part.split('\n') if 'const auto first_root=' not in line)
a=s.index('   if(incoming_alternation){');s=s[:a]+part+s[a:]
needle='if(argc==2&&std::string(argv[1])=="--incoming-alternation"){incoming_alternation=true;prepared_block=true;scenario(false);return 0;}'
assert s.count(needle)==1;s=s.replace(needle,needle+'\nif(argc==2&&std::string(argv[1])=="--vote-target-alternation"){vote_target_alternation=true;prepared_block=true;scenario(false);return 0;}');p.write_text(s)
p=repo/'CMakeLists.txt';s=p.read_text();needle='add_test(NAME consensus_capella_incoming_target_alternation COMMAND nano-capella-transition-tests --incoming-alternation)';assert s.count(needle)==1;s=s.replace(needle,'add_test(NAME consensus_capella_vote_target_alternation COMMAND nano-capella-transition-tests --vote-target-alternation)\nset_tests_properties(consensus_capella_vote_target_alternation PROPERTIES TIMEOUT 180 LABELS "qualification_consensus_recovery")\n'+needle);p.write_text(s)
