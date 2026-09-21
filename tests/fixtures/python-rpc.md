# Original Python read RPC responses

`python-rpc.json` captures 33 responses from the unmodified `NanoPyNode` and
`NanoPyRPC`, instantiated with persistence disabled and no server started.
The generator seeds one known account through the source StateDB methods
(balance, nonce, code and storage); the fixture contains equivalent native
genesis allocations. It also queries an unknown account and absent hashes.

The native RPC suite compares each entire JSON-RPC response, including ID,
quantity/data encoding, booleans, empty collections and null results. Cases
cover network/chain identifiers, inactive mining, coinbase, accounts, empty
pool, account reads, missing blocks/transactions/receipts, transaction counts
and Keccak. The native state root is unchanged after the reads.

Regenerate from the Nano repository root:

```sh
PYTHONDONTWRITEBYTECODE=1 python scripts/generate-rpc-fixtures.py /path/to/blockchain-python
```

Use an environment with the original Python dependencies. The fixture records
SHA-256 hashes for the node, HTTP RPC and pool modules. The source repository
is imported without bytecode writes and is never edited. These cases do not
establish historical block-hash compatibility, fee-policy parity, malformed
request parity or coverage of all registered RPC methods.
