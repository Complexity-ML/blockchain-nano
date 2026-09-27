# Bounded Capella catalog relay — 2026-09-27

Base revision `b0e52ae` plus accompanying changes.

CapellaCatalogRelay opens one immutable catalog, validates ordered root pages and
a stable checkpoint sequence/digest, exports one message at a time and advances
only after a positive delivery acknowledgement. Page size, total entries and an
end-to-end deadline bound the attempt. Exported envelopes must match the selected
network identity and kind and fit the ingress wire limits. No complete payload
catalog is retained. The private RPC transport must enforce its response size
limit before parsing; these logical limits do not bound arbitrary injected JSON.

The relay closes its handle on success and failure. Lost close replies do not
report success. Timeout/cancellation has an unknown delivery outcome and late
acknowledgements cannot resurrect success. A delayed open response received after
cancellation is closed once. An open whose response never arrives relies on the
existing server-side TTL/count/root-byte quotas for reclamation.

Fifteen synthetic scenarios passed in 0.32 s: pagination, duplicate callbacks,
empty catalog, changed checkpoint, bad continuation, unsorted roots, unavailable
payload, foreign envelope, rejection, lost delivery/close replies, entry budget,
expired handle and late callbacks during opening/page retrieval.

The enriched configured-runtime integration passed in 21.87 s. The same relay
uses actual Rpc::handle catalog/export methods for blocks, attestations and
slashings, submits exports to the real Capella admission worker and requires
successful closure. Re-admitting already-known messages must preserve the exact
checkpoint. Empty categories are valid; this does not claim newly admitted
slashings or a new independent follower.

Commands:
```
cmake --build build-network --target nano-capella-catalog-relay-tests -j4
ctest --test-dir build-network -R '^capella_catalog_relay$' --output-on-failure
cmake --build build-release --target nano-capella-transition-tests -j4
ctest --test-dir build-release -R '^consensus_capella_configured_runtime$' --output-on-failure
```

The delivery callback is an adapter boundary, not proof of peer consensus. Sorted
hashes are not parent-before-child order: the production adapter must resolve and
validate missing ancestors before acknowledging block delivery. Daemon scheduling,
peer delivery, bounded dependency fetching, expired-catalog retry and advancing
distributed finality remain open. Consensus network activation remains disabled.
