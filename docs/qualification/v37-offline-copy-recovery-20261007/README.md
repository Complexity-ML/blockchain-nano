# V37 failed-runtime source — recovery on isolated physical copy

Original V37 remains FAILED with VPS exit1 and no terminal VPS signing receipt. The original read-only audit refused primary/witness mismatch and left all3216 files unchanged (previous integrated proof). Independent physical copy preserves3216 files/328467624 bytes. Original source was never mounted writable by this recovery.

Offline recovery V2 original25616/0: primary0x410b -> witnessed pending intent0x410c, idempotent reopen preserves0x410c, full canonical replay passes, all33 current signer/independent-witness pairs and pre-V37 external minima verified. All signer/peer file bytes on copy unchanged; original3216 files unchanged at final comparison. No RPC/P2P/duties/signing started. Private capsule remains on VPS; its external seal is retained here. Historical digest ancestry and an unavailable latest terminal external signing receipt are not invented.

First helper original91077/1 refused `external copy request binding` before opening owner DB: config profile contains maximumBytes, request correctly binds file/sha only. Original helper and refusal retained; V2 matches the existing capture profile binding and passes. Helper is offline synthetic fixture tooling, not a public recovery service.

Explicit runtime-refusal source guard integrated separately from old startup-refusal and audited-kill cases.75 targeted tests pass (including unchanged historical guard cases); actual sealed source state/audit accepted. Runtime-ready event is bound to archived VPS log; effective validator signing is not inferred. Controller requires the committed audit artifact hash before admitting this distinct source case. Default healthy/startup/kill requirements remain.

Core85eb492f pushed and pulled on VPS. GitHub CI37562317444 refused before steps for billing; network/core jobs skipped. No CI success claimed. Existing images/volumes reused. Recovered copy is not yet a live resumed network: physical restoration for next campaign, actual duties and advancing common finality remain to demonstrate. All other production, EIP profile, resources, Base-native/bridge/Doppler and independent-review gates remain open.
