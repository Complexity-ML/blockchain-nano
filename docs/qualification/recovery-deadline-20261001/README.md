# Strict recovery observation deadlines — 1 October 2026

Three new deterministic fake-clock cases test an exact inclusive limit, a read completed one millisecond too late, and delayed reconnect logging. Before correction two cases failed with missing rejection; the exact-boundary case passed. Before-test temporary variant was not separately committed; its log is retained.

At 443b19a, the real clock is monotonic (node:perf_hooks). The first budget anchors immediately after verified network restoration, before asynchronous logging. Recovery is accepted only when the completed sample is at or before the deadline. Sleeps are clipped to remaining budget; the separately bounded late observer retains the original failure and history. Events report observed recovery elapsed and limit values. No consensus, runtime image or 180-second budget changed.

All 33 partition/cycle/late-observation tests pass locally, including the three new boundary cases. This qualifies the harness logic with injected clocks, not WAN recovery. Attempt E raw pass remains recorded; its strict deadline scope is unqualified pending a new observation with the corrected harness. Existing images can be reused.
