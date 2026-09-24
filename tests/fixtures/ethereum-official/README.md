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

Shanghai fixtures (complete original files, unchanged contents):
- `shanghai-warm_coinbase_call_out_of_gas.json`: `fixtures/state_tests/for_shanghai/shanghai/eip3651_warm_coinbase/warm_coinbase/warm_coinbase_call_out_of_gas.json`
- `shanghai-warm_coinbase_gas_usage.json`: `fixtures/state_tests/for_shanghai/shanghai/eip3651_warm_coinbase/warm_coinbase/warm_coinbase_gas_usage.json`
- `shanghai-push0_contract_during_call_contexts.json`: `fixtures/state_tests/for_shanghai/shanghai/eip3855_push0/push0/push0_contract_during_call_contexts.json`
- `shanghai-push0_contracts.json`: `fixtures/state_tests/for_shanghai/shanghai/eip3855_push0/push0/push0_contracts.json`
- `shanghai-contract_creating_tx.json`: `fixtures/state_tests/for_shanghai/shanghai/eip3860_initcode/initcode/contract_creating_tx.json`
- `shanghai-create2_oversized_initcode_with_insufficient_balance.json`: `fixtures/state_tests/for_shanghai/shanghai/eip3860_initcode/initcode/create2_oversized_initcode_with_insufficient_balance.json`
- `shanghai-create_opcode_initcode.json`: `fixtures/state_tests/for_shanghai/shanghai/eip3860_initcode/initcode/create_opcode_initcode.json`
- `shanghai-gas_usage.json`: `fixtures/state_tests/for_shanghai/shanghai/eip3860_initcode/initcode/gas_usage.json`

Shanghai cases in the transition group (complete, renamed only):
- `shanghai-transition-blob_type_tx_pre_fork.json`: `fixtures/state_tests/for_shanghaitocancunattime15k/cancun/eip4844_blobs/blob_txs/blob_type_tx_pre_fork.json`
- `shanghai-transition-precompile_before_fork.json`: `fixtures/state_tests/for_shanghaitocancunattime15k/cancun/eip4844_blobs/point_evaluation_precompile/precompile_before_fork.json`
- `shanghai-transition-blobbasefee_before_fork.json`: `fixtures/state_tests/for_shanghaitocancunattime15k/cancun/eip7516_blobgasfee/blobgasfee_opcode/blobbasefee_before_fork.json`

