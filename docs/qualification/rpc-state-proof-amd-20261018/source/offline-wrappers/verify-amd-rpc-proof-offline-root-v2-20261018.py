"""Root offline continuation: reuse56 public files; verify24 vectors once; no SSH/C++."""
from pathlib import Path
import ast,hashlib,json,sys,time
E=Path(__file__).parent.resolve()
B=E/'rpc-candidate-amd-continuation-public-audit-root-20261018-02'
O=E/'rpc-candidate-amd-offline-proof-audit-root-20261018-01'
OWN=Path(__file__).resolve(strict=True)
assert len(sys.argv)==3
OWN_PIN={'sha256':sys.argv[1],'bytes':int(sys.argv[2])}
own_raw=OWN.read_bytes();assert len(OWN_PIN['sha256'])==64 and len(own_raw)==OWN_PIN['bytes']and hashlib.sha256(own_raw).hexdigest()==OWN_PIN['sha256']
H=E/'amd-rpc-continuation-audit-source-v2-20261018/collect.py'
HP={'sha256':'62defdf144e44060e4af97933c220fc909a4840396e64f23ef9a7cb441adbe16','bytes':29806}
CAT={'sha256':'22a28aad1911b51c1c2b4a8a2955be6846d2ff2fa4c9ff83f41646862ff3f8b6','bytes':25476}
FAILED={'sha256':'28066da7b764245cf78d0921655aab08b984c6215c817bdb9f856c31b77697f9','bytes':11653}
DP={'sha256':'2344b99faf5630d6de65b52fada80735c746b3af3e4efb593193b98ce1c55497','bytes':206}
raw=H.read_bytes();assert H.resolve(strict=True)==H and len(raw)==HP['bytes']and hashlib.sha256(raw).hexdigest()==HP['sha256']
ns={'__name__':'frozen_public_audit_helpers','__file__':str(H)}
exec(compile(raw,str(H),'exec'),ns) # frozen declarations only; guarded main never invoked
read,need,loads,save=ns['read'],ns['need'],ns['loads'],ns['save']
ns['O']=O;O.mkdir(mode=0o700,exist_ok=False)
UNIT=ns['UNIT'];MODULE=ns['MODULE'];DRIVER=ns['DRIVER'];NODE=ns['NODE'];R=ns['R']
held={};pending={};dispatch=None;current_input=None;first=None;failures=[];parsed=None;event=None;mechanical=False;old_count=0;new_count=0;observations=[]
def fail(stage,error):
 global first
 failures.append({'stage':stage,'errorType':type(error).__name__,'message':str(error),'currentInput':None if current_input is None else str(current_input)})
 if first is None:first=error
def known(p,want):
 global current_input
 p=Path(p);current_input=p;need(type(want['sha256'])is str and len(want['sha256'])==64 and all(c in '0123456789abcdef'for c in want['sha256'])and type(want['bytes'])is int and want['bytes']>=0,'complete known byte pin');row,body=read(p,held.get(str(p)),bound=want['bytes']);need(ns['pin'](row)==want,'known byte pin: '+str(p))
 if str(p)not in held:
  need(p not in pending or pending[p]==want,'one known pin per new input');pending[p]=want
 return body
