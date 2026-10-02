# Bellatrix partition phase profile — AMD64 timeout retained

Original 240-second limit, 112-slot scenario, partition interval, duty ordering
and terminal assertions unchanged. The single instrumented test timed out at
240.03 seconds. Last epoch observation is slot 88, equal heads, finality 9 on
both nodes and three consecutive advancing epochs. Terminal checks, including
later finality, were not completed. This does not qualify the complete scenario.
Runtime/static inputs are original c104; only the instrumented test is compiled.
This composition is diagnostic evidence, not a coherent production candidate.
Source/program/link-input hashes before/after are equal, post manifest exit 0.

At slot 88: relay 69.805946 s, nested block admission 18.353232 s and vote
admission 51.372387 s. Admissions occupy 99.885% of measured relay time;
relay remainder is about 0.080327 s. Thus the 57 259 block checks and 104 051
vote checks do not establish historical scans as a dominant cost. Validator
polls total 128.777147 s and clock polls 32.729020 s. Admission timings are
inside relay: adding them again would double-count. These are cumulative
measurements only through slot 88; tracing overhead and later work are not
inferred. Next cost investigation should separate transition/signing/durable
commit work rather than remove checks or increase deadlines.

Result audit exit 1 preserves the timeout. Source hashes, logs, XML, commands
and phase-analysis.json are retained; raw-sha256.json checks original bytes,
SHA256SUMS archived bytes. No extra runtime test is performed by this archive.
