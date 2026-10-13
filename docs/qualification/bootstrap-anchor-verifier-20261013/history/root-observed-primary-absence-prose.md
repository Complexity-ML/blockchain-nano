# ROOT observations of the stopped PRIMARY audit

This is generic diagnostic history reported by ROOT, not a copy or hash of any private report and not an independent execution by this collector.

The V3 public helper self-tests passed on ARM and AMD64. Two subsequent audit preflights refused before DB access because new helper namespace permissions and copied library permissions were too broad. ROOT hardened four new helper directories and seven new copied libraries; compiled bytes/core and all old parents stayed unchanged. No C++ self-test was repeated for that metadata repair.

The bounded actual optional PRIMARY audit then refused at its generic optional-archive stage. That stage alone did not establish cause. ROOT separately used the official RocksDB C API in read-only mode on the already stopped owned copy and measured that the expected PRIMARY token matched but the initial signed anchor archive was absent. Database files were unchanged, writer quiescence was observed and the audit container was absent afterwards. No runtime or signature occurred. The metadata doctor itself did not verify signatures; public signed-anchor authentication was supplied by the separate genuine C++ verification.

Thus the cause is measured absence, while archive equality and operational restoration remain refused/open. Next is a separately reviewed atomic CAS with independent witness retaining the already-signed wire; it is not implemented or applied. No raw/safe private result, token value, request/report/inventory hash or count proof is included or transferred by this package. All private results remain local0600 on VPS. Earlier helper code proposals and every compiler/controller refusal are retained separately.
