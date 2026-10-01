# Automatic C++ Docker qualification

GitHub run 36890479029, commit dad9a14c60ef0f87ba6f08ec63e2a77accf617d9,
completed/success. The push triggered changes → core automatically.
Operations: 232 passed, zero failures/skips, 4725.811149 ms.
Docker core job: 12m06s, Ubuntu pinned manifest, 2 CPU/3 GiB.
Exact nine CTest registrations and nine JUnit cases verified; zero failures,
errors or skips, 27.44s. Logs from configure/build/tests are streamed and
archived. Nine executable SHA-256 records and actual build package versions
are present, tied to the checked source revision.

New coverage is CI routing, live failure-preserving logging and provenance.
The C++ subset itself is unchanged. This is not full P2P/EEST CI, a complete
regression, reproducible release or production readiness. No runtime image
was built/deployed by this workflow. Build logs are deterministically gzipped.
