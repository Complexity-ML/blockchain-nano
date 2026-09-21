# Python VM execution reference

`python-evm.json` contains 451 executions captured from the unmodified source
`nanopy.vm.evm.NanoPyEVM`: 16 cases for each of 25 arithmetic, comparison and
bitwise instructions, plus 24 memory, calldata/code copy, storage, log, stack,
jump and error cases, plus 15 nested CALL/STATICCALL/DELEGATECALL executions and 12 block-context
executions (direct and each call type). Inputs include zero, word and sign boundaries, all-one
words, and reproducible random 256-bit values. Exponents are bounded.

Regenerate from the Nano repository root using a Python environment with the
original project's dependencies (including py-ecc):

```sh
PYTHONDONTWRITEBYTECODE=1 python scripts/generate-evm-fixtures.py /path/to/blockchain-python
```

The fixture records SHA-256 hashes of the reference VM, opcode and gas tables.
The generator imports the original modules without changing them or writing
bytecode into their repository. Each instruction result is returned through
MSTORE/RETURN. The core suite compares success, return bytes, gas used and gas
refunds in a fresh native state for every case, as well as logs and the final
storage slot for the stateful cases.

Intentional differences are asserted separately:

- the Python VM reports a
successful RETURN using 30 gas with only 10 available when memory expands.
Nano rejects the expansion, consumes the available 10 gas and preserves state.
- For CALL, STATICCALL and DELEGATECALL returning a REVERT payload, the Python
  VM leaves the caller output-memory region unchanged. Nano copies the revert
  payload to that region. Gas and storage are still compared to the reference.

This fixture retains the actual Python results rather than masking differences.

These vectors establish parity for the sampled instructions, not full Ethereum
conformance or exhaustive parity of nested calls, persistent state and historical
consensus. Nested cases cover frame address/caller/origin/value/calldata length,
storage isolation, logs, successful return and revert. The block-context cases
compare COINBASE, TIMESTAMP, NUMBER, PREVRANDAO, GASLIMIT, CHAINID, BASEFEE and
GASPRICE together, including zero and large integer values, in direct and nested
execution. BLOCKHASH has separate native historical-chain tests because the
Python implementation is a placeholder.