Cancun initial rules and deep-call regression (complete unchanged files):
- `cancun-cancun--eip1153_tstore--basic_tload--basic_tload_after_store.json`: `fixtures/state_tests/for_cancun/cancun/eip1153_tstore/basic_tload/basic_tload_after_store.json`
- `cancun-cancun--eip1153_tstore--basic_tload--basic_tload_gasprice.json`: `fixtures/state_tests/for_cancun/cancun/eip1153_tstore/basic_tload/basic_tload_gasprice.json`
- `cancun-cancun--eip1153_tstore--basic_tload--basic_tload_other_after_tstore.json`: `fixtures/state_tests/for_cancun/cancun/eip1153_tstore/basic_tload/basic_tload_other_after_tstore.json`
- `cancun-cancun--eip1153_tstore--basic_tload--basic_tload_transaction_begin.json`: `fixtures/state_tests/for_cancun/cancun/eip1153_tstore/basic_tload/basic_tload_transaction_begin.json`
- `cancun-cancun--eip1153_tstore--basic_tload--basic_tload_works.json`: `fixtures/state_tests/for_cancun/cancun/eip1153_tstore/basic_tload/basic_tload_works.json`
- `cancun-cancun--eip1153_tstore--tload_calls--tload_calls.json`: `fixtures/state_tests/for_cancun/cancun/eip1153_tstore/tload_calls/tload_calls.json`
- `cancun-cancun--eip1153_tstore--tload_reentrancy--tload_reentrancy.json`: `fixtures/state_tests/for_cancun/cancun/eip1153_tstore/tload_reentrancy/tload_reentrancy.json`
- `cancun-cancun--eip1153_tstore--tstorage--gas_usage.json`: `fixtures/state_tests/for_cancun/cancun/eip1153_tstore/tstorage/gas_usage.json`
- `cancun-cancun--eip1153_tstore--tstorage--tload_after_sstore.json`: `fixtures/state_tests/for_cancun/cancun/eip1153_tstore/tstorage/tload_after_sstore.json`
- `cancun-cancun--eip1153_tstore--tstorage--tload_after_tstore.json`: `fixtures/state_tests/for_cancun/cancun/eip1153_tstore/tstorage/tload_after_tstore.json`
- `cancun-cancun--eip1153_tstore--tstorage--tload_after_tstore_is_zero.json`: `fixtures/state_tests/for_cancun/cancun/eip1153_tstore/tstorage/tload_after_tstore_is_zero.json`
- `cancun-cancun--eip1153_tstore--tstorage--transient_storage_unset_values.json`: `fixtures/state_tests/for_cancun/cancun/eip1153_tstore/tstorage/transient_storage_unset_values.json`
- `cancun-cancun--eip1153_tstore--tstorage_create_contexts--contract_creation.json`: `fixtures/state_tests/for_cancun/cancun/eip1153_tstore/tstorage_create_contexts/contract_creation.json`
- `cancun-cancun--eip1153_tstore--tstorage_create_contexts--tstore_rollback_on_failed_create.json`: `fixtures/state_tests/for_cancun/cancun/eip1153_tstore/tstorage_create_contexts/tstore_rollback_on_failed_create.json`
- `cancun-cancun--eip1153_tstore--tstorage_execution_contexts--subcall.json`: `fixtures/state_tests/for_cancun/cancun/eip1153_tstore/tstorage_execution_contexts/subcall.json`
- `cancun-cancun--eip1153_tstore--tstorage_execution_contexts--tstore_rollback_on_callcode_revert.json`: `fixtures/state_tests/for_cancun/cancun/eip1153_tstore/tstorage_execution_contexts/tstore_rollback_on_callcode_revert.json`
- `cancun-cancun--eip1153_tstore--tstorage_reentrancy_contexts--reentrant_call.json`: `fixtures/state_tests/for_cancun/cancun/eip1153_tstore/tstorage_reentrancy_contexts/reentrant_call.json`
- `cancun-cancun--eip1153_tstore--tstorage_selfdestruct--reentrant_selfdestructing_call.json`: `fixtures/state_tests/for_cancun/cancun/eip1153_tstore/tstorage_selfdestruct/reentrant_selfdestructing_call.json`
- `cancun-cancun--eip1153_tstore--tstore_reentrancy--tstore_reentrancy.json`: `fixtures/state_tests/for_cancun/cancun/eip1153_tstore/tstore_reentrancy/tstore_reentrancy.json`
- `cancun-cancun--eip5656_mcopy--mcopy--mcopy_on_empty_memory.json`: `fixtures/state_tests/for_cancun/cancun/eip5656_mcopy/mcopy/mcopy_on_empty_memory.json`
- `cancun-cancun--eip5656_mcopy--mcopy--mcopy_repeated.json`: `fixtures/state_tests/for_cancun/cancun/eip5656_mcopy/mcopy/mcopy_repeated.json`
- `cancun-cancun--eip5656_mcopy--mcopy--valid_mcopy_operations.json`: `fixtures/state_tests/for_cancun/cancun/eip5656_mcopy/mcopy/valid_mcopy_operations.json`
- `cancun-cancun--eip5656_mcopy--mcopy_contexts--no_memory_corruption_on_upper_call_stack_levels.json`: `fixtures/state_tests/for_cancun/cancun/eip5656_mcopy/mcopy_contexts/no_memory_corruption_on_upper_call_stack_levels.json`
- `cancun-cancun--eip5656_mcopy--mcopy_contexts--no_memory_corruption_on_upper_create_stack_levels.json`: `fixtures/state_tests/for_cancun/cancun/eip5656_mcopy/mcopy_contexts/no_memory_corruption_on_upper_create_stack_levels.json`
- `cancun-cancun--eip5656_mcopy--mcopy_memory_expansion--mcopy_huge_memory_expansion.json`: `fixtures/state_tests/for_cancun/cancun/eip5656_mcopy/mcopy_memory_expansion/mcopy_huge_memory_expansion.json`
- `cancun-cancun--eip5656_mcopy--mcopy_memory_expansion--mcopy_memory_expansion.json`: `fixtures/state_tests/for_cancun/cancun/eip5656_mcopy/mcopy_memory_expansion/mcopy_memory_expansion.json`
- `cancun-cancun--eip6780_selfdestruct--dynamic_create2_selfdestruct_collision--dynamic_create2_selfdestruct_collision.json`: `fixtures/state_tests/for_cancun/cancun/eip6780_selfdestruct/dynamic_create2_selfdestruct_collision/dynamic_create2_selfdestruct_collision.json`
- `cancun-cancun--eip6780_selfdestruct--journal_revert--selfdestruct_balance_transfer_reverted.json`: `fixtures/state_tests/for_cancun/cancun/eip6780_selfdestruct/journal_revert/selfdestruct_balance_transfer_reverted.json`
- `cancun-cancun--eip6780_selfdestruct--reentrancy_selfdestruct_revert--reentrancy_selfdestruct_revert.json`: `fixtures/state_tests/for_cancun/cancun/eip6780_selfdestruct/reentrancy_selfdestruct_revert/reentrancy_selfdestruct_revert.json`
- `cancun-cancun--eip6780_selfdestruct--selfdestruct--calling_from_new_contract_to_pre_existing_contract.json`: `fixtures/state_tests/for_cancun/cancun/eip6780_selfdestruct/selfdestruct/calling_from_new_contract_to_pre_existing_contract.json`
- `cancun-cancun--eip6780_selfdestruct--selfdestruct--calling_from_pre_existing_contract_to_new_contract.json`: `fixtures/state_tests/for_cancun/cancun/eip6780_selfdestruct/selfdestruct/calling_from_pre_existing_contract_to_new_contract.json`
- `cancun-cancun--eip6780_selfdestruct--selfdestruct--create_and_destroy_multiple_contracts_same_tx.json`: `fixtures/state_tests/for_cancun/cancun/eip6780_selfdestruct/selfdestruct/create_and_destroy_multiple_contracts_same_tx.json`
- `cancun-cancun--eip6780_selfdestruct--selfdestruct--create_selfdestruct_same_tx.json`: `fixtures/state_tests/for_cancun/cancun/eip6780_selfdestruct/selfdestruct/create_selfdestruct_same_tx.json`
- `cancun-cancun--eip6780_selfdestruct--selfdestruct--create_selfdestruct_same_tx_increased_nonce.json`: `fixtures/state_tests/for_cancun/cancun/eip6780_selfdestruct/selfdestruct/create_selfdestruct_same_tx_increased_nonce.json`
- `cancun-cancun--eip6780_selfdestruct--selfdestruct--parent_creates_child_selfdestruct_one.json`: `fixtures/state_tests/for_cancun/cancun/eip6780_selfdestruct/selfdestruct/parent_creates_child_selfdestruct_one.json`
- `cancun-cancun--eip6780_selfdestruct--selfdestruct--recursive_contract_creation_and_selfdestruct.json`: `fixtures/state_tests/for_cancun/cancun/eip6780_selfdestruct/selfdestruct/recursive_contract_creation_and_selfdestruct.json`
- `cancun-cancun--eip6780_selfdestruct--selfdestruct--self_destructing_initcode.json`: `fixtures/state_tests/for_cancun/cancun/eip6780_selfdestruct/selfdestruct/self_destructing_initcode.json`
- `cancun-cancun--eip6780_selfdestruct--selfdestruct--self_destructing_initcode_create_tx.json`: `fixtures/state_tests/for_cancun/cancun/eip6780_selfdestruct/selfdestruct/self_destructing_initcode_create_tx.json`
- `cancun-cancun--eip6780_selfdestruct--selfdestruct--selfdestruct_pre_existing.json`: `fixtures/state_tests/for_cancun/cancun/eip6780_selfdestruct/selfdestruct/selfdestruct_pre_existing.json`
- `cancun-cancun--eip6780_selfdestruct--selfdestruct_revert--selfdestruct_created_in_same_tx_with_revert.json`: `fixtures/state_tests/for_cancun/cancun/eip6780_selfdestruct/selfdestruct_revert/selfdestruct_created_in_same_tx_with_revert.json`
- `cancun-cancun--eip6780_selfdestruct--selfdestruct_revert--selfdestruct_not_created_in_same_tx_with_revert.json`: `fixtures/state_tests/for_cancun/cancun/eip6780_selfdestruct/selfdestruct_revert/selfdestruct_not_created_in_same_tx_with_revert.json`
- `cancun-ported_static--stCallCreateCallCodeTest--call1024_balance_too_low--call1024_balance_too_low.json`: `fixtures/state_tests/for_cancun/ported_static/stCallCreateCallCodeTest/call1024_balance_too_low/call1024_balance_too_low.json`

