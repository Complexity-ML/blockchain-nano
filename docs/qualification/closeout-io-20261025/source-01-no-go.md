SOURCE NO_GO retained before execution. Root/SourceMerge found that the new
unconditional `interrupted` I/O guard would refuse new bounded closeout I/O after
a work interruption. A phase-only bypass would also allow a pending work read
to borrow the new closeout signal. SourceMerge additionally requested admission
after close/check; Recovery identified masking a primary I/O error when close
also rejects. Generation02 addresses these without changing the original clock,
old authority predicates or runtime/signing gates. No tests or actual campaign
were executed for this rejected Source generation.
