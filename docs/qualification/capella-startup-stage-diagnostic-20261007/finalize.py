from pathlib import Path
import json,hashlib,shutil
repo=Path('/Users/boris/Dev/blockchain-nano');root=Path(__file__).parent;audit=Path('/private/tmp/nano-v33-post-stop-readonly-20261007')
def read(p):return json.loads(p.read_text())
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def save(p,x):p.write_text(json.dumps(x,indent=2)+'\n')
for arch in ['arm64','amd64']:
 for gate in ['compile-runtime','link-nano','link-test','configured-runtime-trace']:assert read(root/(arch+'-build')/(gate+'-exit.json'))['exitCode']==0
 assert (root/(arch+'-build/parent-before.sha256')).read_bytes()==(root/(arch+'-build/parent-after.sha256')).read_bytes()
 assert read(root/(arch+'-build/trace-check.json'))['onlyAllowedFields']
manifest=read(root/'manifest.json')
for rel,h in manifest['files'].items():assert sha(repo/rel)==h['baseSHA256'];assert sha(root/'source'/rel)==h['draftSHA256']
assert read(audit/'vps/exit.json')['exitCode']==0;im=read(audit/'vps/immutability.json');assert im['hashesUnchanged'] and im['exactUnchangedSignerPins']==33
ap=repo/'docs/qualification/v33-post-stop-readonly-20261007';ap.mkdir();shutil.copytree(audit/'vps',ap/'vps');shutil.copy2(audit/'vps-audit.py',ap/'vps-audit.py');shutil.copy2(audit/'vps-audit-original.log',ap/'vps-audit-original.log');save(ap/'original-exit.json',{'originalSession':34298,'exitCode':0,'terminalChunk':'7b5187','publicFetchOriginalSession':18652,'publicFetchExitCode':0,'publicFetchTerminalChunk':'195257'})
(ap/'README.md').write_text('''# V33 forced-stop read-only integrity audit — 7 October 2026

Original34298 exit0. VPS capture ran204.706 seconds with one CPU and read-only stores, after V33 forced stop137/noOOM.2936 files exact before/after. Primary and independent recovery witness at a common publication boundary; full canonical replay and required finalized ancestor verified. Core sequence0x3b93→0x3b94, new digest recorded in capture. All33 signer commitments exactly unchanged against V32, independently witnessed. This proves scoped crash integrity, not successful runtime recovery, duties, live finality or historical token digest ancestry. No Mac writer ran duringV33; current Mac copies/sources remain unchanged.

Only public capsule, exit/error/immutability and script are retained here. Config/request/full file hashes and source database stay private on VPS. No signer/network started by this audit. The204.706-second read-only audit is not a startup benchmark. V33 readiness failure and all prior failures remain open evidence.
''')
save(ap/'manifest.json',{'sourceRevision':'c03e0a0eb0d74c53bbf383c15606823c9e5b60bd','sourceCore':'df31cd48a3d1680f7d864190fcbdd5e437683a23','sourceContainer':'nano-restored-three-node-vps-muxbpvx9','sourceForcedStopExit':137,'networkQualified':False,'productionQualified':False,'files':{str(p.relative_to(ap)):sha(p) for p in sorted(ap.rglob('*')) if p.is_file()}})
proof=repo/'docs/qualification/capella-startup-stage-diagnostic-20261007';proof.mkdir()
for name in ['source','arm64-build','amd64-build']:shutil.copytree(root/name,proof/name)
for name in ['manifest.json','draft.patch','build-docker.py','arm64-original.log','amd64-original.log','finalize.py']:shutil.copy2(root/name,proof/('draft-input.json' if name=='manifest.json' else name))
save(proof/'original-exits.json',{'arm64':{'originalSession':53167,'exitCode':0,'terminalChunk':'bcd367'},'amd64':{'originalSession':6363,'exitCode':0,'terminalChunk':'b87e21'}})
(proof/'README.md').write_text('''# Private startup stage timing — 7 October 2026

V33 hit the180-second VPS IPC readiness limit. Instrument start_capella_runtime with the existing NANO_TEST_NETWORK_TRACE=1 flag only. Fixed stage names distinguish input validation, witnessed reactor recovery, checkpoint migration/setup, signing journals, startup catch-up, initial clock poll, worker launch and runtime return. Monotonic total/previous-stage timing, no config/key/token material. Logging errors are swallowed so telemetry cannot fail boot. No validation, clock/consensus logic, network settings, budgets or timeout changed. A subsequent real startup still must identify the costly phase; instrumentation alone does not fix the defect.

Original ARM6453167/0 and AMD646363/0. Recompile only consensus_capella_runtime.cpp into independent copies of the qualified df31 core archive and relink nano/test. Configured-runtime check passes on both with trace enabled:16-block fixture, witness/signing restart and six invalid configuration refusals. All stage fields bounded by an explicit allowed-field set, nonnegative timing, runtime-ready observed. Parent nano/test/capture/library hashes unchanged. Existing Docker images/volumes reused; no image build, source-node replacement or network launched. Native macOS artifact not compiled for this diagnostic yet. No clean release, full CTest/EEST, large-store performance or WAN qualification claimed.

Build paths retain their diagnostic name. Draft input records the parent checkout/hash; source file copied here matches the tested file exactly. Next: use timings on a safely restored current source, correct the measured cost, then qualify recovery and advancing three-node common finality. Production/Base–Nano/Doppler and independent review remain open.
''')
save(proof/'manifest.json',{'sourceParentRevision':manifest['sourceRevision'],'sourceFiles':{rel:h['draftSHA256'] for rel,h in manifest['files'].items()},'networkQualified':False,'productionQualified':False,'files':{str(p.relative_to(proof)):sha(p) for p in sorted(proof.rglob('*')) if p.is_file()}})
for rel in manifest['files']:shutil.copy2(root/'source'/rel,repo/rel)
msg='Audit V33 après arrêt forcé terminé34298/0 :2936 fichiers inchangés, checkpoint cœur0x3b93→0x3b94 et témoin indépendant vérifiés,33 engagements de signature exactement conservés, finalité inchangée. [Audit](qualification/v33-post-stop-readonly-20261007/README.md). Diagnostic par phases de démarrage ajouté sous NANO_TEST_NETWORK_TRACE=1 ; compilation/relink Docker ARM64 et AMD64 et test configured-runtime16 blocs passés, binaires parents inchangés, aucun nœud lancé. [Périmètre](qualification/capella-startup-stage-diagnostic-20261007/README.md). Ce diagnostic ne corrige pas encore le coût de reprise ; source du retard à mesurer, natif Mac non recompilé pour ce changement, V33 FAILED et tous critères production/Base–Nano/Doppler/revue indépendante conservés.'
for name in ['PRODUCTION-GOAL.md','VALIDATOR-EIP-ROADMAP.md','EVM-PRODUCTION-EIP-ROADMAP.md','EIP-IMPLEMENTATION-STATUS.md']:
 p=repo/'docs'/name;s=p.read_text();needle='## État courant vérifié — 7 octobre 2026\n\n';assert needle in s;p.write_text(s.replace(needle,needle+msg+'\n\nÉtat précédent conservé : ',1))
print('Read-only audit and scoped diagnostic proof archived; exact compiled source integrated; no runtime started')
