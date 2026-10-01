# ARM64 Docker historical-export qualification

Existing dependency image and nano-linux-sync-build volume reused, source read-only, network disabled, 2 CPUs / 3 GiB, nofile 65536. Four relevant CTests pass: historical_export (32.92 s), signed_transition (10.05 s), block_delivery, head_catalog. No real daemon catch-up or WAN recovery qualified yet. No image packaged or source marker advanced.

nano SHA256: 3f9c6fed1be76fea8ace26c8c8e4f500aba46d352a8a29482963d38fa9c414d0
nano-p2p SHA256: ae567df0787a6d64e72a126b4d8b00bbc8ed761c4d7fe308d42c61a878ee2d75
