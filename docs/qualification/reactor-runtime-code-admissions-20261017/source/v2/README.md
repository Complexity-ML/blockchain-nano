# V2: accept the existing Native P2P terminal fence

SOURCE ONLY; no proposal import, test, producer execution or private input read by the author. V1 remains frozen and byte-exact: emitter `3ff413f28ad0ba6e899fd234e3e9dc4bde7a19cdad58a07ca8cd88ab3b69d8ba` /27064, manifest `aa9cf9d733a74b61cc8c2424029cd6ac957e5e6fa3794c169ebc4110e26303cb` /2139.

Root found an input-contract blocker before execution: the actual public Native P2P `original-producer-fence.json` has `exitCode:0`, `originalFenceAfterPassed:true`, `fenceError:null`, plus elapsed time and 902 retained producer files. It does not contain a `passed` field. Its actual SHA/bytes are `a400cb2cd175932f220bb38a6ab85cbe1dce74d84bcd515c4530389f5342f679` /131. V2 consumes this real receipt directly, rather than manufacturing another one.

The only product-code delta is `completion()` and its Native P2P call. A normal terminal receipt still requires externally pinned `checks.passed:true`. Only the `p2p()` call for `mac-native` may instead require the exact typed triple: `exitCode` integer zero (boolean false is invalid), `originalFenceAfterPassed` boolean true, and an explicitly present null `fenceError`. All Root-requested check fields are still matched with exact type and value against the authenticated receipt. An explicit `passed:false`, any `firstFailurePreserved` other than false, or any `oldAuthoritiesRefreshed` other than false is refused. Native runtime completion, ARM, AMD, and their P2P completion calls do not get this alternative.

`CONTROL.example.json` is still two INCOMPLETE examples with `ready:false`, never executable authority. Its Native P2P completion now names and pins the actual public receipt and three predicates. Its worker location/pin names V2. Root must write and externally pin one new control, fill the other retained original rows/producer locators, and set the new package/MF/admissions source base to documentary HEAD `85f6922fb5dd2516905f080230ba518d2d39f9d8`. Producer source revisions and qualified runtime build revision remain `1925eeeabc9f7d0aef41f2c3fe43c11c31f1f352`, with the same explicit selected-tree/CMake and P2P source joins.

All other V1 behavior and bounds remain byte-identical: same selected 17 local and seven remote software files, same original-nine/SHA checks, new-only metadata admission, the same index+join → manifest → three 13-field admissions → separate receipt sequence, and the existing pure R APIs. No build, software smoke, source mutation, key, DB, signing, network or campaign operation is added. See the V1 README for the unchanged full contract and limits. This fixes a setup contract; it is not evidence that either emitter mode has executed successfully.

Root-only CLI:

```text
python3 /absolute/v2/emit.py audit-amd /absolute/new-control.json CONTROL_SHA CONTROL_BYTES
python3 /absolute/v2/emit.py emit /absolute/new-control.json CONTROL_SHA CONTROL_BYTES
```
