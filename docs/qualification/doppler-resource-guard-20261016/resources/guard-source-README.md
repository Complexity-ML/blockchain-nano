# External campaign resource guard — source proposal

This package is outside the qualified C++ and multihost source tree. It changes
no code admission, spec, seed, key, persistent ledger, image or existing volume.
No author has executed this module, its tests, Docker, SSH, or a campaign.
Root must review and pin the source and controls before any invocation.

The guard adds test resource bounds, not protocol rules or campaign permission.
The existing `run.mjs` continues to validate the exact spec, three owner
admissions, and every operational authorization before fixture export. A successful
resource probe proves neither duties nor common finality or production readiness.

## Measurements and policy

Three startup samples must pass before a launcher can be created. Subsequent
samples run after at most 10 seconds of waiting; each sample has a 5 second
software deadline. A failure, missing surface, parse refusal, SSH loss, or stale
sample refuses startup or requests an orderly stop. Polling and OS scheduling
are not an ENOSPC/OOM prevention guarantee; a fast write burst can consume the
remaining capacity between observations. Ordinary filesystem/tool fences also
depend on a responsive local OS.

| Surface | Available disk | Memory used by policy |
| --- | --- | --- |
| Mac host | `statfs.bavail * bsize` on the campaign output parent | `(Pages free + Pages inactive + Pages speculative) * pageSize` from pinned `/usr/bin/vm_stat` |
| Mac Docker VM | `df -Pk /probe` on an existing Docker volume | `/proc/meminfo` `MemAvailable * 1024` |
| VPS host/Docker storage | minimum of `df -Pk / /home/ubuntu /var/lib/docker` | `/proc/meminfo` `MemAvailable * 1024` |

The Mac value is explicitly a **reclaimable-memory estimate**, not Linux
`MemAvailable` or a guarantee that all pages can immediately be reclaimed.
Purgeable pages are not added a second time. The parser requires one page size
and each of the three counters, rejects duplicates/overflow, and bounds the
estimate by physical RAM. `os.freemem()` is recorded only for information.
Linux values describe the host/VM, not a promise about a Nano cgroup's limits.

Startup requires disk **at least 4 GiB** and policy memory **at least 3 GiB** on
all three surfaces. During a campaign the floors are **2 GiB disk / 1 GiB memory**.
These are explicit Root-selected test thresholds. They do not establish a safe
production sizing model for a seven-hour run.

The only VM mount is the existing volume
`nano-local-supervisor-507081131d57-data`, read-only at `/probe`. Before creation
the guard requires its exact name, `Driver=local` and Root-supplied complete
labels; absence refuses rather than creating a volume. It compares inspection
again after creation and checks the actual container mount. This is a host
Docker volume filesystem, not a Mac file bind. No data file is opened. Concurrent
external removal/recreation of this protected volume is unsupported; an observed
change refuses. Docker's name-based mount API itself is not an atomic old-volume
handle guarantee.

The temporary sampler uses immutable image
`sha256:e30ce85e363309a64814c300fbf07b482e28b841c8f9577cba9b4e70070d37fb`,
Linux/arm64, UID 501:20, read-only root, network none, no capabilities, no new
privileges, 64 MiB / 0.1 CPU / 16 PIDs. It runs only a sleep, `df` and `awk` over
`/proc/meminfo`. It never executes a project program. The VPS command is fixed
read-only `df` (using `sudo -n` for `/var/lib/docker`) plus `MemAvailable`, to
`ubuntu@145.239.199.166`; SSH is BatchMode with a 2 second connect timeout and
strict host-key checking. Existing SSH authentication/policy remains Root's.

## Control interface

CLI (absolute reviewed paths; no default execution mode):

```text
NODE campaign-resource-guard.mjs --config CONTROL --config-sha256 SHA256 --config-bytes BYTES --probe-only
NODE campaign-resource-guard.mjs --config CONTROL --config-sha256 SHA256 --config-bytes BYTES --launch-reviewed-campaign
```

`--probe-only` collects exactly three startup samples and never invokes the
launcher callback or reads a seed/ledger. A failing first sample stops early.
The second mode still grants no authorization; the pinned launcher owns all
existing checks. Root must separately authorize any real campaign.

The pinned JSON has exactly these ten top-level fields:

