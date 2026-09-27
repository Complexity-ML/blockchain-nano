# Bounded reusable consensus receive session

The asymmetric 500ms finality failure remains OPEN. This is a receiver
primitive, not an activated transport protocol or a production qualification.
`network/main.cpp` still opens one stream per message. No deployment or Docker
campaign was performed for this tranche.

## Implementation

`AltairStreamSession` accepts sequential frames on a single stream, at most 16
(default) and with a per-frame deadline no greater than 10 seconds. The caller
must negotiate a separate compatible protocol and check current peer/session
registration at each dispatch. Those integration steps remain outstanding.
Each frame reacquires the existing ingress budget, validates its envelope/domain,
and waits for durable worker completion before writing its own ACK. Accepted and
rejected admissions remain distinct. No pipelining, retries or quota exemptions.
Idle reads hold an ordinary admission lease until deadline; worker cancellation
retains its lease until worker completion. Total session lifetime can reach
160 seconds; the existing global and peer active limits still apply.

The optional request continuation reports ACK transport completion, NOT consensus
acceptance. Legacy requests without that callback retain their close-after-ACK
behavior. Session cancellation also resets an already-finished request's stream,
covering the ACK/next-frame boundary.

## New coverage and evidence

Native P2P and stream-test targets build. `altair_stream` and `timed_reply` pass.
The stream suite grows from 19 to 30 cases (11 new cases):

- three sequential admissions, accepted/rejected/accepted, duplicate completion,
  maximum-frame close, lease accounting and session ownership release;
- malformed second frame, idle timeout, cancelled running worker, ACK write
  failure, and local admission failure (five separate cases);
- cancellation from the ACK observer, before any continuation;
- other work consuming peer concurrency between ACK and next frame;
- deterministic exhaustion of the peer bulk token bucket on reuse;
- second-frame chain-domain mismatch;
- write success delivered after the ACK deadline must not resume the session.

The first test run failed `failed session continued or leaked admission`.
Review showed the test incorrectly demanded exactly one low-level reset in all
failure paths: the request deadline and JSON-read deadline can both reset the
same stream. The assertion now accepts one or more resets for these five paths,
while still requiring exact dispatch/ACK counts, zero close calls, zero remaining
leases and released session ownership. Other deterministic boundary cases retain
exact reset-count assertions. No timeout or admission policy was weakened.
The initial failure was observed in tool output; its original CTest file was
replaced by the later run. This paragraph preserves that failure without claiming
an archived original log.

Build/test logs and source/binary SHA256 values are retained alongside this file.
There are no real libp2p peer connections in this suite.

## Next gate

Implement versioned capability negotiation and a bounded sequential sender with
peer-generation invalidation, idle expiry and no ambiguous delivery retry.
Integrate both ends, qualify missing/late ACKs and peer changes, then rerun the
unchanged asymmetric-delay campaign. This tranche does not close any EIP or the
production goal.
