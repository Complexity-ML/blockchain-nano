# Observed candidate dependencies

Read-only probes of existing 74784c4 images ARM64 99810019c0e155fd4ed1a35cc6c9dcc7935457d3e94bc1636f94cddded074729 and AMD64 f507006eae149de4770bc1ada07547be78439806d2028361422b2e574cad2655, network none/capabilities dropped/no chain mounts. dpkg-query and ldd for both executables; zero unresolved libraries. Same observed package versions on both architectures. No new images or build.

Tracked source inventory at ee565c1 includes file-set digests and notice hashes: mcl 292 files, ckzg/blst 210, secp256k1 178, bip39 3 and JSON 3.12.0 header. Counts and digests are source integrity metadata, not proof of upstream signature, audit or license compliance. Runtime package inventory is not a complete transitive SBOM or a frozen reconstruction recipe. Packaging all upstream notices and reproducibility remain open.
