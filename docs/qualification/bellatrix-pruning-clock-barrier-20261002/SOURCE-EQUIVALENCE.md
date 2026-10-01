# Source comparison for corrective qualification

The clean c10426b2 baseline and clean 1ac57e25 corrective checkout were compared by SHA256 across tracked src, include, network, third_party, tests and CMakeLists.txt. 9,266 files are unchanged; nine paths differ, listed with both hashes in source-equivalence-c104-1ac.json. The EVM, RPC, types and BLS implementation files, and all network/third_party files, are byte-identical.

Changes consist of Bellatrix failure/clock diagnostics, the RPC observation-window controls and the exit-inclusion synchronization barrier. This scopes the pending 19-case AMD64 qualification and later review. It does not prove binary equivalence, compiler-setting equivalence or a full candidate regression. The original AMD64 Altair ingress failure and Bellatrix partition timeout remain open and are outside that selection. Historical full-campaign results and all production requirements are preserved.
