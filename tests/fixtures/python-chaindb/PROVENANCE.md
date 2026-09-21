Generated from unmodified NanoPy source commit `77f2664390db135171608e7bbb613f4ea6468312`.

`legacy-genesis` captures the unmodified node genesis construction and its stale cached hash. `genesis-only` explicitly refreshes that cache by reconstructing the same header, and must import successfully. `legacy-block` captures the Python engine unchanged: it retains the parent base fee and uses keccak(empty bytes) for the empty uncle hash. Native strict replay must report the divergence rather than silently replace the block hash.
