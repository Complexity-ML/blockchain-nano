add_test(bft_large_block_paris_network "/build/nano-bft-network-tests" "/build/nano" "/build/network/nano-p2p" "--large-block")
set_tests_properties(bft_large_block_paris_network PROPERTIES TIMEOUT 240 RUN_SERIAL TRUE WORKING_DIRECTORY "/build" LABELS "qualification_bft_large_block_local" ENVIRONMENT "NANO_TEST_EVM_FORK=Paris")