Cancun blob execution and validation (complete files, no case filters):
- `cancun-cancun--eip4844_blobs--blob_txs--blob_gas_subtraction_tx.json`: `fixtures/state_tests/for_cancun/cancun/eip4844_blobs/blob_txs/blob_gas_subtraction_tx.json`
- `cancun-cancun--eip4844_blobs--blob_txs--blob_tx_attribute_calldata_opcodes.json`: `fixtures/state_tests/for_cancun/cancun/eip4844_blobs/blob_txs/blob_tx_attribute_calldata_opcodes.json`
- `cancun-cancun--eip4844_blobs--blob_txs--blob_tx_attribute_gasprice_opcode.json`: `fixtures/state_tests/for_cancun/cancun/eip4844_blobs/blob_txs/blob_tx_attribute_gasprice_opcode.json`
- `cancun-cancun--eip4844_blobs--blob_txs--blob_tx_attribute_opcodes.json`: `fixtures/state_tests/for_cancun/cancun/eip4844_blobs/blob_txs/blob_tx_attribute_opcodes.json`
- `cancun-cancun--eip4844_blobs--blob_txs--blob_tx_attribute_value_opcode.json`: `fixtures/state_tests/for_cancun/cancun/eip4844_blobs/blob_txs/blob_tx_attribute_value_opcode.json`
- `cancun-cancun--eip4844_blobs--blob_txs--insufficient_balance_blob_tx.json`: `fixtures/state_tests/for_cancun/cancun/eip4844_blobs/blob_txs/insufficient_balance_blob_tx.json`
- `cancun-cancun--eip4844_blobs--blob_txs--invalid_blob_hash_versioning_single_tx.json`: `fixtures/state_tests/for_cancun/cancun/eip4844_blobs/blob_txs/invalid_blob_hash_versioning_single_tx.json`
- `cancun-cancun--eip4844_blobs--blob_txs--invalid_normal_gas.json`: `fixtures/state_tests/for_cancun/cancun/eip4844_blobs/blob_txs/invalid_normal_gas.json`
- `cancun-cancun--eip4844_blobs--blob_txs--invalid_tx_blob_count.json`: `fixtures/state_tests/for_cancun/cancun/eip4844_blobs/blob_txs/invalid_tx_blob_count.json`
- `cancun-cancun--eip4844_blobs--blob_txs--invalid_tx_max_fee_per_blob_gas_state.json`: `fixtures/state_tests/for_cancun/cancun/eip4844_blobs/blob_txs/invalid_tx_max_fee_per_blob_gas_state.json`
- `cancun-cancun--eip4844_blobs--blob_txs--sufficient_balance_blob_tx.json`: `fixtures/state_tests/for_cancun/cancun/eip4844_blobs/blob_txs/sufficient_balance_blob_tx.json`
- `cancun-cancun--eip4844_blobs--blobhash_opcode--blobhash_gas_cost.json`: `fixtures/state_tests/for_cancun/cancun/eip4844_blobs/blobhash_opcode/blobhash_gas_cost.json`
- `cancun-cancun--eip4844_blobs--blobhash_opcode_contexts--blobhash_opcode_contexts.json`: `fixtures/state_tests/for_cancun/cancun/eip4844_blobs/blobhash_opcode_contexts/blobhash_opcode_contexts.json`
- `cancun-cancun--eip4844_blobs--blobhash_opcode_contexts--blobhash_opcode_contexts_tx_types.json`: `fixtures/state_tests/for_cancun/cancun/eip4844_blobs/blobhash_opcode_contexts/blobhash_opcode_contexts_tx_types.json`
- `cancun-cancun--eip7516_blobgasfee--blobgasfee_opcode--blobbasefee_out_of_gas.json`: `fixtures/state_tests/for_cancun/cancun/eip7516_blobgasfee/blobgasfee_opcode/blobbasefee_out_of_gas.json`
- `cancun-cancun--eip7516_blobgasfee--blobgasfee_opcode--blobbasefee_stack_overflow.json`: `fixtures/state_tests/for_cancun/cancun/eip7516_blobgasfee/blobgasfee_opcode/blobbasefee_stack_overflow.json`