try:
 known(OWN,OWN_PIN)
 auth=UNIT/'new-input-admission.json'
 old=loads(known(auth,ns['UNIT_AUTHORITY_PIN']))['samples'][-1]
 need(type(old)is dict and str(NODE)in old and len(old)==6,'six exact original authority rows')
 for path,row in old.items():
  need(ns['valid_row'](row),'original row shape');read(Path(path),row,body=False);held[path]=row
 old_count=len(held)
 public=loads(known(UNIT/'result-public.json',{'sha256':'44e6950c127151fc4a5ad9ab620e81c336ae2bc495853c7ff856e00634491a55','bytes':1996}))
 correction=loads(known(UNIT/'terminal-audit-correction-public.json',{'sha256':'dd2e4544e8936932cd402b664c04a5c62de1e4bbe9c557c5087461dff09fa0ee','bytes':1592}))
 need(correction['passedEight']is True and correction['testsReplayed']is False and correction['nodeTestExitCode']==0,'existing eight units retained, never replayed')
 origins={'tests/qualification/rpc-proof.mjs':E/'rpc-eip1186-account-storage-proof-source-proposal-20261017/source/tests/qualification/rpc-proof.mjs','tests/qualification/rpc-proof.test.mjs':E/'rpc-eip1186-account-storage-proof-source-proposal-20261017/source/tests/qualification/rpc-proof.test.mjs','tests/qualification/account-proof.mjs':R/'tests/qualification/account-proof.mjs','tests/hardhat/package.json':R/'tests/hardhat/package.json'}
 for rel,p in origins.items():need(public['newSourcePins'][rel]==old[str(p)],'source origin full9 joined to its real path')
 catalog=loads(known(B/'readback-catalog-public.json',CAT))
 need(catalog['privateFilesTransferred']==0 and catalog['readbackRowsAreHistoricalAuthorities']is False,'public readback scope')
 files=catalog['files'];need(set(files)=={p for p,_ in ns['ALLOWLIST']}and len(files)==56,'same56 public files')
 aggregate=0
 for rel,bound in ns['ALLOWLIST']:
  p=files[rel];need(type(p['bytes'])is int and 0<=p['bytes']<=bound,'allowlist file size')
  known(B/rel,{'sha256':p['sha256'],'bytes':p['bytes']});aggregate+=p['bytes']
 need(aggregate<=ns['AGGREGATE'],'aggregate public cap')
 previous=loads(known(B/'root-audit-public.json',FAILED))
 need(previous['passed']is False and previous['allowlistPublicFiles']==56 and previous['verifiedProofVectors']==0 and previous['selectedInputsUnchanged']is True,'failed collector remains failed')
 dispatch=loads(known(ns['D']/'dispatch-exit-public.json',DP))
 need(dispatch['actualProcessTerminal']is True and type(dispatch['exitCode'])is int and dispatch['exitCode']==0,'actual CPP continuation terminal0')
 old_root=E/'rpc-candidate-amd-terminal-failure-root-20261018-01'
 old_audit=loads(known(old_root/'root-failure-audit-public.json',{'sha256':'e1b1f0d01107b64c14559afda379ee538eb6b2e27b174020b0bdc72f80a52ea7','bytes':2054}))
 retained_proof=loads(known(old_root/'outputs/core-member-proof.json',{'sha256':'acabca41c17b518fa2a82dcf8ec7751dab9358344769dd812b53c7b5a43e17cb','bytes':8779}))
 need(old_audit['actualDispatcherExitCode']==1 and old_audit['failureType']=='TimeoutExpired'and old_audit['hostPassed']is False,'failed CPP parent preserved')
 batch=loads(known(E/'rpc-state-proof-test-oracles-source-20261017/test-batch.json',{'sha256':'17231520964bd3b3e039bff96a5b46f6024c64af853767f9d909779545e4c802','bytes':5545}))
 runtime=loads(known(E/'native-rpc-state-proof-controller-source-v3-20261017/runtime-cases.json',{'sha256':'aa613ce2e1980a8f73aaf886be81ac5c3c0a1d9edf72cc5749f97ac61b68e068','bytes':7038}))
 for p,want in [(H,HP),(DRIVER,{'sha256':'94b5a31b2bd404e600a9c09f5d261d109b1c13e789820ba67fe22d5b738b7f3f','bytes':3291}),(MODULE,{'sha256':'b59224b4a64231d493ccf21b98d4c5ddb5f5ce3ec9889b792190d0e0e0b090ce','bytes':5340}),(MODULE.parent/'account-proof.mjs',{'sha256':'2a07c2f6d0602250e85a71b563161fbbaafd5c8cac7d612f3c3c8c02909d31c9','bytes':2439}),(MODULE.parent.parent/'hardhat/package.json',{'sha256':'39a64e23c4cde8a0a0c3db20da85f0ac5c5e4508d9fe118c8a8c42fd0d1a2e8d','bytes':550})]:known(p,want)
 # Pending inputs have never been admitted. Every byte pin stays strict;
 # only valid new metadata may settle, bounded30s. Original six rows never reset.
 started=time.monotonic();last=None;stable=0;since=None
 while True:
  rows={};now=time.monotonic();need(now-started<30,'new input admission deadline')
  for path,row in held.items():current_input=Path(path);read(Path(path),row,body=False)
  for p,want in pending.items():
   current_input=p
   row,_=read(p,bound=want['bytes'],body=False);need(ns['pin'](row)==want,'new input known bytes');rows[str(p)]=row
  now=time.monotonic()
  changes=[]if last is None else[{'path':p,'fields':[k for k in rows[p]if rows[p][k]!=last[p][k]]}for p in rows if rows[p]!=last[p]]
  if rows==last:stable+=1
  else:stable=1;since=now
  observation={'elapsedMs':round((now-started)*1000),'stableSamples':stable,'stableSpanMs':round((now-since)*1000),'rows':rows,'metadataDifferencesFromPrevious':changes,'knownBytePinsUnchanged':True,'originalSixRowsRefreshed':False}
  observations.append(observation);save('admission-observation-'+str(len(observations))+'.private.json',observation)
  need(now-started<30,'new input admission observation deadline')
  if stable>=3 and now-since>=2:
   for p,row in rows.items():need(p not in held,'new input cannot replace an old row');held[p]=row
   new_count=len(rows);break
  last=rows;time.sleep(1.05)
 save('admission-summary-public.json',{'passed':True,'observations':len(observations),'oldRows':old_count,'newInputRows':new_count,'oldRowsRefreshed':False,'historicalProjectionFd9Claimed':False})
 # Re-evaluate the exact previously reviewed pure56-file validation block.
 # It contains reports, case joins and stream oracles, no process launch/write.
 tree=ast.parse(raw);main=next(n for n in tree.body if isinstance(n,ast.FunctionDef)and n.name=='main');body=next(n for n in main.body if isinstance(n,ast.Try)).body
 def assigns(node,name):return isinstance(node,ast.Assign)and any(isinstance(t,ast.Name)and t.id==name for t in node.targets)
 start=next(i for i,n in enumerate(body)if assigns(n,'host'));end=next(i for i,n in enumerate(body)if assigns(n,'local_pins'))
 validation=ast.Module(body=body[start:end],type_ignores=[])
 ns.update(O=B,batch=batch,runtime_policies=runtime,retained_proof=retained_proof)
 try:exec(compile(validation,'<frozen-pure56-file-audit>','exec'),ns)
 finally:ns['O']=O
 mechanical=True
 proof=B/'outputs/test-candidate-proof.stdout.log';proof_pin={'sha256':files['outputs/test-candidate-proof.stdout.log']['sha256'],'bytes':files['outputs/test-candidate-proof.stdout.log']['bytes']}
 argv=[str(NODE),str(DRIVER),str(proof),proof_pin['sha256'],str(MODULE)]
 out,err,event=ns['bounded_process']('independent-proof',argv,timeout=60,stdout_limit=ns['AGGREGATE'],stderr_limit=1048576,env={'PATH':'/usr/bin:/bin','NODE_PATH':str(R/'tests/hardhat/node_modules')})
 need(event['exitCode']==0 and event['actualProcessJoined']is True and not err,'actual independent verifier terminal0');parsed=loads(out)
 need(parsed['format']=='nano-rpc-proof-vectors-independent-check-v1'and parsed['passed']is True and type(parsed['verifiedVectors'])is int and parsed['verifiedVectors']==24 and parsed['inputSha256']==proof_pin['sha256']and len(parsed['results'])==24 and parsed['consensusRootAuthenticated']is False and parsed['productionQualified']is False,'actual24 proof vectors')