```text
{
  format: "nano-external-campaign-resource-guard-v1",
  tools: {node: Artifact, docker: Artifact, ssh: Artifact, vmStat: Artifact},
  launcher: Artifact,
  spec: Artifact,
  sourceBaseRevision: "40 lowercase hex",
  outputDirectory: "/absolute/new-private-guard-output",
  campaignOutputDirectory: "/absolute/new-launcher-output",
  dockerEndpoint: "unix:///Users/boris/.docker/run/docker.sock",
  proofVolume: {name: "nano-local-supervisor-507081131d57-data", driver: "local", labels: {}}
}
Artifact = {path: "/absolute/physical/file", sha256: "64 lowercase hex", bytes: positive integer,
            material: [dev,ino,mode,nlink,uid,gid,size,mtimeNs,ctimeNs]}
```

Every material element is a canonical unsigned decimal **string**. Root supplies
the original retained nine and SHA; this module never refreshes them. `labels`
must be the full Root-authenticated existing label map, not an invented empty map.
The Node path must be the physical running interpreter, launcher is exactly
`/Users/boris/Dev/blockchain-nano/tests/multihost/run.mjs`, SSH is `/usr/bin/ssh`,
and vmStat is `/usr/bin/vm_stat`. Root's source/tool closure remains separate;
pinning a tool here does not prove its complete OS loader/SDK provenance.

The config is bounded to 1 MiB, the spec to 1 MiB, each tool to 256 MiB. Reads use
FD/path full nine plus known SHA before/after, excluding atime. The config's first
read is known-SHA-bound; Root must provide its prior stable admission externally.
The guard rechecks all pins before launch and in common cleanup; measurement
tools are rechecked around each use. Source/spec/code/seed/ledger paths are never
accepted as output directories. Both output names must initially be absent and
have physical, owned, non-group/world-writable existing parents.

Only this fixed launcher argv can be constructed:

```text
NODE /Users/boris/Dev/blockchain-nano/tests/multihost/run.mjs ubuntu@145.239.199.166 SOURCE_BASE NEW_CAMPAIGN_OUTPUT --fresh-campaign SPEC --fresh-campaign-sha SPEC_SHA
```

The spec must bind the same source revision/target and `maximumRunMs=25200000`
(7 hours). The guard does not replace `validateFreshCampaignSpec` or the runtime
authorization checks. Launcher environment is inherited without being logged;
Root must launch this guard under its reviewed environment. Read-only probes use
a fixed minimal environment and a new private Docker config directory with an
explicit local endpoint.

## Stop and preservation

The guard retains the exact ChildProcess handle that it creates. A resource
failure, whole-run timeout or wrapper SIGINT/TERM/HUP sends **one SIGINT only** to
that launcher handle. It does not signal a process group, repeat SIGINT, inspect
global process lists, or ever signal PID 28722. The launcher owns its cleanup of
its children. Its once-handlers need time for three owner captures, joins,
replays and fences: the guard waits up to **1800 seconds**. A still-unjoined
launcher is reported as a failure and preserved for Root; there is no automatic
SIGTERM/SIGKILL escalation of the launcher or its children.

Only the guard's own short read-only measurement subprocess groups can receive
bounded TERM/KILL on timeout. Only the newly attempted UUID-labelled sampler
container can be removed, after exact full ID/name/image/label/mount checks.
Cleanup has a separate 15 second bound and uses original Docker authority even
if another source pin drifted. No existing volume, image or container is removed;
there is no prune. A first failure is retained even if cleanup also fails.

`resources.public.jsonl` contains only numeric resource observations, metric
names and phases. `result-public.json` contains static reason codes, child join
status, source revision and production false. Launcher stdout/stderr remain
private mode 0600 files; this wrapper never prints them, env values, keys, raw
config or arbitrary exception messages. Root should treat those launcher logs
with the existing private-data policy.

## Pending Root checks

`campaign-resource-guard.test.mjs` is a new pure suite: exact memory parsing,
disk/memory boundaries, missing/stale VM samples, duplicate/overflow refusal,
no-spawn preflight/probe mode, one-child signal identity, SSH loss, seven-hour
deadline and unjoined grace. Tests use fake child/clock/scheduler dependencies;
they do not sample the OS or prove physical Docker/SSH behavior. No author has
run them. Root will run only these new tests and a three-sample `--probe-only`
invocation after review. Real campaign permission, capacity over seven hours,
duties, common finality and production remain open.
