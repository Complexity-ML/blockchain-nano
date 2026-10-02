# Altair boundary: transport write failure isolated

Original consensus_altair_libp2p test still fails (2.11 s) with its original
120-second limit. Only the qualification probe and test diagnostic stderr are
instrumented. The c104 core, daemon, headers, fixtures and static inputs remain
read-only. This diagnostic composition is not a production candidate.
The full 16 777 370-byte frame returns Stream: reset by remote peer during
write-frame. Therefore its ACK-read step was not entered. HighWater remains
0x1, checkpoint unchanged, clock healthy. Earlier invalid/unhandshaken frames
also reset as expected; they do not explain this compatible full-bound failure.
The exact remote guard or transport fault is still unidentified. No timeout,
accepted payload size or authentication rule has been altered.

Source, program, core link-input and probe dependency hashes are equal before/
after; post manifest exit 0. Terminal XML and transport events are retained.
The earlier setup attempt exited 127 at python3: command not found before any
compile or test; its reports and original control are retained. The final
control generates compiler/link commands on the host and runs only existing
compiler/sha256sum tools in the unchanged image. No dependency is installed.

Controls, captured original network compiler/link metadata and exact patch are
archived. Logs preserve original decompressed bytes. Scratch/ctest directories
and binary/object products are excluded from Git; exact program manifests are
retained. Next diagnostic must distinguish daemon pre-admission guards and
transport behavior. This result does not qualify the full Altair scenario.
