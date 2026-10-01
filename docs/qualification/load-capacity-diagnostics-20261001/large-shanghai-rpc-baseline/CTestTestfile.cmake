add_test(bft_large_block_shanghai_network "/build/nano-bft-network-tests" "/build/nano" "/build/network/nano-p2p" "--large-block")
set_tests_properties(bft_large_block_shanghai_network PROPERTIES TIMEOUT 240.0 RUN_SERIAL TRUE WORKING_DIRECTORY "/build" ENVIRONMENT "NANO_TEST_EVM_FORK=Shanghai")
add_test(bft_rpc_cost_many_paris_network "/build/nano-bft-network-tests" "/build/nano" "/build/network/nano-p2p" "--rpc-cost-many")
set_tests_properties(bft_rpc_cost_many_paris_network PROPERTIES TIMEOUT 480.0 RUN_SERIAL TRUE WORKING_DIRECTORY "/build" ENVIRONMENT "NANO_TEST_EVM_FORK=Paris")
