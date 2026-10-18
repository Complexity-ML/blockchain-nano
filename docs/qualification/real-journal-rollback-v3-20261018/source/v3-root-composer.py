"""Root only: collect bounded public terminal facts and prepare a byte-pinned plan."""
import base64,hashlib,json,os,pathlib,stat,subprocess
E=pathlib.Path(__file__).resolve().parent
R=pathlib.Path('/Users/boris/Dev/blockchain-nano')
P=E/'real-journal-rollback-source-proposal-20261018-03'
A=E/'real-journal-rollback-arm-source-proposal-20261018-02'
N=E/'native-c7-cmake-root-20261018-01'
ARM=E/'real-journal-rollback-arm-root-20261018-02'
O=E/'real-journal-v3-publication-root-20261018-01'
Q='docs/qualification/real-journal-rollback-v3-20261018/'
HEAD='86fa881a15fcd111fe8bb31cb32910decc9ddde6'
GIT='/Library/Developer/CommandLineTools/usr/bin/git'
F=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')
held={};items=[]
def need(ok,why):
 if not ok:raise RuntimeError(why)
def pin(b):return {'sha256':hashlib.sha256(b).hexdigest(),'bytes':len(b)}
def read(path):
 path=pathlib.Path(path);need(path.is_absolute()and path.resolve(strict=True)==path,'physical explicit input');fd=os.open(path,os.O_RDONLY|os.O_NOFOLLOW)
 try:
  a=os.fstat(fd);row={k:getattr(a,k)for k in F};need(stat.S_ISREG(a.st_mode)and a.st_nlink==1 and 0<=a.st_size<=8*1024*1024,'bounded regular input');raw=b''
  while part:=os.read(fd,65536):raw+=part;need(len(raw)<=8*1024*1024,'bounded input read')
  need(len(raw)==a.st_size and row=={k:getattr(os.fstat(fd),k)for k in F}=={k:getattr(path.lstat(),k)for k in F},'FD9/path read');row['sha256']=pin(raw)['sha256'];need(str(path)not in held or held[str(path)]==row,'no held row refresh');held.setdefault(str(path),row);return raw
 finally:os.close(fd)
def decoded(raw):
 def unique(pairs):
  d={}
  for k,v in pairs:need(k not in d,'duplicate JSON key');d[k]=v
  return d
 return json.loads(raw,object_pairs_hook=unique,parse_constant=lambda _:(_ for _ in()).throw(ValueError('nonfinite JSON')))
def document(path):return decoded(read(path))
def git(*args):
 p=subprocess.run([GIT,'--no-optional-locks','-c','core.fsmonitor=false',*args],cwd=R,capture_output=True,timeout=15);need(p.returncode==0 and not p.stderr,'read-only Git0');return p.stdout.decode().strip()
def write(path,raw):
 path.parent.mkdir(mode=0o700,parents=True,exist_ok=True);need(path.parent.resolve(strict=True)==path.parent and not os.path.lexists(path),'new physical payload');fd=os.open(path,os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,0o400)
 with os.fdopen(fd,'wb')as f:f.write(raw);f.flush();os.fsync(f.fileno())
def add(name,raw,mode='new',destination=None):
 destination=destination or Q+name;path=O/'payload'/destination;write(path,raw);item={'source':str(path),'destination':destination,'mode':mode,'input':pin(raw)}
 if mode!='new':
  old=read(R/destination);item['before']=pin(old);output=old+raw if mode=='append'else raw
 else:need(not os.path.lexists(R/destination),'new destination');output=raw
 item['after']=pin(output);items.append(item)
