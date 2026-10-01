# Isolated RPC client fix: scoped positive and negative qualification

Detached checkout 0836378, separate build subdirectories in the existing
volume; same builder image, no dependencies/image created. Both source trees
stay independent. Original 400-test campaign continues with its original
runtime. Corrected checkout has not been merged to main.

First overload using Json::initializer_list_t compiled, events passed (0.21 s),
four validation/network cases still failed: overload ranking still chose Json.
That failure is preserved. Overloads now accept std::initializer_list<Json>
and explicitly construct an array. Explicit Json object arguments stay on the
original method and must be rejected by the server. New negative validator
assertion checks -32602 with unchanged head/state/pending/events.

Four modified paths pass in 31.26 s: validator 3.50, network 9.20,
signed_network 9.25, bft_network 9.31, no skipped/failed cases. Events helper's
unchanged first pass is retained, not represented as a new all-five campaign.
New nano hash d5c40202...; P2P remains ae567df... . This changed candidate needs
broader/full regression, portable AMD64 and relevant network/lifecycle gates.

Dispatcher fixture replaces io.poll with io.run_one and keeps the exactly-one
admission/no-completion and queued shutdown assertions. Only this modified
transport case rebuilt: passes 0.01 s. Dispatcher runtime untouched. Recipe
moves binary/package provenance before CTest (bash syntax valid). Fresh GitHub
transport qualification remains required; prior full run failure is retained.
