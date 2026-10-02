# Queued AMD64 Altair diagnostic against the original c104 runtime

Prepared and shell syntax checked on 2026-10-02. VPS driver PID 318471 was observed waiting for corrective driver PID 245349. No diagnostic compilation or execution has occurred yet. The earlier full campaign and its subsequent 19-case corrective campaign must both finish before this control launches; the corrective registry must contain exactly 19 reported, non-skipped cases with unchanged source/program manifests. The diagnostic is permitted to run after reported failures: its purpose is to explain a failure, not certify production.

The original c10426b2 runtime checkout, its existing static libraries and headers/fixtures are reused unchanged. Only the Altair test translation unit from f92533e9 is compiled and linked in the report directory. The shared build volume and runtime checkout are mounted read-only; no image or core rebuild is requested. This deliberate diagnostic composition is not a coherent new production candidate. Runtime revision, diagnostic test blob and manifest tool revision are recorded separately in provenance.json.

The test retains its original 120-second limit and assertions. Existing network trace instrumentation can affect timing. Received reply, admission high-water, checkpoint/clock state and the last eight stream-completion events will be captured only if the full-block boundary assertion fails. Results and pre/post program/link-input/source hashes must still be inspected after termination. Shell syntax checking does not prove compilation or execution success.

No EIP status, production gate, original failed result or resource threshold is changed by queuing this diagnostic.
