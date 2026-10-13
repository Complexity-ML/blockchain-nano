#!/usr/bin/env python3
"""Copy an explicit PUBLIC allowlist and propose docs; no compiler/test/network/DB APIs."""
from pathlib import Path
import os, stat, json, hashlib, difflib, xml.etree.ElementTree as ET
E=Path('/Users/boris/Documents/Codex/2026-09-20/users-boris-dev-blockchain-python/qualification-resume-20261012')
BASE=E/'checkpoint-blob-gc-amd64-addendum-proposal-20261013'
OUT=BASE/'final-six-pass-natural-open'
REPO=Path('/Users/boris/Dev/blockchain-nano')
ACT=E/'checkpoint-blob-gc-amd64-actual-results-20261013'
FOUND=BASE/'preliminary-build-and-run01/package'
FOUND_SHA='2c25d163bb3a559a72d8cb7a0d0265fb0472a2bbfcde262c3fca6a5249e5b3ae'
BASELINE_SHA='4f7adfffbb0ff0e47b8bebec5978eea0b10461787a981f25f4794a9f88b99b61'
TRANSFER2_SHA='36b28caacf5afcfa4981b7e291812effc186a2f27d10c67f6587316ea5e43146'
REV='f9d078d0d45d6081c8c8745a80575d898335ec3e'
def sha(b): return hashlib.sha256(b).hexdigest()
def dump(o): return (json.dumps(o,indent=2,ensure_ascii=False)+'\n').encode()
def ident(s): return tuple(getattr(s,k) for k in ('st_dev','st_ino','st_uid','st_gid','st_mode','st_size','st_mtime_ns','st_ctime_ns'))
reads={}
def read(p,expected=None,size=None):
 p=Path(p)
 if p.is_symlink() or p.resolve()!=p.absolute(): raise ValueError('noncanonical/symlink public input')
 fd=os.open(p,os.O_RDONLY|os.O_NOFOLLOW)
 try:
  before=os.fstat(fd)
  if not stat.S_ISREG(before.st_mode): raise ValueError('nonregular public input')
  chunks=[]
  while True:
   b=os.read(fd,65536)
   if not b: break
   chunks.append(b)
  raw=b''.join(chunks);after=os.fstat(fd)
  if ident(before)!=ident(after) or ident(after)!=ident(os.stat(p,follow_symlinks=False)): raise ValueError('public input identity changed')
 finally: os.close(fd)
 if expected and sha(raw)!=expected: raise ValueError('public input SHA mismatch: '+p.name)
 if size is not None and len(raw)!=size: raise ValueError('public input size mismatch')
 reads[str(p)]={'sha256':sha(raw),'bytes':len(raw)}
 return raw
foundation_raw=read(FOUND/'manifest.json',FOUND_SHA);foundation=json.loads(foundation_raw)
baseline_raw=read(BASE/'baseline-f9-document-preservation.json',BASELINE_SHA);baseline=json.loads(baseline_raw)
t2raw=read(ACT/'tests-root-dispatched-02/public-transfer-receipt.json',TRANSFER2_SHA);t2=json.loads(t2raw)
if t2.get('privateDataIncluded') is not False: raise ValueError('transfer must be public')
idx={x['name']:x for x in t2['files']}
if len(idx)!=11: raise ValueError('unexpected run02 public transfer scope')
run2bytes={n:read(ACT/'tests-root-dispatched-02'/n,r['sha256'],r['bytes']) for n,r in idx.items()}
# Verify raw transferred compiler metadata on the Mac; emit only a compact derived public binding.
b2=json.loads(run2bytes['before.json']);a2=json.loads(run2bytes['after.json'])
b1raw=read(ACT/'tests-root-dispatched-01/before.json');b1=json.loads(b1raw)
common=['source','roots','artifacts']
if b1['gitCommit']!=REV or b2['gitCommit']!=REV: raise ValueError('wrong source revision')
for k in common:
 if b1[k]!=b2[k] or b2[k]!=a2[k]: raise ValueError('actual artifact/source binding drift '+k)
if b1['metadata']!=b2['metadata']: raise ValueError('build metadata changed between runs')
if b2['fixtures']!=a2['fixtures']: raise ValueError('runtime fixture changed')
if set(b2['metadata'])!=set(a2['metadata']) or len(b2['metadata'])!=8: raise ValueError('wrong target metadata set')
for n,row in b2['metadata'].items():
 if set(a2['metadata'][n])!={'flagsSha256','linkSha256'}: raise ValueError('unexpected terminal metadata shape')
 for typ in ['flags','link']:
  if sha(row[typ+'Text'].encode())!=row[typ+'Sha256'] or a2['metadata'][n][typ+'Sha256']!=row[typ+'Sha256']: raise ValueError('flags/link hash mismatch')
