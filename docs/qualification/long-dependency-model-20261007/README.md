# Bounded long dependency transport model

Actual V36 limits: quantum8, ACK1024, dependency256, export cache256/16MiB, catalogue page8/max1024. Original driver91425 ended0 (d5af19); compile/link and each case exit0 retained.

| Ancestry blocks | Relay turns | Local export requests | Modeled admissions |
| --- | --- | --- | --- |
| 513 | 68 | 1281 | 513 |
| 935 | 123 | 2471 | 935 |
| 2049 | 293 | 11265 | 2049 |

The fixture asserts parent-first sends, bounded admissions, exact complete acknowledgement and no false selected-head success. It uses actual relay headers with synthetic SSZ and mocked authenticated exports/receiver ACKs. Empty historical catalogue and a fixed single ancestry omit multi-peer, concurrent/moving forks, rejection/restart and real state transition. Counts are not WAN duration or global cause attribution. Native fixture compilation ran concurrently with V36: no resource-isolation claim. No active node/configuration/code/image was changed.

Initial compile/link failure (`boost_system` missing), repaired link command, exploratory quantum32/ACK4096 cases and their historical summary are preserved. Exploratory cases are not V36 settings; additional exploratory exits were captured only in aggregate. `actual-summary.json` contains the V36-parameter results; this is a transport model, not production qualification.
