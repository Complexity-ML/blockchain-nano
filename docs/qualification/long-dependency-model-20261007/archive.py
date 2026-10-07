from pathlib import Path
import json,hashlib,shutil
r=Path(__file__).parent;repo=Path('/Users/boris/Dev/blockchain-nano');out=repo/'docs/qualification/long-dependency-model-20261007'
assert not out.exists()
cases=[]
for n in (513,935,2049):
 assert json.loads((r/f'actual-{n}-exit.json').read_text())['exitCode']==0
 x=json.loads((r/f'actual-{n}-result.json').read_text());assert x['payloadsAdmitted']==n and x['admissionQuantum']==8 and x['acknowledgedRoots']==1024
 cases.append({k:x[k] for k in ('chainLength','turns','exportsRequested','payloadsAdmitted')})
out.mkdir()
for p in r.iterdir():
 if p.is_file() and p.suffix in ('.json','.log','.cpp','.py'):shutil.copy2(p,out/p.name)
summary={'originalSession':91425,'exitCode':0,'terminalChunk':'d5af19','cases':cases,'productionQualified':False,'wanQualified':False,'globalCauseProven':False,'scope':'Fixed single ancestry, synthetic SSZ, trusted local mock exports and modeled parent-first receiver ACKs; empty historical catalog; real production relay headers. Not runtime timings, EVM, full peer scheduling, moving forks or WAN.'}
(out/'actual-summary.json').write_text(json.dumps(summary,indent=2)+'\n')
(out/'README.md').write_text('''# Bounded long dependency transport model\n\nActual V36 limits: quantum8, ACK1024, dependency256, export cache256/16MiB, catalogue page8/max1024. Original driver91425 ended0 (d5af19); compile/link and each case exit0 retained.\n\n| Ancestry blocks | Relay turns | Local export requests | Modeled admissions |\n| --- | --- | --- | --- |\n'''+''.join(f"| {x['chainLength']} | {x['turns']} | {x['exportsRequested']} | {x['payloadsAdmitted']} |\n" for x in cases)+'''\nThe fixture asserts parent-first sends, bounded admissions, exact complete acknowledgement and no false selected-head success. It uses actual relay headers with synthetic SSZ and mocked authenticated exports/receiver ACKs. Empty historical catalogue and a fixed single ancestry omit multi-peer, concurrent/moving forks, rejection/restart and real state transition. Counts are not WAN duration or global cause attribution. Native fixture compilation ran concurrently with V36: no resource-isolation claim. No active node/configuration/code/image was changed.\n\nInitial compile/link failure (`boost_system` missing), repaired link command, exploratory quantum32/ACK4096 cases and their historical summary are preserved. Exploratory cases are not V36 settings; additional exploratory exits were captured only in aggregate. `actual-summary.json` contains the V36-parameter results; this is a transport model, not production qualification.\n''')
headers=['network/capella_head_catalog_relay.hpp','network/capella_block_delivery.hpp','network/capella_export_cache.hpp']
(out/'manifest.json').write_text(json.dumps({'sourceRevision':'9eb636b3c36349d6a7ec96c8c013344d5c1e53ea','sourceHeaders':{f:hashlib.sha256((repo/f).read_bytes()).hexdigest() for f in headers},'files':{str(p.relative_to(out)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(out.iterdir()) if p.is_file()}},indent=2)+'\n')
print(json.dumps(summary))