if not a2['allBindingsUnchanged'] or not a2['ctestGraphSourceAndCopyUnchanged']: raise ValueError('controller invariant false')
term2=json.loads(run2bytes['execution-terminal.json']);res2=json.loads(run2bytes['result.json'])
if term2['exitCode']!=8 or term2['timedOut'] or not term2['bindingInvariantsHeld'] or not term2['containerAbsentAfterCleanup']: raise ValueError('unexpected terminal scope')
if res2['passedWithinTestScope'] or res2['singleSevenTestExecution']: raise ValueError('incorrect green-run promotion')
# JUnit contains real CTest status="fail" without a failure element; do not invent JUnit.
def junit_rows(raw):
 rows={}
 for c in ET.fromstring(raw).iter('testcase'):
  n=c.attrib['name'];status=c.attrib.get('status')
  if n in rows or c.find('skipped') is not None: raise ValueError('duplicate/skipped testcase')
  if status not in ('run','fail'): raise ValueError('unexpected real JUnit status')
  rows[n]={'status':'PASS' if status=='run' and c.find('failure') is None and c.find('error') is None else 'FAIL','elapsedSeconds':float(c.attrib['time'])}
 return rows
run1JUnit=read(FOUND/'tests/run01-junit.xml','ae17839baf82708d3e90878656273d85cf7fa9f29f1e01b54140426ee80ac601')
r1=junit_rows(run1JUnit);r2=junit_rows(run2bytes['testing/gc-selected.junit.xml'])
priorPass={'consensus_capella_duty_projection_availability','storage_checkpoint_immutable_blobs'}
newPass={'consensus_capella_fragment_checkpoint_migration','storage_capella_blob_gc_next_publication','storage_capella_blob_gc_cli_closed_fixture','storage_capella_blob_gc_adversarial_boundaries'}
natural='storage_capella_blob_gc_natural_pruning'
if {n for n,r in r1.items() if r['status']=='PASS'}!=priorPass or len(r1)!=7: raise ValueError('wrong run01 outcome')
if {n for n,r in r2.items() if r['status']=='PASS'}!=newPass or set(r2)!=newPass|{natural}: raise ValueError('wrong run02 outcome')
log2=run2bytes['ctest.log'].decode();objects=[]
for line in log2.splitlines():
 if '{' in line:
  try: objects.append(json.loads(line[line.index('{'):]))
  except ValueError: pass
by=lambda name:[o for o in objects if o.get('scenario')==name]
cli=by('checkpoint-gc-cli-closed-staged')
adverse=by('checkpoint-gc-adversarial-boundaries')
phase=by('gc-crash-child-phase')
if len(cli)!=1 or not cli[0]['actualCliCompleted'] or cli[0]['cliExitCode']!=0 or cli[0]['receiptMode']!=0o600 or cli[0]['receiptLinks']!=1: raise ValueError('CLI observation missing')
if sha(cli[0]['receiptBytes'].encode())!=cli[0]['receiptSha256'] or len(cli[0]['refusals'])!=5: raise ValueError('CLI receipt/refusal mismatch')
if any(r['exitCode']!=1 or r['newReceiptPublished'] or not r['logicalCopyContentsUnchanged'] for r in cli[0]['refusals']): raise ValueError('CLI refusal false')
if cli[0]['deletedPrimaryRecords']!=1 or cli[0]['deletedWitnessRecords']!=1: raise ValueError('wrong staged deletion count')
if len(adverse)!=1 or len(set(adverse[0]['passedBoundaries']))!=30: raise ValueError('adverse boundaries missing')
for k in ['primary-sync-failure-close-new-plan','witness-sync-failure-close-new-plan','sigkill-after-primary-before-witness-new-plan-reopen']:
 if k not in adverse[0]['passedBoundaries']: raise ValueError('required adverse boundary missing')
