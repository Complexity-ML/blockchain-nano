
if(TARGET nano-test-sync-fault AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
 add_test(NAME consensus_capella_cold_memo_sigkill_recovery COMMAND ${CMAKE_COMMAND} -E env NANO_TEST_FRAGMENT_CHECKPOINT=1 NANO_TEST_COLD_REACTOR=1 NANO_TEST_RUNTIME_REFERENCE_MEMO=1 NANO_TEST_COLD_MEMO_SYNC=1 NANO_TEST_COLD_MEMO_SIGKILL=1 $<TARGET_FILE:nano-capella-transition-tests> --prepared-block)
 set_tests_properties(consensus_capella_cold_memo_sigkill_recovery PROPERTIES TIMEOUT 240 LABELS qualification_consensus_recovery)
endif()
