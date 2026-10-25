# Terminal fixture dependency correction (GEN03)

SOURCE ONLY, no program/test/process executed by the author. GEN01 source NO_GO,
GEN02, and Root's first FAILED Docker batch remain unchanged.

Root's public `../closeout-io-amd-root-01/retained-stdout.log` reports 41 groups:
31 PASS, 5 FAIL, 5 SKIP. All twelve new I/O cases, all six guard/NEW cases and the
selected harness closure case passed. Five terminal cases failed because their
extracted, unchanged finally referenced two globals absent from the synthetic
fixture: `specSha` and `dockerRuntimeInspectTemplate`. Root separately reported
all 241 input descriptors fenced and closed; this author did not run that audit.

Only `terminal-evidence.test.mjs` is replaced. The fixture now imports the same
pure bounded Docker projection template as the actual runner, passes it into
the extraction and asserts the adapter receives exactly four inspect arguments
with that template. No parser/owner guard is bypassed. The unchanged synthetic
spec object is named `fixtureSpec`; `specSha` is SHA256 of its JSON bytes, and
the closeout receipt must echo it. It is fixture authority, not a real campaign
admission. Every existing test-body assertion and the actual-finally extraction
remain unchanged; two fixture assertions strengthen the new dependency wiring.

No runtime source is delivered or changed. GEN02 `run.mjs` remains
`f943a69b290502facc26ff9588c68d72163f08d515564c1bf75d71a1bae4b5a9`.
Its complete fresh finally is still unchanged relative to the original baseline.
No budget, signal, clock, timeout, file-authority, Core/CMake or production rule
is altered by GEN03. Full physical closeout and post-SIGINT capture remain OPEN.

`TARGETS.source.json` names exactly the five previously failing cases. Root is
to rerun only those five against the new fixture bytes, expecting 5 selected
groups and 12 visible skips in this 17-group file. The previous 31 PASS retain
their prior bytes/scope; the first batch remains FAILED. A later successful
targeted run must be recorded separately, not rewritten into that old result.

SourceMerge independently reviewed the narrow diff and gave bounded SOURCE GO.
Root runtime qualification and terminal observation are still required.
