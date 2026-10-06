from pathlib import Path
import json,hashlib,shutil
repo=Path('/Users/boris/Dev/blockchain-nano');source=Path('/private/tmp/nano-restored-three-node-v33-published-inventory-20261007/campaign');root=Path(__file__).parent;proof=repo/'docs/qualification/restored-three-node-v33-terminal-20261007';proof.mkdir()
def read(p):return json.loads(p.read_text())
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def save(p,x):p.write_text(json.dumps(x,indent=2)+'\n')
result=read(source/'result.json');state=read(source/'vps.log.state.json')['State'];assert result['result']=='FAILED' and result['failure']=='VPS IPC readiness deadline' and result['stopReceipts']==0;assert not state['Running'] and state['ExitCode']==137 and not state['OOMKilled'];assert not (source/'resources.json').exists()
controller=read(source/'controller-hashes.json');assert all(sha(repo/f)==h for f,h in controller.items())
for n in ['artifacts.json','controller-hashes.json','events.jsonl','result.json','vps.log','vps.log.state.json','vps-local-preparation-proof.json']:shutil.copy2(source/n,proof/n)
shutil.copy2(root/'original-exit.json',proof/'original-exit.json');shutil.copy2(Path('/private/tmp/nano-restoration-v33-published-inventory-prepare-20261007/campaign-original.log'),proof/'controller-original.log');shutil.copy2(Path(__file__),proof/'finalize.py')
save(proof/'summary.json',{'originalSession':5043,'exitCode':1,'result':'FAILED','failureStage':'VPS IPC startup readiness, before Mac writers','vpsStartedAt':state['StartedAt'],'vpsFinishedAt':state['FinishedAt'],'vpsExitCode':137,'vpsOomKilled':False,'sourceStopReceipts':0,'newCommonFinalities':0,'macNativeStarted':False,'macDockerStarted':False,'startupCatchupCompletionLogged':True,'validatorDutiesVerified':False,'controllerHashesUnchanged':True,'sourceRevision':'22d73d5702bd6a239eb1b5ad98202085b4860874','coreRevision':'df31cd48a3d1680f7d864190fcbdd5e437683a23','relayRevision':'77848f1fa52fa2c44ae622e0809a74501616e28a','productionQualified':False})
(proof/'README.md').write_text('''# V33 terminal failure — 7 October 2026

Original controller session5043 terminal exit1. Preflight passed23:42:30.689 UTC; VPS started23:42:31.902. IPC readiness deadline failed23:45:32.393, terminal23:45:53.978. VPS stopped137 after the unchanged20-second stop grace, OOMKilledfalse. No Mac native/Docker writer started, no stop receipt, no verified duties or new common finality. No restart launched. Clock startup catch-up completion was logged, but IPC readiness was never observed. This does not establish which startup phase consumed the full time or why shutdown failed to complete within its grace.

Core df31cd48, relay77848f1f, controller checkout22d73d57 and all controller file hashes retained. This run never reached the900-second three-common-checkpoint test. The immutable block-inventory correction remains locally qualified only; it has not resolved WAN finality by this evidence. The180-second startup limit and all budgets were not widened.

Next required work: independently audit the forced-stop database and signing journals/witnesses read-only, then identify and correct startup/recovery cost before another writer. V32 and earlier failures remain historical evidence. All production/Base–Nano/Doppler/independent-review gates remain open. No funds, sale or public deployment.
''')
save(proof/'manifest.json',{'sourceRevision':'22d73d5702bd6a239eb1b5ad98202085b4860874','result':'FAILED','productionQualified':False,'files':{str(p.relative_to(proof)):sha(p) for p in sorted(proof.rglob('*')) if p.is_file()}})
msg='V33 terminée FAILED, processus original5043/1 : précontrôle passé, mais disponibilité IPC du VPS absente dans180s. Arrêt VPS137 après20s de grâce, sans OOM ; aucun nœud Mac lancé, aucune duty ni finalité nouvelle démontrée, aucun reçu d’arrêt. Catch-up initial journalisé, phase coûteuse encore à identifier. [Preuves terminales](qualification/restored-three-node-v33-terminal-20261007/README.md). Audit après arrêt forcé lancé en lecture seule ; aucune relance ni élargissement du délai. Correction d’inventaire df31cd48 seulement qualifiée localement. Tous critères production et Base–Nano/Doppler demeurent ouverts.'
for name in ['PRODUCTION-GOAL.md','VALIDATOR-EIP-ROADMAP.md','EVM-PRODUCTION-EIP-ROADMAP.md','EIP-IMPLEMENTATION-STATUS.md']:
 p=repo/'docs'/name;s=p.read_text();needle='## État courant vérifié — 7 octobre 2026\n\n';assert needle in s;p.write_text(s.replace(needle,needle+msg+'\n\nÉtat précédent conservé : ',1))
print('V33 failure and exact original exit archived; controller unchanged; no restart')
