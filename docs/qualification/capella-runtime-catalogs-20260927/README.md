# Capella runtime catalog lifecycle — 2026-09-27

Base revision `0a3daba` plus accompanying changes.

Optional runtime `catalogs` configuration has exactly `handles`, `rootBytes` and
`ttlMs` as positive canonical hex quantities. TTL is range-checked before chrono
conversion. Root budget must fit the largest configured block/message/proof count.
Validation/construction precedes validator startup. Catalogs share the runtime
owner, and therefore cannot outlive the coordinator they borrow. Stop clears the
registry and prevents reopening, before admission/signing workers are stopped.

Two affected tests passed: configured runtime 21.52 s; wire admission/registry
regression 4.99 s (26.52 s wall total). The runtime still produces sixteen blocks
with witnessed restart. A sixth invalid configuration (zero catalog TTL) is
rejected before opening signing journals. Three catalog lifecycle checks cover
configured open/page, invalidation at stop and reopening refusal after stop.
The registry quota/foreign-owner/expiry coverage remains green.

No HTTP methods or CLI profile activation are added. Runtime now owns the service,
receipts and catalogs needed by a future private RPC adapter. Transport identity,
request admission limits, HTTP dispatch and peer synchronization remain open.
No production activation or real funds.
