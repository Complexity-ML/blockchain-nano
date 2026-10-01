# WAN attempt F — strict deadline failure retained

Runtime 5165e51, corrected harness b158628, three processes on two hosts with 64 synthetic validator keys; Capella/Shanghai. Existing immutable Docker ARM64/amd64 images reused, no rebuild. Campaign terminal exit 1. First cut: actual 97,499 ms, accepted recovery 141,559.221 ms measured with the monotonic clock. Second cut: actual 193,134 ms; the first completed common-finality sample is recorded 180,183 ms after reconnection using wall event timestamps. The corrected harness rejects this late read; it does not report an accepted monotonic recovery duration. Exact physical finality time within the sample interval is unknown. No threshold change.

The diagnostic extension preserves the failure. Common finality then advances 15→18 in 144,742 ms; all three terminal validator receipts are healthy. Both Docker containers exit 0 without OOM. Containers, data volumes and native databases retained. No clock fault reproduced; earlier WAN A/B causes remain open.

Archived full runtime traces verify admitted-payload quanta; see relay-analysis.json. They do not prove causal performance improvement or general production readiness. Next action is to analyze the catch-up/finality timeline before any further rerun, preserving this strict failure.
