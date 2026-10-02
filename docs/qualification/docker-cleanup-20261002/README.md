# Docker cleanup — 2026-10-02

User requested a full cleanup and a clean subsequent build. Executed builder prune and system prune of unused resources on Mac and VPS. Running containers and every volume were retained pending explicit clarification on destructive volume deletion. Commands reported 20 GB + 6.438 GB reclaimed on Mac, and 7.98 GB + 1.264 GB on VPS; totals are command-reported, not a physical Docker Desktop disk-file shrink measurement.

The former ARM builder image `sha256:6116669430b02b7e8012af8c8bd515a5a7dcd1dd2034649f9b45ae5b9e70a3a4` is now absent. `nano-linux-sync-build` is still present. Archived ARM qualification remains historical evidence, but future live-container checks cannot reuse the deleted image until a replacement is built and identified. No new build was launched.

The original VPS campaign remains running at PID 946683, with 389 completed cases: 388 passed, the retained WebSocket RPC failure, and 38 cases outstanding. Its timing diagnostic PID 1082858 still waits. No campaign restart, runtime source mutation, public launch, or production qualification occurred.

## Full reset subsequently authorized

User explicitly requested cleanup from A to Z. All Mac/VPS containers, images, volumes and caches were removed. Both `docker system df` outputs now report 0B in every category. Mac BuildKit retained leases required Docker Desktop restart before the final prune. Build history was removed on Mac. Reports stored outside Docker and archived Git evidence remain. The original VPS campaign was deliberately interrupted at 393 completed / 392 passed / 1 retained RPC failure, with 34 unfinished tests; this is not a terminal qualification. Its waiting network/signed/timing controllers were terminated before deleting resources. The diagnostic was not acquired. All previous Docker images and live build-volume artifacts are absent; future execution requires a clean identified build. No new build or public deployment started.
