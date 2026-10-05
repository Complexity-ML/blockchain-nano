# Prepared targets for included votes and FFG

Parent checkout: d32df0b598af8df3bed924adfb237a601d4d4640; parent C++ producer: b2c952b6ce1c99923fb171902e8c88bd71d994ee.

Adds typed contextual block/tick APIs. Used frames must be complete and match the exact checkpoint, admitted source and full profile. Missing targets keep the original replay budget. Full signed state transition precedes embedded vote handling and private Store publication. No layouts or snapshot formats changed.

ARM64 initial: terminal 0/no OOM, 106 checks plus configured-runtime and scheduler. ARM64 test extension: unchanged implementation, terminal 0/no OOM, 116 checks. Adds two signed BLS blocks containing valid previous-slot attestations after long gaps; complete Store equals ordinary full-gap oracle. Legacy budget remains enforced; incomplete frames and invalid outer signature fail. Clock promotion fixture is explicitly synthetic unrealized FFG metadata, not a quorum qualification.

Controllers retained with exact commands and source/program hashes. Existing image and volume reused. Dependency consumers conservatively rebuilt when prior per-unit objects/exits absent; extension log preserves missing prior duty-exit diagnostic and its successful recompilation. No identical runtime/scheduler rerun for test-only extension.

Not yet integrated into Reactor incoming/tick preflight caches. No network, cold activation, full recovery, committee rotation, validator lifecycle or production readiness claim. AMD64 pending.
