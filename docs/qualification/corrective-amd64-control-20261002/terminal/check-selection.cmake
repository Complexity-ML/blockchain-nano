file(READ /reports/registry.json registry)
string(JSON count LENGTH "${registry}" tests)
if(NOT count EQUAL 19)
 message(FATAL_ERROR "Expected exactly 19 corrective scenarios, got ${count}")
endif()
set(expected core rpc_load_window consensus_bellatrix_clock_coalescing
 consensus_bellatrix_voluntary_exit consensus_bellatrix_voluntary_exit_pruning
 consensus_bellatrix_voluntary_exit_network consensus_bellatrix_voluntary_exit_reorg
 bft_rpc_cost_paris_network bft_rpc_cost_sustained_paris_network
 bft_rpc_cost_shanghai_network bft_rpc_cost_sustained_shanghai_network
 rpc_cost_paris rpc_cost_success_paris rpc_cost_nested_1_paris rpc_cost_nested_4_paris
 rpc_cost_shanghai rpc_cost_success_shanghai rpc_cost_nested_1_shanghai rpc_cost_nested_4_shanghai)
foreach(i RANGE 0 18)
 string(JSON name GET "${registry}" tests ${i} name)
 list(APPEND names "${name}")
endforeach()
list(SORT expected)
list(SORT names)
if(NOT "${names}" STREQUAL "${expected}")
 message(FATAL_ERROR "Corrective selection contains missing, duplicate or foreign scenarios")
endif()
