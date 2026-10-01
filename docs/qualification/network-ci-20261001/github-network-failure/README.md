# Fresh GitHub transport build: retained terminal failure

Run 36894479093, source 85a4a7c. Dependencies and all transport targets compile.
Twenty tests pass, altair_core_dispatch fails at "concurrent ticket admission";
21 registered/actual cases, no skipped case, CTest exit 8, total 1.09 seconds.
Run completed/failure; core succeeds (9 tests), operations succeeds (235).
The nonblocking io.poll test assumption is being replaced in an isolated
checkout by waiting for exactly one handler, preserving admission/shutdown
assertions. No dispatcher runtime change. Full fresh-job success remains open.
The original CI script wrote binary/package hashes only after CTest; those
artifacts are therefore absent for this failure. The prepared recipe moves
provenance before CTest so future failures retain it. No hashes invented here.
