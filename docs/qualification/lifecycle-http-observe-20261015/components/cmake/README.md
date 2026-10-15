# Lifecycle component CMake composition

This source-only overlay adds the two previously frozen component registrations to the exact current public CMake baseline. The Linux HTTP/exec block is byte-exact from manifest f0d5bf3a and the observe registration is byte-exact from manifest 811f4ff1. Removing those two insertions reconstructs the baseline SHA256 exactly.

The HTTP test adds one Linux-only translation unit/target and depends on the unchanged lifecycleSubmit executable. It exercises real loopback HTTP and a controlled daemon exec from a synthetic parent; it does not qualify the real Nano supervisor, cryptographic admission, a private audit or a lifecycle campaign. Docker execution requires an executable private fixture filesystem selected by TMPDIR; the prior /tmp noexec failure remains historical. The observe test runs only the new authenticated HEAD discovery selector.

No core source, existing target, legacy test command, compile flag or clock/profile changes. This proposal has not configured CMake or compiled/executed any program. ROOT alone imports and executes.
