# Cas officiels conservés pour les régressions

Source : release `tests@v20.0.0` de https://github.com/ethereum/execution-specs,
archive vérifiée selon `../ethereum-official.json`. Fichiers JSON complets et non
modifiés, distribués sous CC0 (LICENSE.md).

Chemins dans l'archive :

- `call-insufficient-balance.json` : `fixtures/state_tests/for_berlin/berlin/eip2929_gas_cost_increases/call/call_insufficient_balance.json`
- `account_storage_warm_cold_state.json` : `fixtures/state_tests/for_berlin/berlin/eip2930_access_list/acl/account_storage_warm_cold_state.json`
- `repeated_address_acl.json` : `fixtures/state_tests/for_berlin/berlin/eip2930_access_list/acl/repeated_address_acl.json`

Ces trois fichiers contiennent cinq cas. Leur sélection protège les correctifs
acquis ; elle ne représente pas toute la suite ni une certification. Le bilan du
groupe Berlin complet figure dans `../ethereum-official.json` et reste incomplet.

Creation regressions, complete unchanged files from the same archive:
- `fixtures/state_tests/for_berlin/frontier/create/create_deposit_oog/create_deposit_oog.json`
- `fixtures/state_tests/for_berlin/frontier/create/create_one_byte/create_one_byte.json`
- `fixtures/state_tests/for_berlin/constantinople/eip1052_extcodehash/extcodehash/extcodehash_in_init_code.json`
- `fixtures/state_tests/for_berlin/constantinople/eip1052_extcodehash/extcodehash/extcodehash_self_in_init.json`

MODEXP and copy regressions (all post-state variants target Berlin):
- `fixtures/state_tests/for_berlin/byzantium/eip198_modexp_precompile/modexp/modexp.json`
- `fixtures/state_tests/for_berlin/osaka/eip7883_modexp_gas_increase/modexp_thresholds/modexp_variable_gas_cost.json`
- `fixtures/state_tests/for_berlin/homestead/identity_precompile/identity/identity_return_buffer_modify.json`
- `fixtures/state_tests/for_berlin/constantinople/eip1052_extcodehash/extcodehash/codecopy_zero_in_create2.json`

Deferred destruction and storage-collision regressions:
- `fixtures/state_tests/for_berlin/constantinople/eip1052_extcodehash/extcodehash/extcodehash_subcall_selfdestruct.json`
- `fixtures/state_tests/for_berlin/frontier/create/create_suicide_during_init/create_suicide_during_transaction_create.json`
- `fixtures/state_tests/for_berlin/paris/eip7610_create_collision/initcollision/init_collision_create_tx.json`

- `fixtures/state_tests/for_berlin/frontier/opcodes/blockhash_state_test_recency/blockhash_zero_in_window_control.json`

Validation fixtures (all complete, no case filters):
- `fixtures/state_tests/for_berlin/berlin/eip2930_access_list/tx_intrinsic_gas/tx_intrinsic_gas.json`
- `fixtures/state_tests/for_berlin/berlin/eip2930_access_list/acl/transaction_intrinsic_gas_cost.json`
- `fixtures/state_tests/for_berlin/london/eip1559_fee_market_change/tx_type/invalid_chain_id.json`
- `fixtures/state_tests/for_berlin/london/eip1559_fee_market_change/tx_type/eip1559_tx_validity.json`
- `fixtures/state_tests/for_berlin/frontier/validation/transaction/sender_balance_insufficient_state_test.json`
- `fixtures/state_tests/for_berlin/frontier/validation/transaction/bad_v_r_s.json`

Paris fixtures (renamed only, unchanged contents):
- `paris-eip1559_tx_validity.json`: `fixtures/state_tests/for_paris/london/eip1559_fee_market_change/tx_type/eip1559_tx_validity.json`
- `paris-invalid_chain_id.json`: `fixtures/state_tests/for_paris/london/eip1559_fee_market_change/tx_type/invalid_chain_id.json`
- `paris-coverage.json`: `fixtures/state_tests/for_paris/homestead/coverage/coverage/coverage.json`
