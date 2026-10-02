# Clean AMD64 compilation and targeted RPC diagnostic

Prepared after the explicitly authorized full Docker reset. The dependency image build is live at VPS PID 1204418, source candidate `9e9ebed3833a24f75ce7b29b9a80dcce341dfc3a`; log `/home/ubuntu/nano-clean-build-amd64-20261002/dependencies-build.log`. No old image or build volume is reused after deletion.

`run.py` waits for that actual process, requires its resulting image, fixes the digest, verifies the clean source revision, creates a fresh named build volume, and compiles nano-p2p, nano and nano-bft-network-tests under Docker (2 CPU / 3 GiB). It selects the unchanged Paris WebSocket RPC case from the CTest registry, checks its command and original 480-second timeout, and executes only that case with existing opt-in timing instrumentation. Sources and three program hashes must stay stable. Scratch and reports are outside ephemeral containers. The case remains bounded to its original 16-client scenario.

Python syntax checked. Controller execution and compilation/diagnostic results not yet acquired. This is diagnostic preparation, not a production or full-regression qualification; prior RPC failure and interrupted 393-case campaign remain retained. No public deployment or WAN restore is launched.