if not any(o.get('phase')=='wal-change-logical-check' and o.get('primaryOrphanAbsent') and o.get('witnessOrphanPresent') for o in phase): raise ValueError('actual between-store crash phase absent')
progress=[o for o in objects if 'progressSlot' in o]
if not progress or max(o['progressSlot'] for o in progress)!=16 or '***Timeout 180.03 sec' not in log2: raise ValueError('natural failure evidence absent')
# All chosen source files are explicit PUBLIC synthetic/compiler records; no directory recursion.
if OUT.exists(): raise ValueError('fresh destination required; retain existing outputs')
OUT.mkdir(mode=0o700);pkg=OUT/'package';pkg.mkdir(mode=0o700)
files=[]
def put(rel,raw,source=None):
 p=pkg/rel
 if p.exists(): raise ValueError('no overwrite')
 p.parent.mkdir(parents=True,exist_ok=True)
 fd=os.open(p,os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,0o644)
 try:
  with os.fdopen(fd,'wb') as f: f.write(raw);f.flush();os.fsync(f.fileno())
 except: raise
 row={'path':rel,'sha256':sha(raw),'bytes':len(raw)}
 if source:row['sourcePath']=str(source)
 files.append(row)
for row in foundation['files']:
 put('foundation/'+row['path'],read(FOUND/row['path'],row['sha256'],row['bytes']),FOUND/row['path'])
put('history/preliminary-foundation-manifest.json',foundation_raw,FOUND/'manifest.json')
for n in ['package-preparation-refusal.json','package-preparation-schema-refusal.json']:
 p=FOUND.parent/n;put('history/'+n,read(p),p)
for n in ['execution-terminal.json','result.json','ctest.log','ctest.command.json','selected-ctest-graph.json','events.jsonl','testing/gc-selected.junit.xml','admission-ctime-diagnostics.json']:
 rel='run02/'+('junit.xml' if n.startswith('testing/') else n);put(rel,run2bytes[n],ACT/'tests-root-dispatched-02'/n)
put('run02/public-transfer-receipt.json',t2raw,ACT/'tests-root-dispatched-02/public-transfer-receipt.json')
recipe=E/'checkpoint-blob-gc-amd64-five-retry-controller-20261013'
for src,dst,expected in [
 ('run-amd64-five-retries-sibling-fixture.py','recipe/five-retry-runner.py','3855656671eaf07e8b229e8caae3345be12d924ec21740dc467fa22b9245d95f'),
 ('audit-amd64-five-retries-plus-two-prior.py','recipe/five-retry-auditor.py','c54750a331569a4b191ac5fb1d2f6574fe0a77952a3ce13e8d032a55acf8ee8b'),
 ('prior-public-bindings.json','recipe/prior-public-bindings.json','3221c362c901e32a569727ea5febb15ee4d1a5f6124b6e40e5c5a63c1c132776')]:
 put(dst,read(recipe/src,expected),recipe/src)
