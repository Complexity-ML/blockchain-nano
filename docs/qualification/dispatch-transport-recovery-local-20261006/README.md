# Same-session dispatch recovery: isolated native evidence

Current 5902c983 dispatcher freezes after three missing replies and retains its uncertain ticket. Candidate header remains outside the repository source and was not deployed. Native standalone tests: eight simulated-response cases, five cases using actual AltairReceipts with simulated execution, ten existing dispatcher cases (permanent outage now observed in a bounded run), and one actual loopback HTTP case using production CoreRequest five-second deadlines and real receipt ledger.

The HTTP server processes one worker admission, deliberately loses three replies, and preserves its receipt. Baseline exits 1 and never retires it. Candidate exits 0 after Cancel and confirmed Forget, one execution and one completion. Pending work is never retired before completion; cancellation fences a delayed Submit; contradictory sequence or decision remains fail-closed. Same session, watermark and tickets remain fixed. This is not a real consensus worker or WAN qualification.

No core/relay source changed while the v8 network ran. Native compilation and local fixtures overlapped that campaign; resource samples are not an isolated performance benchmark. The campaign's actual failure cause has not been proven by these fixtures. Prototype still needs integration, Docker regression, complete P2P linkage and a new independently pinned network campaign. Core restart/session replacement recovery remains separate. No production qualification.

Compile/link controller failures and sandbox loopback bind denials are preserved alongside corrected commands and terminal exits. No repeat image build. Full raw binaries/objects remain in /private/tmp/nano-dispatch-recovery-20261006; source and program hashes are in manifest-final.json.
