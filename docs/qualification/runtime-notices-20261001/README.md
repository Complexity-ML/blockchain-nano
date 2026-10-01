# Prepared runtime notice overlays

Read-only probes of the existing 74784c4 ARM64 candidate
`sha256:99810019c0e155fd4ed1a35cc6c9dcc7935457d3e94bc1636f94cddded074729`
and AMD64 candidate
`sha256:f507006eae149de4770bc1ada07547be78439806d2028361422b2e574cad2655`.
Network disabled, capabilities dropped, no chain mounts. A temporary tmpfs
combines the seven existing notices with four prepared supplementary files.
All eleven hashes match. Overwriting the JSON license is then refused.

The prepared CMake installer and both Docker assembly paths ship and verify
these notices. No new image was built in this probe: actual installation and
image assembly remain pending. cpp-libp2p source commit in the existing
builder was independently checked as c03bf7c44c4fd30a390f3ef93bd1eab482e686a6.
JSON copyright entries preserve all five unique SPDX copyright lines in the
bundled 3.12.0 header; its official tagged MIT license is included.

This is direct dependency notice packaging, not a complete transitive SBOM,
license-compliance audit, reproducible release or production qualification.

Native CMake configure/install succeeded into a temporary prefix without
compilation. All eleven installed notice hashes match; installed nano --help
succeeds and its binary hash equals the pre-existing qualified native binary
0925ae6cb47b77c06bcf25dc38eaf6ba6b973e3f1c631d04d3b8e28d854ed9d7.

ARM64 assembly now succeeded, one image, no apt or compilation:
sha256:7b3db08d4076cca560a7d599a9601cc8ce02f8614274bf5a72679969e45568bf.
Actual image rechecked read-only, no network, uid/gid 10001, all eleven notices,
help and unchanged nano/P2P hashes. AMD64 assembly and the complete Dockerfile
build remain pending. This delivery-only image does not replace the runtime
images used in the prior WAN qualification.

The source notice checker accepts the exact inventory and rejects duplicate
records, modified license bytes and missing JSON copyrights even after their
file hash is updated. First manual control fixture omitted json.hpp and was
refused with ENOENT; the corrected fixture copied the actual header. No
implementation bypass or test relaxation was needed.

AMD64 assembly now succeeded after Git pull of 930bf00, using the existing
74784c4 binaries, without apt or compilation:
sha256:6eb8175cd8507d260b51501e64f16aff5f23456e0b68b117f4c1c10314874f3f.
Actual image rechecked with network disabled/read-only, uid/gid 10001, all
eleven notices, CLI help and unchanged nano/P2P hashes. Both architectures
are qualified for this delivery-only correction, not for a new WAN run.

GitHub run 36892286298 at 930bf00: operations completed/success, 232 tests
passed with zero skips/failures (5926.333331 ms); the new notice report confirms
eleven hashes and five copyright entries. Core was still in progress at the
archived snapshot; no terminal/full-run success is claimed here.
