# AMD: final copies and read-only file probe

Source-only Root controller. No compiler, linker, project program, Docker, SSH, private input, SDK or binary was executed or read by the author. Root performs the operation on the VPS as UID/GID1000. This is not a runtime, bootstrap, loader or campaign qualification.

The seven fixed artifacts are Nano, four helpers, P2P, and the current Core. Their full SHA256/byte pins and exact original paths are constants in the source. Helpers come from the successful current AMD helper namespace; Nano and Core retain their separately qualified pins. The P2P path is mandatory on the CLI and must equal `/home/ubuntu/nano-candidate-e37f349c-5b37-444a-901c-7b7a8747c63a/network/nano-p2p`; its original full-nine tuple is additionally hardcoded. Root keeps that historical row unchanged.

Root supplies only a reduced, already authenticated envelope, not any old SDK/source table. Its exact shape is:

```json
{
  "format": "nano-current-amd-final-runtime-input-v1",
  "sources": {
    "bootstrapAudit": {"path":"ABSOLUTE_PHYSICAL", "sha256":"FIXED", "bytes":1, "material":{}},
    "stoppedCapture": {"path":"ABSOLUTE_PHYSICAL", "sha256":"FIXED", "bytes":1, "material":{}},
    "lifecycleTool": {"path":"ABSOLUTE_PHYSICAL", "sha256":"FIXED", "bytes":1, "material":{}},
    "lifecycleSubmit": {"path":"ABSOLUTE_PHYSICAL", "sha256":"FIXED", "bytes":1, "material":{}},
    "nano": {"path":"ABSOLUTE_PHYSICAL", "sha256":"FIXED", "bytes":1, "material":{}},
    "core": {"path":"ABSOLUTE_PHYSICAL", "sha256":"FIXED", "bytes":1, "material":{}},
    "p2p": {"path":"ABSOLUTE_PHYSICAL", "sha256":"FIXED", "bytes":1, "material":{}}
  },
  "tools": {
    "docker": {"path":"/usr/bin/docker", "material":{}},
    "sudo": {"path":"/usr/bin/sudo", "material":{}}
  }
}
```

The placeholder `material` is exactly the dictionary of integer `st_dev`, `st_ino`, `st_mode`, `st_nlink`, `st_uid`, `st_gid`, `st_size`, `st_mtime_ns`, `st_ctime_ns`, plus lowercase `sha256`. Source descriptors must match the fixed constants; the example's `bytes:1` and `FIXED` are deliberately not runnable values. Root projects the helper/Core rows from its retained new producer admissions, Docker/sudo from their selected utility admissions, and P2P from the historical artifact authority. **Nano has no retained producer host-nine row:** Root must first observe its known bytes with at least three equal samples over two seconds and label that row prospective. The controller preserves that explicit supplied row before/after; it does not call it recovered historical metadata. The envelope SHA/bytes are CLI prerequisites.

```sh
python3 assemble-current-amd-runtime-root.py \
  --input ROOT_PRIVATE_ENVELOPE --input-sha256 KNOWN_SHA --input-bytes KNOWN_BYTES \
  --output-root /home/ubuntu/OWNED_PARENT/current-amd-final-runtime-NEW_NONCE \
  --p2p-path /home/ubuntu/nano-candidate-e37f349c-5b37-444a-901c-7b7a8747c63a/network/nano-p2p
```

The absent output leaf must have a physical parent owned by1000 and be disjoint from the old candidate, helper namespace, Nano output and repository. Only its new directories/files are created. Copies stream from bounded source FDs, checking original FD/path nine fields and SHA before/after. Sources must be ordinary, nlink1, UID/GID1000, and not group/other writable. New executable files become0500, Core0400 and code directories0555 before the first stable admission. Three consecutive equal full-nine/SHA samples spanning at least two seconds are retained. No source chmod, rewrite, journal or existing volume is requested.

`core-build/` holds Nano, the four helpers and Core; `network-build/` holds P2P. Docker uses only these two new directory binds read-only at `/outputs` and `/network-build`. The exact existing fd9 image must inspect as amd64/Linux. Its sole command is a fixed Perl reader checking each file's canonical guest path, FD/path GNU-stat nine string fields, nlink1, UID/GID1000, exact modes and known SHA/bytes. It does not invoke any of the seven artifacts. Guest rows remain separate from source/host rows. Immutable-image file tools are not promoted into a whole-SDK or loader proof.

Global monotonic budget is100seconds, with the final15reserved for cleanup. Client calls use explicit local socket and a new empty Docker config, filtered environment, bounded streams and owned process groups. Cleanup fences only the original Docker/sudo clients, requires exact full ID/name/UUID label/configured and actual image, and verifies absence. No pre-existing resource is removed; the first error remains primary if cleanup or final fences fail. No volumes are created or pruned.

The new private `runtime-bundle-admission.private.json` retains the seven stable host rows, original selected source rows, input/tool rows and separate read-only guest receipt. Its SHA/bytes are returned in the public result for downstream binding. Raw argv, logs, source paths and full metadata stay in the new private namespace. The public summary contains hashes/counts and false flags for programs, compilation, runtime, duties, finality and production. Root must retain the returned descriptor pin; no success exists until its actual execution completes.
