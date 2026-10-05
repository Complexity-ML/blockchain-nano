# Distinct shared finality preparation

Fresh 16/16/33 restart pins come from v3 terminal post-stop readonly captures. Native and Docker Mac use their own independently audited full copies; no signer journals are merged or reset. Each copied file except peer.key matches its respective source SHA. VPS data remains on the VPS and reuses its quiescent audited volume.

The updated controller requires three distinct shared checkpoint advances plus stable reads. Preparation has not launched any signer and does not qualify network/production. Source core 2795d4b5, relay 8adbabff, six binaries unchanged; no new image build.
