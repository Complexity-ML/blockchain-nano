# Add inside the existing NANO_RECOVERY_NODEJS_EXECUTABLE condition.
# The existing pure lifecycle and guarded-driver registrations remain in place.
add_test(NAME consensus_multihost_fresh_lifecycle_runtime_protocol COMMAND
 ${NANO_RECOVERY_NODEJS_EXECUTABLE} --test
 ${CMAKE_CURRENT_SOURCE_DIR}/tests/multihost/lifecycle-runtime.test.mjs)
set_tests_properties(consensus_multihost_fresh_lifecycle_runtime_protocol PROPERTIES
 TIMEOUT 30 LABELS "qualification_consensus_lifecycle")
