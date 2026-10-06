# V33 terminal failure — 7 October 2026

Original controller session5043 terminal exit1. Preflight passed23:42:30.689 UTC; VPS started23:42:31.902. IPC readiness deadline failed23:45:32.393, terminal23:45:53.978. VPS stopped137 after the unchanged20-second stop grace, OOMKilledfalse. No Mac native/Docker writer started, no stop receipt, no verified duties or new common finality. No restart launched. Clock startup catch-up completion was logged, but IPC readiness was never observed. This does not establish which startup phase consumed the full time or why shutdown failed to complete within its grace.

Core df31cd48, relay77848f1f, controller checkout22d73d57 and all controller file hashes retained. This run never reached the900-second three-common-checkpoint test. The immutable block-inventory correction remains locally qualified only; it has not resolved WAN finality by this evidence. The180-second startup limit and all budgets were not widened.

Next required work: independently audit the forced-stop database and signing journals/witnesses read-only, then identify and correct startup/recovery cost before another writer. V32 and earlier failures remain historical evidence. All production/Base–Nano/Doppler/independent-review gates remain open. No funds, sale or public deployment.