def copy(name,path):add(name,read(path))
def main():
 os.umask(0o077);need(git('branch','--show-current')=='main'and git('rev-parse','HEAD')==HEAD and not git('status','--porcelain'),'clean fixed parent');need(not os.path.lexists(O),'new publication namespace');O.mkdir(mode=0o700)
 old_cpp=read(R/'tests/qualification/beacon_journal_physical_rollback.cpp');cpp=read(P/'beacon_journal_physical_rollback.cpp')
 need(pin(old_cpp)=={'sha256':'b6760c6ddf98ec1cbd8968351e37155848d5eab752c4c11e2c0bf96b8bb901d6','bytes':24367}and pin(cpp)=={'sha256':'2e5ef451f13cca770cf02128bbb389eaae60379974252859829e197d14521acd','bytes':28801},'exact reviewed V2 and V3')
 n4=document(N/'native-successor-04/result-public.json');n5=document(N/'native-successor-05/result-public.json');need(n4['passed']is False and n5['passed']is False and n4['cases']['cohort']['passed']is True and n5['cases']['journal']['passed']is True,'preserve actual whole failures and CLI facts')
 need(n5['runtimeCliInvocations']==1 and len(n5['runtimeEvents'])==1 and n5['runtimeEvents'][0]['exitCode']==0 and n5['runtimeEvents'][0]['groupClosed']is True,'actual Native J CLI0 and group closed')
 packet_raw=read(P/'amd-v3-passive-collection.json');need(pin(packet_raw)=={'sha256':'95db5953c488fc6f778e43a552544ac60b4a1c4dbb5e15475baf628f3d8928e9','bytes':36680},'Root-collected public packet exact bytes');packet=decoded(packet_raw);audit=packet['audit'];need(audit['passed']is True and audit['originalSoftwareFilesUnchanged']==9 and audit['mmdDependencyCount']==482 and audit['publicFiles']==18 and audit['newCppTestInvocations']==0 and audit['heldFilesUnchanged']==33,'AMD passive exact scope');need(len(packet['exports'])==18,'exact public packet count')
 base={'result-public.json','host-final-fence-public.json','container-terminal-public.json','worker-start-attached.stderr.log'}
 extra={'outputs/result-public.json','outputs/RUN.stdout.log','outputs/RUN.stderr.log','outputs/run-root/commitments.json','outputs/loader-closure-public.json',*{'outputs/'+x+'.exit-public.json'for x in('M','CXX','LINK','LDD','RUN')}}
 allowed={'03/'+n for n in base}|{'04/'+n for n in base|extra};need(set(packet['exports'])==allowed,'closed eighteen public export paths; no controls/DB payload')
 for name,entry in sorted(packet['exports'].items()):
  parts=pathlib.PurePosixPath(name);need(parts.parts[0]in('03','04')and not parts.is_absolute()and '..'not in parts.parts,'bounded public AMD export path');raw=base64.b64decode(entry['base64'],validate=True);need(len(raw)<=2097152 and pin(raw)=={'sha256':entry['row']['sha256'],'bytes':entry['row']['st_size']},'exact export bytes');add('amd/'+name,raw)
 amd=decoded(base64.b64decode(packet['exports']['04/result-public.json']['base64'],validate=True));need(amd['passed']is True and amd['worker']['passed']is True and amd['worker']['counts']=={'independentM':1,'cppCompilations':1,'links':1,'loaderChecks':1,'runs':1}and amd['inputFinalFence']['passed']is True,'actual AMD host and worker PASS');add('amd/root-passive-audit.json',(json.dumps(audit,sort_keys=True,indent=2)+'\n').encode())
 need(amd['worker']['record']['elapsedMs']==6416 and amd['worker']['record']['actualPeakRssKiB']==102764 and amd['inputFinalFence']['heldFiles']==671 and amd['inputFinalFence']['testPythonProviderFiles']==628 and amd['inputFinalFence']['testPythonProviderDirectories']==51,'actual displayed AMD metrics')
 arm=document(ARM/'result-public.json');need(type(arm['passed'])is bool and arm['actualHead']==HEAD and arm['CoreRebuilt']is False and arm['imagesBuilt']==0 and arm['productionQualified']is False,'terminal ARM report source/scope')
 copy('arm/result-public.json',ARM/'result-public.json');copy('arm/host-final-fence-public.json',ARM/'host-final-fence-public.json')
 for name in ('result-public.json','RUN.stdout.log','RUN.stderr.log','RUN.exit-public.json','M.exit-public.json','CXX.exit-public.json','LINK.exit-public.json','LDD.exit-public.json','loader-closure-public.json'):
  if (ARM/'outputs'/name).exists():copy('arm/worker/'+name,ARM/'outputs'/name)
 copy('arm/worker/initial-unadmitted-priming-public.json',ARM/'outputs/initial-unadmitted-priming-public.json')
 arm_worker=document(ARM/'outputs/result-public.json');need(arm['passed']is False and all(arm_worker['counts'][k]==0 for k in ('independentM','cppCompilations','runs'))and arm_worker['firstFailure']['message']=='new guest eight fields/bytes/aliases unchanged','ARM02 actual preparation refusal; no C++ executed')
 for name in ('fixture-before-RUN-public.json','fixture-after-RUN-public.json','fixture-after-cleanup-public.json','source-before-public.json','source-after-cleanup-public.json'):
  if (ARM/name).exists():copy('arm/'+name,ARM/name)
 copy('arm/preparation-result-public.json',E/'real-journal-rollback-arm-control-root-20261018-02/preparation-result-public.json')
 for name in ('result-public.json','host-final-fence-public.json','outputs/result-public.json'):copy('arm/prior01/'+name,E/'real-journal-rollback-arm-root-20261018-01'/name)
 samples=[document(E/'real-journal-rollback-arm-root-20261018-01/outputs'/('initial-sample-'+str(i)+'.private.json'))for i in (1,2)];need(samples[0]['aliases']==samples[1]['aliases'],'prior01 alias diagnostic');changed=[]
 for path,row in samples[0]['rows'].items():
  after=samples[1]['rows'][path]
  if row!=after:
   need(path.startswith(('/old/','/retained/'))and row['bytes']==after['bytes']and row['sha256']==after['sha256']and all(a==b for i,(a,b)in enumerate(zip(row['materialGNU9'],after['materialGNU9']))if i not in(4,5)),'prior01 only observed bind owner differs');changed.append({'guestPath':path,'before':row,'after':after})
 add('arm/prior01/mount-owner-observations.json',(json.dumps({'observedChanges':changed,'causeOrActorEstablished':False,'historicalHostRowsRefreshed':False,'productionQualified':False},sort_keys=True,indent=2)+'\n').encode())
 for name in ('host.py','worker.py','README.md','manifest.json','control.template.json','prepare-root.py'):copy('source/arm-prior01/'+name,E/'real-journal-rollback-arm-source-proposal-20261018-01'/name)
 for number in ('04','05'):
  src=N/('native-successor-'+number)
  for name in ('result-public.json','final-fences-public.json','journal.stdout.jsonl','journal.stderr.log','journal.terminal.json','preprocess.terminal-public.json','compile.terminal-public.json','link.terminal-public.json','case-decision-public.json','products-decision-public.json','executable-creator-public.json'):
   if (src/name).exists():copy('native/'+number+'/'+name,src/name)
  if number=='04':
   for name in ('cohort-produce.stdout.jsonl','cohort-produce.stderr.log','cohort-produce.terminal.json','cohort-check.stdout.jsonl','cohort-check.stderr.log','cohort-check.terminal.json','held-manifest-commitment.json'):copy('native/04/'+name,src/name)
 for name in ('beacon_journal_physical_rollback.cpp','README.source.md','manifest.json','source.patch','native_successor_05.py','amd_host_v5.py','amd_worker_v3.py','admit-staged-amd-v5.py','collect-amd-v3-terminal.py'):copy('source/v3/'+name,P/name)
 for name in ('host.py','worker.py','README.md','manifest.json','control.template.json','prepare-root.py'):copy('source/arm/'+name,A/name)
 add('source/v2/beacon_journal_physical_rollback.cpp',old_cpp)
 add('source/v3-root-publisher.py',read(E/'publish-journal-v3-batch-root-20261018.py'))
 add('source/v3-root-composer.py',read(pathlib.Path(__file__).resolve()))
 add('',cpp,'replace','tests/qualification/beacon_journal_physical_rollback.cpp')
 facts={'format':'nano-journal-v3-public-scope-v1','sourceParent':HEAD,'coreSourceReference':'9692ccd15b168c513596bc59f951d6e5bf1326f2','cpp':pin(cpp),'amdHostPassed':True,'armHostPassed':arm['passed'],'native04HostPassed':False,'native04CohortCliPassed':True,'native05HostPassed':False,'native05JournalCliPassed':True,'newCoreBuilds':0,'newImageBuilds':0,'operatorRestoreQualified':False,'runtimeDutiesResumed':False,'commonFinalityQualified':False,'externalHistoricalCommitmentFD9Qualified':False,'independentSecurityReviewCompleted':False,'productionQualified':False,'baseNanoDopplerQualified':False}
 add('scope.json',(json.dumps(facts,sort_keys=True,indent=2)+'\n').encode())
 native=read(E/'native-source-cli-controller-scope-proposed-20261018.md').decode()
 arm_sentence=('Le contrôleur ARM est PASS ; ses sources consommées, produits, ancien cohort de69entrées et nettoyage sont vérifiés.'if arm['passed']else 'Le contrôleur ARM reste FAILED ; consulter son résultat exact, aucune qualification globale ARM n’est acquise.')
 text=f'''# Journaux physiques V3 — qualification bornée\n\nParent Git `{HEAD}` ; Core C7 source `9692ccd1`. TU V3 `{pin(cpp)['sha256']}` /28801octets. Le changement concerne uniquement le test : synchronisations avant admission des deux états frais, aucune modification du Core, des floors ou des contrôles après scellement. Aucun build Core/image, réseau, duty ou fonds réel. Le manifeste source `SOURCE_ONLY` décrit son état avant exécution et n’est pas réécrit rétrospectivement.\n\n## Résultats\n\n- AMD64/VPS : contrôleur et C++ PASS,1M/1CXX-MD/1lien/1ldd/1RUN,482dépendances ;7authentifications (4positives/3refus),8ouvertures RO,1signature publique de préparation. Test6416ms/RSS102764KiB. Les671fichiers tenus, neuf logiciels d’origine et provider628fichiers/51répertoires sont inchangés après nettoyage. Audit passif indépendant33fichiers/ELF AMD64/M-MD/image/absence du conteneur PASS.\n- Docker ARM64/Mac : {arm_sentence} [Résultat](arm/result-public.json). Les observations GNU9 en cours de test ne prouvent pas des identités historiques invitées ; les1047sources globales sont vérifiées par noms/taille/SHA, les sources consommées par leurs anciennes FD9 strictes. Le keepalive Docker n’est pas l’exit du C++.\n- Native : {native}\n\n## Portée et suite\n\nV3 remplace le test V2 ; les octets V2 sont archivés dans `source/v2`, les échecs antérieurs restent dans le lot précédent et les nouveaux refus sont conservés ici. Les compteurs d’admission ne permettent aucun rafraîchissement d’anciennes autorités. Les identités de tous les répertoires/SDK historiques ne sont pas qualifiées par la vérification des seuls inputs consommés.\n\nLes bases et engagements de fixtures ne sont pas les journaux opérateur perdus. Restauration complète opérateur, engagements indépendants, reprise effective des duties et progression d’une finalité commune restent ouvertes, comme WAN A/partitions/reconnexion, cycle/rotations/sanctions/retraits/sync, charge et ressources, profil EVM/RPC activé Paris/Shanghai seulement, pruning, CI/reproductibilité/opérations et revue indépendante. Base–Nano/USDC/Doppler et visibilité T0 restent distincts et non qualifiés. Tentative réseau privée03 FAILED et accord consommé ; aucune04 autorisée. `productionQualified=false`.\n'''
 add('README.md',text.encode())
 append=f'''\n\n## Journaux physiques V3 — résultats distincts, objectif actif\n\nLe lot [V3](qualification/real-journal-rollback-v3-20261018/README.md), préparé depuis main86fa881a et Core C7 source9692, remplace uniquement le TU de test b676 par2e5ef451/28801 ; synchronisations avant premières admissions des nouveaux états, anciens floors/Seals stricts inchangés. AMD64 contrôleur+CLI PASS (1TU/482MMD/7auth dont3refus/8RO/1signature publique) ; Docker ARM64 contrôleur {'PASS'if arm['passed']else 'FAILED'}. Native04produce/checkCLI0 et Native05journalCLI0 sont conservés séparément de leurs contrôleurs globaux FAILED lors des clôtures FD9 de porteurs d’engagement. Cause/acteur inconnus, aucune ancienne autorité rafraîchie ni rétention FD9 historique externe qualifiée. Aucune répétition des appels natifs déjà réussis pour forcer un résultat global. La tentative ARM01 a échoué avant compilation après des observations de sept propriétaires invités passant de 0:0 à 501:20 ; les 17 entrées hôte sont restées strictement intactes, cause et acteur inconnus. ARM02 prépare ses seuls nouveaux montages avant leur première admission, sans relâcher les anciennes autorités.\n\nTous les critères précédents sont maintenus : restauration opérateur/duties/finalité commune, consensus/réseau/WAN, profil EVM réellement activé, pruning/CI/reproductibilité/exploitation, revue indépendante non réalisée et Base–Nano/bridge/Doppler restent ouverts. Aucun nouveau Core/image, fonds réel, vente ou déploiement public ; publication de sources distincte du runtime réseau1925. Tentative privée03 FAILED consommée, aucune04 autorisée. Objectif ACTIF, production non qualifiée.\n'''
 for name in ('PRODUCTION-GOAL.md','VALIDATOR-EIP-ROADMAP.md','EVM-PRODUCTION-EIP-ROADMAP.md','EIP-IMPLEMENTATION-STATUS.md'):add('',append.encode(),'append','docs/'+name)
 need(1<=len(items)<=128 and sum(x['after']['bytes']for x in items)<=16*1024*1024,'publication bounds')
 for path,row in list(held.items()):read(path);need(held[path]==row,'all held inputs unchanged')
 plan={'format':'nano-journal-batch-publication-plan-v1','parent':HEAD,'receipt':str(O/'source-write-receipt.json'),'items':items,'productionQualified':False}
 raw=(json.dumps(plan,sort_keys=True,indent=2)+'\n').encode();write(O/'publication-plan.json',raw);print(json.dumps({'passed':True,'files':len(items),'plan':str(O/'publication-plan.json'),'pin':pin(raw),'armHostPassed':arm['passed'],'noRepositoryWrites':True,'productionQualified':False}))
if __name__=='__main__':main()
