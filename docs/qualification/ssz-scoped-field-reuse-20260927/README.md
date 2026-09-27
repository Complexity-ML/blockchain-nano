# Scoped authenticated SSZ field reuse — 27 September 2026

`transform_capella_beacon_state` owns an input snapshot, computes and verifies
its full root, and only then invokes the private transformation callback.
Variable-field roots are captured internally during authentication. Output
parsing validates offsets and bounds; a field root is reused only if its exact
bytes and length equal the same input field. Changed fields take the normal
validation/hash path. Fixed fields and the final container are recomputed.
The limits/fork are identical for both passes and the cache cannot escape the call.
No caller-provided field roots or persistent pointer-identity cache are accepted.

Withdrawals use this transform, retaining the direct bounded sweep and exact
advertised-list check. The owned snapshot isolates reads/cache from subsequent
changes to the external input. The output is a separate private byte vector.

Seven suites pass: official roots Phase0/Altair, containers Bellatrix/Capella,
Capella payload/withdrawals, paired execution/consensus and SSZ scale. New checks
on all eight official Capella states cover unchanged transforms, every nonempty
variable field changed independently, wrong input root before callback, nullopt
rejection, malformed output offsets, and external-input mutation isolation.
The container suite now records 105 commitment mutations and 88 malformed
rejections. Existing withdrawal selection/late-hash cancellations still pass.

| Validators | Previous preparation µs | New preparation µs | New peak process RSS bytes |
| --- | ---: | ---: | ---: |
| 100000 | 291692 | 148627 | 68681728 |
| 1000000 | 2911148 | 1525469 | 639090688 |

Pre/post roots match the prior run at both sizes. At one million, ~2.91s becomes
~1.53s, with ~639 MB peak process RSS (including test copies). One run/version;
not a latency guarantee. Other vectors use the minimal preset. No BLS, full
transition, persistence, distributed capacity or public production qualification.

The remaining full input hash dominates this operation. Broader runtime work
must establish authenticated-state lifetimes across steps before reusing more
work; skipping authentication on arbitrary input is not justified by this result.
All EIP/global production gates remain open.
