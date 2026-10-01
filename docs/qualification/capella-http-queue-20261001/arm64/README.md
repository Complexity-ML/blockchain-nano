# ARM64 Capella HTTP dispatch qualification

Source 18ba7ef4d2d6075f721340dfb97b579060272ecd; existing builder/volume, network none, 2 CPUs / 3 GiB. Controlled real admission-mutex HTTP test passes (8.15 s), public identity and Beacon ready while private RPC pending, checkpoint unchanged. RPC queue expiry regression passes (1.04 s). Same workers/capacity/deadlines. No WAN recovery claim.

nano SHA256: 4df65fac9355b3118201f0d9ed257ab3a14e7b808848bd955dec507f84b3fde6
nano-p2p SHA256: ae567df0787a6d64e72a126b4d8b00bbc8ed761c4d7fe308d42c61a878ee2d75

P2P compiled at 5bd6f95 reused: network and third_party sources unchanged, standalone network CMake target excludes core and simulation_queue; checksum identical to previously qualified candidate. Effective source inputs match 18ba7ef. Runtime core rebuilt; previous core/image provenance not reused.