except BaseException as error:fail('offline-proof-audit',error)
finally:
 ns['O']=O;diff=[]
 for path,row in held.items():
  try:read(Path(path),row,body=False)
  except BaseException as error:diff.append({'path':path,'errorType':type(error).__name__});fail('final-fence',error)
 try:save('final-input-fence-public.json',{'passed':not diff,'selectedFiles':len(held),'failures':diff,'originalRowsRefreshed':False})
 except BaseException as error:fail('final-fence-publication',error)
 passed=first is None and mechanical and parsed is not None and parsed.get('verifiedVectors')==24
 result={'format':'nano-amd-offline-proof-continuation-root-v1','passed':passed,'sourceRevision':ns['REV'],'actualHeadOfCppQualification':ns['HEAD'],'actualCppDispatcherExitCode':None if dispatch is None else dispatch.get('exitCode'),'mechanical56FileAuditRevalidatedOffline':mechanical,'verifiedProofVectors':24 if passed else 0,'verifierReportedVectors':None if parsed is None else parsed.get('verifiedVectors'),'verifierActualExitCode':None if event is None else event['exitCode'],'verifierActualProcessJoined':bool(event and event['actualProcessJoined']),'originalRowsRetained':old_count,'newInputRowsAdmitted':new_count,'allSelectedInputsUnchanged':not diff,'newSshReadbacks':0,'cppTestsReplayed':0,'oldJavascriptUnitsReplayed':0,'oldFailedCollectorsReclassified':False,'oldCppFailureReclassified':False,'projectionHistoricalFd9Claimed':False,'consensusRootAuthenticated':False,'runtimeDutiesResumed':False,'candidateImportedToMain':False,'productionQualified':False,'sourcePin':OWN_PIN,'failures':failures}
 if first is not None:result['firstFailureType']=type(first).__name__
 try:save('result-public.json',result);print(json.dumps(result),flush=True)
 except BaseException as error:
  if first is None:first=error
if first is not None:raise first
need(passed,'offline proof audit must pass')
