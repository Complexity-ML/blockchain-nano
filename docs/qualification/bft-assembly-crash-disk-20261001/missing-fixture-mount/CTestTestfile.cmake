add_test(bft_assembly_crash_paris /build/nano-bft-large-envelope-tests)
set_tests_properties(bft_assembly_crash_paris PROPERTIES ENVIRONMENT "NANO_TEST_EVM_FORK=Paris;NANO_TEST_BFT_ASSEMBLY_CRASH=1" TIMEOUT 240 RUN_SERIAL TRUE)
add_test(bft_assembly_crash_shanghai /build/nano-bft-large-envelope-tests)
set_tests_properties(bft_assembly_crash_shanghai PROPERTIES ENVIRONMENT "NANO_TEST_EVM_FORK=Shanghai;NANO_TEST_BFT_ASSEMBLY_CRASH=1" TIMEOUT 240 RUN_SERIAL TRUE)