Cancun KZG proof and gas fixtures (complete upstream files):
- `cancun-cancun--eip4844_blobs--point_evaluation_precompile--call_opcode_types.json`: `fixtures/state_tests/for_cancun/cancun/eip4844_blobs/point_evaluation_precompile/call_opcode_types.json`
- `cancun-cancun--eip4844_blobs--point_evaluation_precompile--external_vectors.json`: `fixtures/state_tests/for_cancun/cancun/eip4844_blobs/point_evaluation_precompile/external_vectors.json`
- `cancun-cancun--eip4844_blobs--point_evaluation_precompile--invalid_inputs.json`: `fixtures/state_tests/for_cancun/cancun/eip4844_blobs/point_evaluation_precompile/invalid_inputs.json`
- `cancun-cancun--eip4844_blobs--point_evaluation_precompile--tx_entry_point.json`: `fixtures/state_tests/for_cancun/cancun/eip4844_blobs/point_evaluation_precompile/tx_entry_point.json`
- `cancun-cancun--eip4844_blobs--point_evaluation_precompile--valid_inputs.json`: `fixtures/state_tests/for_cancun/cancun/eip4844_blobs/point_evaluation_precompile/valid_inputs.json`
- `cancun-cancun--eip4844_blobs--point_evaluation_precompile_gas--point_evaluation_precompile_gas_usage.json`: `fixtures/state_tests/for_cancun/cancun/eip4844_blobs/point_evaluation_precompile_gas/point_evaluation_precompile_gas_usage.json`

