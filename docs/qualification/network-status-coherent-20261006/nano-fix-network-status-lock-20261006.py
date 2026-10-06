from pathlib import Path
repo=Path('/Users/boris/Dev/blockchain-nano')
p=repo/'src/consensus_capella_reactor.cpp';s=p.read_text();old='Json CapellaDurableReactor::network_identity()const{\n std::lock_guard lock(mutex_);return network_identity_locked();\n}'
new='''Json CapellaDurableReactor::network_identity()const{
 // Read an authenticated immutable publication without waiting for admission.
 // fail_closed clears this publication; the fallback retains the latched error.
 const auto published=beacon_snapshot();return network_identity_for_store(*published.state);
}'''
assert s.count(old)==1;p.write_text(s.replace(old,new))
p=repo/'include/nano/consensus_capella_reactor.hpp';s=p.read_text();old=' std::array<uint64_t,static_cast<size_t>(SyncOutcome::count)> sync_outcomes_{};';new=''' // Telemetry reads must not wait behind execution or signing admission.
 // Mutators hold mutex_ first, then this short-lived stats lock. Readers never
 // acquire mutex_ while holding it; all counter changes use the same lock.
 mutable std::mutex sync_stats_mutex_;
 std::array<uint64_t,static_cast<size_t>(SyncOutcome::count)> sync_outcomes_{};''';assert s.count(old)==1;p.write_text(s.replace(old,new))
p=repo/'src/consensus_capella_local_body.cpp';s=p.read_text()
for name in ['hits','builds']:
 old=f'if(sync_context_{name}_!=UINT64_MAX)++sync_context_{name}_;'
 new='{std::lock_guard stats_lock(sync_stats_mutex_);'+old+'}'
 assert s.count(old)==1;s=s.replace(old,new)
s=s.replace(' auto& count=sync_outcomes_[static_cast<size_t>(outcome)];',' std::lock_guard stats_lock(sync_stats_mutex_);auto& count=sync_outcomes_[static_cast<size_t>(outcome)];')
old='Json CapellaDurableReactor::sync_admission_stats()const{\n std::lock_guard lock(mutex_);';new='Json CapellaDurableReactor::sync_admission_stats()const{\n std::lock_guard lock(sync_stats_mutex_);';assert s.count(old)==1;s=s.replace(old,new);p.write_text(s)
p=repo/'tests/test_capella_transition.cpp';current=p.read_text();draft=Path('/private/tmp/nano-network-status-lock-probe-repair-20261006/test_capella_transition.cpp').read_text();a=draft.index('#ifdef NANO_TEST_NETWORK_STATUS_PROBE');b=draft.index('#endif',a);probe=draft[a+len('#ifdef NANO_TEST_NETWORK_STATUS_PROBE'):b];at='static void verify_beacon_read_admission(CapellaDurableReactor& reactor,const Bytes& wire,const ConsensusRoot& parent){';assert current.count(at)==1;p.write_text(current.replace(at,at+probe))
p=repo/'CMakeLists.txt';s=p.read_text();at='set_tests_properties(consensus_capella_beacon_reads PROPERTIES TIMEOUT 120 LABELS "qualification_consensus_runtime")';assert s.count(at)==1;s=s.replace(at,at+'''
add_test(NAME consensus_capella_network_status_identity COMMAND nano-capella-transition-tests --beacon-reads)
set_tests_properties(consensus_capella_network_status_identity PROPERTIES TIMEOUT 120 LABELS "qualification_consensus_runtime" ENVIRONMENT "NANO_TEST_NETWORK_STATUS_FIELD=identity")
add_test(NAME consensus_capella_network_status_sync COMMAND nano-capella-transition-tests --beacon-reads)
set_tests_properties(consensus_capella_network_status_sync PROPERTIES TIMEOUT 120 LABELS "qualification_consensus_runtime" ENVIRONMENT "NANO_TEST_NETWORK_STATUS_FIELD=sync")''');p.write_text(s)
print('Functional status contention correction written; coherent ABI rebuild and qualification pending')
