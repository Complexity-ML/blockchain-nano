# Candidate images: packaging and integrity only

Two final runtime images were assembled locally from existing build-volume
executables and existing Ubuntu runtime images. No C++ recompilation, apt step,
source transfer outside Git, chain mount, signing or public deployment occurred.
Build logs A preserve the failed bare-ID FROM attempt. Logs B record the fixed
local named reference resolving to its exact digest. The final images contain
both expected executable hashes, resolve shared libraries, and run Nano help
as UID/GID 10001. nano-p2p has no --help mode: its usage refusal exits 1; this
is not a P2P network qualification.

The preflight reports integrityVerified=true and productionQualified=false.
ARM64 uses the actual CLI; x86-64 uses the same verifyCandidate implementation
with read-only SSH Docker probes because Node is not installed on the VPS.
All manifest artifact hashes in both cases are read on the Mac, not on the VPS.
The small node-example.json is the existing unsigned Compose example, not a
production Capella validator configuration. Genesis/profile files are public
synthetic fixture inputs. The prior evidence manifest references the earlier
volume-based network run, not a run of these new packaged images.

The packaging script accepts separately reviewed input hashes and a compiled
revision marker. A marker/label alone is not verified source derivation.
source-equivalence.json records unchanged tracked runtime/build paths between
compiled and packaging revisions; dependency provenance, deterministic rebuilds,
security review, a coherent complete network configuration and fresh packaged
image network/update/rollback qualification remain open. Base image inherited
shared genesis/configuration files are historical examples; never activate them
as a production network. The default command is --help; explicitly mount the
reviewed network inputs and external keys for any subsequent isolated campaign.

No service was updated, no signing journal was restored, and no funding or sale
was opened. Anonymous volumes of removed smoke containers are ephemeral only;
all existing chain/build volumes and existing tagged runtime images remain.