Final Cancun discrepancy regressions: 32 bundled complete, unchanged files
from `fixtures/state_tests/for_cancun/ported_static/`, covering static CALLCODE,
ECRECOVER, empty MODEXP with large declared exponent, nonce maximum and memory
expansion. Filenames begin `cancun-` and replace each original path separator
under `for_cancun/` with `--`. Individual SHA-256 values are in the manifest.

Eight additional large random-test files (about 196 MB total) remain in the
pinned official archive instead of being duplicated here. Their original paths
and SHA-256 values are recorded in `externalCancunRegressionFiles`. All 40 files
(246 cases) are exercised by the full `for_cancun` run; the bundled CTest subset
does not replace that run. No official file has been edited or case-filtered.

Prague calldata-floor regressions: three complete unchanged files from
`fixtures/state_tests/for_prague/prague/eip7623_increase_calldata_cost/transaction_validity/`,
covering transaction types 0, 1, 2 and 3 (372 cases). The type-4 file and
mixed-type execution/refund files remain in the full diagnostic corpus; their
unsupported type-4 cases remain explicit failures. Naming uses the same
`prague-` prefix and `--` path substitution convention.

BLS12-381 Prague regressions: 52 complete, unchanged EIP-2537 files use the
`prague-` / `--` naming convention. The 14 MB G2 MSM valid-vector file remains
in the pinned archive, recorded in `externalPragueBlsRegressionFiles`. The full
BLS diagnostic executes all 53 files and 978 cases, including this large file
and the two pairing transactions larger than 1 MiB.

Authorization regressions: 72 complete EIP-7702 files and seven EIP-7623
files are now bundled, including mixed-type cases previously failing.
The large `set_code_txs/many_delegations.json` and
`transaction_validity/transaction_validity_type_4.json` files remain in the
pinned archive, with SHA-256 in `externalPragueAuthorizationRegressionFiles`.
The full Prague-specific diagnostic includes every case in these files.

## Régressions Osaka

Les fichiers `osaka-*.json` sont les fichiers complets inchangés du groupe
`fixtures/state_tests/for_osaka/osaka`, avec `/` remplacé par `--` après le
préfixe. Empreintes dans `bundledRegressionFiles` ; les fichiers dépassant
10 Mo restent référencés dans `externalOsakaRegressionFiles` du manifeste.
Le corpus complet compte 13 739 concordances, sans prouver la conformité des
blocs complets ni une activation réseau.
