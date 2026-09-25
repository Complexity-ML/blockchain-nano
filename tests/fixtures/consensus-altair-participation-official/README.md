# Altair participation flag updates

All 12 minimal epoch_processing/participation_flag_updates/pyspec_tests cases
at ethereum/consensus-spec-tests commit
bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908. Original Snappy and decoded SSZ
pre/post files are retained with hashes and upstream paths in manifest.json.
This is an isolated rotation component, not an idempotent operation. Epoch
orchestration must ensure exactly-once application.