fixture=E/'root-owned-amd64-seven-test-runner-v2-20261013/runtime-fixture-manifest.json'
put('recipe/runtime-fixture-manifest.json',read(fixture,'ae9bb195e095cd5224d39782b2b88bae936c07f36ec51cb11baf5925c2d3ead2'),fixture)
partial={'format':'nano-gc-amd64-partial-observed-result-v1','revision':REV,'buildTerminalSha256':b2['buildTerminalSha256'],'publicSourceReadOnlyAnalysis':True,'actualTestsExecutedByThisCollector':0,'run01':{'selected':7,'ctestExitCode':8,'cases':r1,'reasonForFiveRefusals':'Missing runtime fixture minimal-phase0.yaml before scenarios'},'run02':{'selected':5,'ctestExitCode':8,'cases':r2,'naturalFailure':'CTest per-test timeout180.027seconds','lastLoggedNaturalProgress':progress[-1],'actualFinalProcessedSlotEstablished':False,'naturalGcSlot40Observed':False,'finalSlot56AndColdReopenObserved':False,'timeoutCauseMeasured':False},'uniquePassCount':6,'uniqueSelectedCount':7,'singleSevenGreenExecution':False,'sevenTestsQualified':False,'armNaturalResultReused':False,'run01PassesNotRepeated':sorted(priorPass),'actualBuildBindingsUnchangedBetweenRuns':True,'sourceAndArtifactCompleteMetadataUnchangedDuringRun02':True,'flagsAndLinkHashesUnchangedAllEightTargets':True,'rawTerminalMetadataShapeDiffersByOmittedBeforeTextsOnly':True,'runtimeFixtureBeforeAfterUnchanged':True,'run02ContainerAbsentAfterCleanup':True,'scope':{'adverseUniqueCases':30,'twoFsyncCasesObserved':True,'actualBetweenStoresLogicalCrashObservation':True,'cliPrimaryDeletedRecords':1,'cliWitnessDeletedRecords':1,'cliRefusals':5,'cliReceiptMode':'0600','cliReceiptLinks':1,'runtimeFixtureCount':len(b2['fixtures']['files'])},'evidenceReferences':{'beforeRun01':{'sha256':sha(b1raw),'bytes':len(b1raw)},'beforeRun02':idx['before.json'],'afterRun02':idx['after.json']},'open':['AMD64 natural56-slot GC/cold-reopen within180s','Native GC qualification','Actual runtime image and installation','Resumed duties and common finality','Independent security review and all prior production criteria'],'privateInputsRead':False,'pendingFourVpsProofsExported':False,'dbOrKeyOrJournalRead':False,'runtimeDutiesResumed':False,'productionQualified':False}
put('partial-result.json',dump(partial))
README='''# GC AMD64 — compilation réussie, qualification partielle\n\nSource `f9d078d0`. Build réel : 41 unités cœur fraîches, 99 réutilisées, archive de 140 membres et 8 consommateurs/liens. La compilation ne qualifie pas la production.\n\nDeux runs CTest distincts sont conservés avec leur sortie 8 : run01 comporte 2 PASS et 5 refus avant scénario (fixture manquante) ; run02 exécute seulement ces5 cas et obtient 4 PASS et 1 timeout naturel de 180,027 s. Cela fait 6 tests distincts réussis sur 7, sans run global vert ni réutilisation du naturel ARM. Le dernier jalon naturel imprimé est le slot 16/finalizedEpoch 0 ; le dernier slot réellement traité et la cause du timeout ne sont pas établis. GC au slot 40, fin au slot 56 et réouverture froide naturelle AMD restent ouverts.\n\nLes logs/JUnit sont les fichiers réels : aucun JUnit n’est fabriqué, aucun résultat de l’auditeur composite7PASS n’est revendiqué. `partial-result.json` provient uniquement des lectures/hash/stat de ces preuves publiques et des comparaisons avant/après. Les originaux volumineux avant/après sont référencés par SHA et conservés dans le workspace. Les flags/link après omettent les textes présents avant ; leurs 16 SHA restent exacts.\n\nLa CLI a réellement supprimé 1 blob primaire et 1 témoin dans une fixture synthétique fermée, vérifié 5 refus et un reçu 0600/nlink1. Le test adverse fournit 30 cas, 2 fsync et l’observation de suppression primaire avant témoin. Ce ne sont ni des bases de campagne ni une panne matérielle.\n\n`foundation/` est une copie byte-exact du paquet préliminaire de 21 fichiers, dont le manifeste original et les refus du collecteur restent séparés. Ses anciens statuts d’attente demeurent historiques ; le présent manifeste décrit le résultat actuel. Les refus V2 aliases, V4 header et run01 fixtures restent conservés. La CI GitHub f9 n’a exécuté aucune étape (refus de facturation), le code a été livré par Git mais aucun runtime n’a été mis à jour.\n\nAucune image runtime/installation, clé/env, capsule privée, DB ou preuve privée VPS en attente n’est incluse. La relation bridge/token, la reprise des duties, la finalité commune et les autres critères de production restent ouverts. Aucun compiler/test/Docker/SSH/runtime n’a été exécuté par ce collecteur.\n'''
put('README.md',README.encode())
# Public copier source documents the exact selection, not a test or generated authority.
put('recipe/prepare-final-public-addendum.py',read(Path(__file__)),Path(__file__))
manifest={'format':'nano-gc-amd64-six-distinct-pass-natural-open-public-v1','status':'PARTIAL_AMD64_NATURAL_TIMEOUT_OPEN','revision':REV,'foundationManifestSha256':FOUND_SHA,'files':files,'payloadCount':len(files),'payloadBytes':sum(x['bytes'] for x in files),'scope':{'buildPassed':True,'uniquePasses':6,'selectedUniqueTests':7,'actualCtestExecutions':2,'ctestExitCodes':[8,8],'singleSevenGreenExecution':False,'amdNaturalGcQualified':False,'runtimeImageBuilt':False,'installed':False,'dutiesResumed':False,'productionQualified':False,'privateInputsRead':False,'fourPendingPrivateVpsProofsIncluded':False},'excludedVoluminousPublicRecords':[{'path':str(ACT/'tests-root-dispatched-01/before.json'),'sha256':sha(b1raw),'bytes':len(b1raw)},*({'path':str(ACT/'tests-root-dispatched-02'/n),'sha256':idx[n]['sha256'],'bytes':idx[n]['bytes']} for n in ['before.json','after.json'])]}
(pkg/'manifest.json').write_bytes(dump(manifest))
# Prefix current complete docs; do not change historical body or EIP table/CSV statuses.
prefix='''## 13 octobre 2026 — AMD64 : build réussi, naturel GC toujours ouvert\n\nLa [preuve publique AMD64](qualification/checkpoint-blob-gc-amd64-20261013/README.md) lie la source `f9d078d0`, 41 unités cœur fraîches et 99 réutilisées, archive de 140 membres et 8 consommateurs/liens. Les binaires/source/dépendances restent inchangés pendant les tests ; aucune image runtime ni installation réelle n’est qualifiée.\n\n**Qualification partielle : 6 tests distincts PASS sur 7, dans2 runs CTest tous deux en sortie 8.** Run01 : disponibilité (3 sous-scénarios bornés, provider 0, max 32 901 ms/32 512 KiB/144 signatures synthétiques) et blobs immuables passent ; les 5 autres cas refusent une fixture runtime absente avant scénario. Après ajout RO des 23 fixtures publiques, seuls ces 5 cas sont rejoués : migration, GC API, CLI et 30 frontières adverses passent ; le naturel dépasse 180 secondes. Le dernier jalon imprimé est le slot 16, finalizedEpoch 0 ; ni GC au slot 40, ni fin au slot 56/réouverture froide, ni cause du timeout ne sont démontrés sur AMD64. Le résultat naturel ARM reste distinct.\n\nLes refus préparatoires V2 aliases et V4 header, run01 fixtures et run02 timeout restent conservés. La CI f9 refuse la facturation sans aucune étape exécutée ; la livraison Git Mac/GitHub/VPS n’est pas un déploiement runtime. Aucun statut EIP n’est promu. Le scénario naturel AMD64, la qualification GC native, la livraison effective de l’image/install, la reprise des duties/finalité commune et tous les critères précédents restent ouverts. Les 4 preuves privées VPS attendent une autorisation dédiée, distinctement du V37 FAILED et des journaux Mac manquants.\n\n'''
props=OUT/'document-proposals';props.mkdir();patch=[];preserved=[]
for row in baseline['documents']:
 old=read(REPO/row['path'],row['sha256'],row['bytes']);new=prefix.encode()+old
 dst=props/row['path'];dst.parent.mkdir(parents=True,exist_ok=True);dst.write_bytes(new)
 if new[len(prefix.encode()):]!=old:raise ValueError('historical body changed')
 patch.extend(difflib.unified_diff(old.decode().splitlines(keepends=True),new.decode().splitlines(keepends=True),fromfile='a/'+row['path'],tofile='b/'+row['path']))
 preserved.append({'path':row['path'],'oldSha256':row['sha256'],'oldBytes':len(old),'newSha256':sha(new),'newBytes':len(new),'prefixBytes':len(prefix.encode()),'wholeOldBodyByteExactSuffix':True})
