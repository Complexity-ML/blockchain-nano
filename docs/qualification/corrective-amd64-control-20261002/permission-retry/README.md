# Dedicated report-directory permission failure and retry

The original queued corrective driver passed its 417-result terminal gate, then
failed before building or testing: bash line 3 could not create
/reports/manifest-tool.cmake (Permission denied). Container exit 1 and missing
registry/result evidence are retained. Source manifests remained identical.
Altair and Bellatrix drivers stopped at their missing-report gates; neither
ran a diagnostic or profile. Their gates were not bypassed.

The directory was ubuntu-owned mode 0775; the root container drops all
capabilities, including DAC_OVERRIDE, and could not write there. The failed
report directory is retained on the VPS as
/home/ubuntu/nano-corrective-1ac57e25-amd64-20261002-permission-failure-reports.
Only a fresh, dedicated report output directory is made mode 0777. Read-only
sources, capability drops, resource bounds and test criteria are unchanged.
This is an infrastructure retry after a confirmed terminal setup failure;
no previous successful test is repeated.

Revised host drivers are archived here. Corrective host PID 398934 compiles
1ac57e25; Altair PID 399610 waits on it, Bellatrix PID 399783 waits on Altair.
Original scripts and diagnostic code/provenance remain intact. Inner controls
are unchanged; runtime revisions, original test deadlines and assertions are
preserved. Original report gates still require exact counts, no skips and
unchanged sources/programs before the next phase starts. Pending results are
not qualified. Sessions: corrective 81463, Altair 16385, Bellatrix 51636.
