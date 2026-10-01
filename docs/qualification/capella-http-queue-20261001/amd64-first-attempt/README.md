# First AMD64 HTTP queue attempt — failure retained

Source 18ba7ef. Controlled test fails after 26.26 s: publicReady=true, beaconReady=false, privatePending=true, checkpointUnchanged=true. No deadline raised and no unchanged WAN rerun.

Code inspection: CapellaLocalClock::healthy locks the same mutex that poll holds while waiting for reactor admission. Beacon runtime handler first calls healthy, so the atomic immutable publication alone does not prevent a transitive wait. Exclusive cause of WAN T still not proved. Existing native/ARM64 pass exposed a scheduling-sensitive fixture: next regression will explicitly run a clock poll while holding admission.