csvs=[]
for row in baseline['csvMatrices']:
 old=read(REPO/row['path'],row['sha256'],row['bytes']);csvs.append({'path':row['path'],'sha256':sha(old),'bytes':len(old),'unchanged':True})
(props/'amd64-docs.patch').write_text(''.join(patch));(props/'prefix.md').write_text(prefix)
preservation={'format':'nano-amd-six-pass-document-preservation-v1','baseRevision':REV,'baselineManifestSha256':BASELINE_SHA,'documents':preserved,'csvMatrices':csvs,'allHistoricalCriteriaPreserved':True,'eipStatusesChanged':False,'repoEdited':False,'pendingPrivateVpsAuthorizationPreserved':True,'packageManifestSha256':sha(dump(manifest))}
(props/'document-preservation.json').write_bytes(dump(preservation))
top={'format':'nano-amd-six-pass-public-and-doc-proposal-v1','status':'PARTIAL_NATURAL_TIMEOUT_OPEN','evidenceManifest':{'path':'package/manifest.json','sha256':sha(dump(manifest)),'payloadCount':len(files),'payloadBytes':manifest['payloadBytes']},'documentPreservation':{'path':'document-proposals/document-preservation.json','sha256':sha(dump(preservation))},'documentPatch':{'path':'document-proposals/amd64-docs.patch','sha256':sha(''.join(patch).encode())},'originalFoundationRetainedUnchanged':True,'repoMutation':False,'compilerOrTestOrNetworkExecutionByCollector':False}
(OUT/'final-delivery-index.json').write_bytes(dump(top))
print(json.dumps({'output':str(OUT),'packageManifestSha256':sha(dump(manifest)),'payloadCount':len(files),'payloadBytes':manifest['payloadBytes'],'preservationSha256':sha(dump(preservation)),'patchSha256':sha(''.join(patch).encode()),'topSha256':sha(dump(top))}))
