# Append to the reviewed top-level test target section. No core source changes.
add_executable(nano-capella-lifecycle-submit tests/multihost/lifecycle_submit.cpp)
target_link_libraries(nano-capella-lifecycle-submit PRIVATE nano_core)
add_test(NAME consensus_fresh_lifecycle_managed_submission COMMAND nano-capella-lifecycle-submit --self-test)
set_tests_properties(consensus_fresh_lifecycle_managed_submission PROPERTIES TIMEOUT 30 LABELS qualification_consensus_lifecycle)
