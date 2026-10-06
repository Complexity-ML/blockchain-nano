
# Qualified cold-checkpoint gates run independently: the prepared-block cold
# reactor branch returns early and cannot cover the writer-memo branch.
add_test(NAME consensus_capella_cold_reference_writer COMMAND ${CMAKE_COMMAND} -E env NANO_TEST_REFERENCE_VALIDATION_MEMO=1 NANO_TEST_COMPACT_REFERENCE_RESTORE=1 NANO_TEST_COMPACT_REFERENCE_WRITER=1 NANO_TEST_COLD_RECORD_CACHE=1 NANO_TEST_FRAGMENT_CHECKPOINT=1 $<TARGET_FILE:nano-capella-transition-tests> --prepared-block)
set_tests_properties(consensus_capella_cold_reference_writer PROPERTIES TIMEOUT 240 LABELS qualification_consensus_recovery)
add_test(NAME consensus_capella_cold_reactor_guards COMMAND ${CMAKE_COMMAND} -E env NANO_TEST_RUNTIME_REFERENCE_MEMO=1 NANO_TEST_COLD_REACTOR=1 NANO_TEST_FRAGMENT_CHECKPOINT=1 NANO_TEST_DUPLICATE_GUARDS=1 $<TARGET_FILE:nano-capella-transition-tests> --prepared-block)
set_tests_properties(consensus_capella_cold_reactor_guards PROPERTIES TIMEOUT 240 LABELS qualification_consensus_recovery)
add_test(NAME consensus_capella_cold_runtime_recovery COMMAND ${CMAKE_COMMAND} -E env NANO_TEST_RUNTIME_REFERENCE_MEMO=1 NANO_TEST_COLD_RUNTIME=1 $<TARGET_FILE:nano-capella-transition-tests> --automatic-recovery)
set_tests_properties(consensus_capella_cold_runtime_recovery PROPERTIES TIMEOUT 1200 LABELS qualification_consensus_recovery)
add_test(NAME consensus_capella_cold_runtime_bls COMMAND ${CMAKE_COMMAND} -E env NANO_TEST_RUNTIME_REFERENCE_MEMO=1 NANO_TEST_COLD_RUNTIME=1 $<TARGET_FILE:nano-capella-transition-tests> --queued-bls-rpc)
set_tests_properties(consensus_capella_cold_runtime_bls PROPERTIES TIMEOUT 1200 LABELS qualification_consensus_recovery)
if(TARGET nano-test-sync-fault)
 add_test(NAME consensus_capella_cold_memo_sync_recovery COMMAND ${CMAKE_COMMAND} -E env NANO_TEST_FRAGMENT_CHECKPOINT=1 NANO_TEST_COLD_REACTOR=1 NANO_TEST_RUNTIME_REFERENCE_MEMO=1 NANO_TEST_COLD_MEMO_SYNC=1 $<TARGET_FILE:nano-capella-transition-tests> --prepared-block)
 set_tests_properties(consensus_capella_cold_memo_sync_recovery PROPERTIES TIMEOUT 240 LABELS qualification_consensus_recovery)
endif()
