"""Collect only fixed public evidence and prepare one append-only documentation lot."""
import base64,hashlib,json,os,pathlib,stat,subprocess
E=pathlib.Path(__file__).resolve().parent
R=pathlib.Path('/Users/boris/Dev/blockchain-nano')
O=E/'native-storage-publication-root-20261018-01'
Q='docs/qualification/native-storage-20261018/'
HEAD='ffe8cd99de3d3aaff2c0c634b871953660da12f9'
GIT='/Library/Developer/CommandLineTools/usr/bin/git'
F=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')
held={};items=[]
def need(ok,why):
 if not ok:raise RuntimeError(why)
def pin(raw):return {'sha256':hashlib.sha256(raw).hexdigest(),'bytes':len(raw)}
def read(p,want=None):
 p=pathlib.Path(p);need(p.is_absolute()and p.resolve(strict=True)==p,'physical explicit input');fd=os.open(p,os.O_RDONLY|os.O_NOFOLLOW)
 try:
  s=os.fstat(fd);row={k:getattr(s,k)for k in F};need(stat.S_ISREG(s.st_mode)and s.st_nlink==1 and 0<=s.st_size<=2097152,'bounded single-link public input');raw=b''
  while b:=os.read(fd,65536):raw+=b;need(len(raw)<=s.st_size,'input grew')
  z=os.fstat(fd);a=p.lstat();need(len(raw)==s.st_size and row=={k:getattr(z,k)for k in F}=={k:getattr(a,k)for k in F},'FD9/path stable');row['sha256']=pin(raw)['sha256']
  if want is not None:need(pin(raw)==want,'independently known bytes')
  need(str(p)not in held or held[str(p)]==row,'no held row refresh');held.setdefault(str(p),row);return raw
 finally:os.close(fd)
def encoded(d):return (json.dumps(d,sort_keys=True,indent=2,allow_nan=False)+'\n').encode()
def decoded(b):
 def unique(pairs):
  d={}
  for k,v in pairs:need(k not in d,'duplicate JSON key');d[k]=v
  return d
 return json.loads(b,object_pairs_hook=unique,parse_constant=lambda _:(_ for _ in()).throw(ValueError('nonfinite JSON')))
def git(*args):
 p=subprocess.run([GIT,'--no-optional-locks','-c','core.fsmonitor=false',*args],cwd=R,capture_output=True,timeout=15);need(p.returncode==0 and not p.stderr,'read-only Git0');return p.stdout.decode().strip()
def write(p,b):
 p.parent.mkdir(mode=0o700,parents=True,exist_ok=True);need(p.parent.resolve(strict=True)==p.parent and not os.path.lexists(p),'new physical payload');fd=os.open(p,os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,0o400)
 with os.fdopen(fd,'wb')as f:need(f.write(b)==len(b),'complete payload');f.flush();os.fsync(f.fileno())
def add(name,b,destination=None):
 dst=destination or Q+name;mode='append'if destination else'new';p=O/'payload'/dst;write(p,b);item={'source':str(p),'destination':dst,'mode':mode,'input':pin(b)}
 if mode=='append':old=read(R/dst);item['before']=pin(old);out=old+b
 else:need(not os.path.lexists(R/dst),'new destination only');out=b
 item['after']=pin(out);items.append(item)
def copy(name,p):add(name,read(p))
def main():
 os.umask(0o077);need(git('branch','--show-current')=='main'and git('rev-parse','HEAD')==HEAD and not git('status','--porcelain'),'clean fixed parent');need(not os.path.lexists(O),'fresh publication namespace');O.mkdir(mode=0o700)
 armroot=E/'journal-arm03-passive-root-20261018-01';raw=read(armroot/'result-public.json',{'sha256':'e183e2fc50ba88322361fad6847f4a0aa1f34e0837ba7a3de757c8908d10a9f6','bytes':5489});audit=decoded(raw)
 need(audit['passed']is True and audit['CppInvoked']is False and audit['oldOriginalSoftwareRows']==9 and audit['consumedOriginalSourceRows']==18 and audit['originalBuildProducts']==4 and audit['MMD']['dependencyCount']==489 and audit['actualCppCounts']=={'CoreAuthentications':7,'expectedCoreRefusals':3,'newTestOnlySignaturesCreated':1,'positiveAuthentications':4,'readonlyDatabaseOpens':8},'ARM03 passive audit exact scope');add('arm/root-passive-audit.json',raw)
 allowed={'host-result-public.json','host-final-fence-public.json','worker-result-public.json','RUN.stdout.log','RUN.stderr.log','M.exit-public.json','CXX.exit-public.json','LINK.exit-public.json','LDD.exit-public.json','RUN.exit-public.json','loader-closure-public.json','fixture-before-RUN-public.json','fixture-after-RUN-public.json','fixture-after-cleanup-public.json','source-before-public.json','source-after-cleanup-public.json','preparation-result-public.json','source-pins-public.json'}
 need(len(audit['publicFiles'])==18 and {x['file']for x in audit['publicFiles']}==allowed,'closed ARM public allowlist')
 for d in audit['publicFiles']:add('arm/'+d['file'],read(armroot/d['file'],d['pin']))
 arm=decoded(read(armroot/'host-result-public.json'));need(arm['passed']is True and arm['worker']['passed']is True and arm['worker']['record']['actualPeakRssKiB']==18736 and arm['worker']['record']['elapsedMs']==6427 and arm['worker']['counts']=={'CoreCpp':0,'CoreM':0,'archiveUpdates':0,'cppCompilations':1,'independentM':1,'links':1,'loaderChecks':1,'runs':1},'ARM actual complete terminal PASS')
 exportroot=E/'physical-native-export-root-20261018-01';exported=decoded(read(exportroot/'result-public.json',{'sha256':'7d55ffa8313961489970a02a27b41a4ebefa7a6a1774c1ba4bc0d7ee98385496','bytes':6406}));need(exported['passed']is True and exported['sourceEntries']==69 and exported['portableRoleInventories']==8 and exported['native04HostWholeFailedPreserved']is True and exported['historicalSourceFD9Qualified']is False,'Native public export scope')
 for n in ('result-public.json','cohort-manifest-public.json','export-inventory-public.json','export-external-commitment-public.json','source-current-observation-public.json'):copy('native-export/'+n,exportroot/n)
 packet=decoded(read(E/'physical-native-import-amd-passive-collection-root-20261018.json',{'sha256':'95878a5738232838ee5aa6b040cdd7da26b03c4d743e637610806829ea345204','bytes':84718}));need(packet['passed']is True and packet['heldFilesUnchanged']==18 and packet['actualCppExitAndTerminalStateZero']is True and packet['newCppInvocations']==0 and packet['imported69CurrentFD9ShaUnchanged']is True,'passive AMD real proof collection')
 amd_allowed={'result-public.json','checker-records-public.json','container-terminal-public.json','import-external-commitment-public.json','input-final-fence-public.json','check-5.exit-public.json','check-5.stdout.log','check-5.stderr.log','created-4.stdout.log','terminal-6.stdout.log','image-before-1.stdout.log','image-final-14.stdout.log','absence-13.exit-public.json','absence-13.stderr.log'};need(set(packet['exports'])==amd_allowed,'closed fourteen AMD public exports')
 bodies={}
 for n,d in packet['exports'].items():
  b=base64.b64decode(d['base64'],validate=True);need(pin(b)=={'sha256':d['row']['sha256'],'bytes':d['row']['st_size']}and len(b)<=2097152,'exact bounded public export');bodies[n]=b;add('amd/'+n,b)
 summary={k:v for k,v in packet.items()if k!='exports'};add('amd/root-passive-audit.json',encoded(summary));amd=decoded(bodies['result-public.json']);need(pin(bodies['result-public.json'])=={'sha256':'08a8287cc4c7b2b0aff9c8ae14cb3f042586ce0000947961a8baa9f01c069de4','bytes':6753}and amd['passed']is True and amd['firstFailure']is None and amd['secondaryFailures']==[] and amd['checkInvocationsAttempted']==1 and amd['inputFinalFence']['heldFiles']==21 and amd['inputFinalFence']['passed']is True,'AMD host actual terminal success')
 records=decoded(bodies['checker-records-public.json']);manifest=decoded(read(exportroot/'cohort-manifest-public.json'));need(records==amd['checkerRecords']and records[2]['databases']==8 and records[2]['archivesAuthenticated']==5 and records[2]['newSignaturesInCheck']==0 and records[2]['actualPeakRssKiB']==24392,'AMD typed C++ result and zero new signatures')
 for a,c in zip(records[:2],manifest['cohorts']):need(a['primaryToken']==c['primary']['token']and a['witnessToken']==c['witness']['token']and a['headRoot']==c['headRoot'],'same independent Native manifest commitments')
 for directory,names,destination in (
  ('physical-native-export-source-proposal-20261018-01',('runner.py','cohort_common.py','README.md','CONTROL.template.json','manifest.json'),'source/native-export/'),
  ('physical-native-import-amd-source-proposal-20261018-01',('runner.py','cohort_common.py','README.md','CONTROL.template.json','manifest.json'),'source/native-import/'),
  ('real-journal-rollback-arm-source-proposal-20261018-03',('host.py','worker.py','README.md','manifest.json','control.template.json','prepare-root.py','passive-audit-root.py'),'source/arm03/')):
  for n in names:copy(destination+n,E/directory/n)
 for n in ('prepare-native-export-control-root-20261018.py','prepare-native-import-amd-control-root-20261018.py','publish-native-storage-batch-root-20261018.py','compose-native-storage-publication-root-20261018.py'):copy('source/root/'+n,E/n)
 copy('native-export/transfer-receipt-01-FAILED.json',E/'native-export-transfer-root-refusal-20261018-01.json')
 scope={'format':'nano-native-storage-public-scope-v1','sourceParent':HEAD,'coreReference':'9692ccd15b168c513596bc59f951d6e5bf1326f2','arm03JournalHostPassed':True,'arm03PassiveAuditPassed':True,'nativeMacToLinuxAmdCohortReadPassed':True,'amdCheckInvocations':1,'amdReadonlyDatabaseCount':8,'amdArchivesAuthenticated':5,'amdNewSignatures':0,'native04WholeFailedPreserved':True,'native05WholeFailedPreserved':True,'newCoreBuilds':0,'newImageBuilds':0,'operatorRecoveryQualified':False,'runtimeDutiesResumed':False,'commonFinalityQualified':False,'globalPhysicalPortabilityQualified':False,'independentSecurityReviewCompleted':False,'productionQualified':False,'baseNanoDopplerQualified':False};add('scope.json',encoded(scope))
 text='''# Stockage natif macOS vers Linux et clôture du journal ARM64

Parent de publication `ffe8cd99`, Core C7 de référence `9692ccd1`. Aucun Core, image ou binaire réseau remplacé par ce lot documentaire. Les qualifications concernent les programmes, fichiers et empreintes joints ; les versions et états précédents restent historiques.

## Résultats acquis

- Docker ARM64/Mac, journal physique V3 : contrôleur et audit indépendant PASS. Une compilation du TU, 489 dépendances M/MD concordantes, un lien, un contrôle du chargeur et une invocation ; Core de 143 membres réutilisé. Sept authentifications (quatre positives et trois refus), huit ouvertures RO, une signature de fixture publique. Test 6 427 ms, RSS 18 736 KiB. Neuf logiciels d’origine, dix-huit sources consommées et quatre produits inchangés ; trois inventaires de 69 entrées identiques, conteneurs retirés, image conservée. Les échecs ARM01/02 restent FAILED ; leur propriétaire invité divergent ne vaut pas une corruption des bytes hôte. [Audit](arm/root-passive-audit.json).
- Export de la cohorte publique Native04 existante : 69 entrées, huit inventaires portables, archive USTAR de 5 283 840 octets. Source et cinq nouveaux carriers inchangés ; aucun appel C++, API DB ou signature pour l’export. L’admission courante de la source ne rétablit pas ses anciennes métadonnées historiques. [Résultat](native-export/result-public.json).
- Lecture sur le VPS Linux AMD64 après vrai transport et copie exclusive sur nouveaux inodes : une seule CLI `check`, huit DB, cinq archives, deux passes RO, zéro signature. Racines et tokens primaire/témoin égaux au manifeste macOS indépendant. CLI et State.ExitCode 0, groupe fermé ; 7 930 ms pour la commande Docker, RSS C++ 24 392 KiB. Les 21 entrées tenues par le contrôleur et les 69 fichiers/répertoires de la copie restent inchangés après nettoyage ; image conservée, conteneur absent. L’audit passif Root tient séparément 18 fichiers et vérifie stdout, état terminal, engagements, lecteur ELF d’origine et copie, sans nouveau test C++. [Résultat](amd/result-public.json), [audit](amd/root-passive-audit.json).

Cette preuve qualifie **cette cohorte macOS Native04 lue par ce binaire AMD64 et cette image Linux**. Elle ne qualifie ni toutes les versions RocksDB ni le sens inverse vers macOS. La version RocksDB AMD exacte n’est pas déduite de celle du lecteur ARM. Aucun ancien volume opérateur n’est monté ; le conteneur n’a que deux binds RO, aucun réseau ni port. Les bases brutes restent hors Git ; leurs inventaires et engagements sont joints.

## Échecs conservés et limites

Native04 et Native05 restent globalement FAILED malgré leurs réussites CLI distinctes. La source macOS est admise uniquement pour ce nouvel export ; aucune ancienne autorité ou rétention historique FD9 n’est rafraîchie. Le premier reçu ad hoc de transfert a aussi échoué : il comparait un tuple stat incluant atime. Un diagnostic séparé observe les neuf champs et hashes inchangés et des variations d’atime ; le champ exact du premier refus non détaillé n’est pas prouvé rétrospectivement. [Refus](native-export/transfer-receipt-01-FAILED.json). La réussite du vrai import ne réécrit aucun de ces échecs.

Restauration opérateur avec ses journaux privés et témoins, engagements indépendants, reprise des duties et progression d’une finalité commune restent ouvertes. Le test de recul des journaux sur Docker ne prouve pas la résistance au recul simultané de toutes les autorités externes. Partitions/WAN A, cycle des validateurs, rotations, sync, sanctions/retraits, charge/ressources, profil EVM/RPC réellement activé, pruning, CI/reproductibilité et exploitation gardent leurs critères détaillés. La revue de sécurité indépendante n’est pas réalisée. Le défaut CLI status/audit ouvrant en écriture est identifié ; sa proposition de correction et son test restent non compilés et non intégrés à cette publication.

Les quatre documents normatifs sont conservés intégralement et reçoivent seulement un suffixe. Profil réseau Paris/Shanghai uniquement ; aucun nouveau fork activé. Le runtime réseau de référence `1925eee` reste distinct des sources, non remplacé ni requalifié par ce lot. Campagne privée03 FAILED, accord consommé ; aucune04 autorisée. Base–Nano/USDC/Doppler, conversion/réserve, sécurité du bridge, termes et visibilité T0 restent ouverts. Aucun fonds réel, vente, déploiement public ou dépense. Objectif ACTIF, production non qualifiée.
'''
 add('README.md',text.encode())
 append='''

## Cohorte macOS → Linux AMD64 et journal ARM03 — qualifications bornées

Le lot [stockage natif](qualification/native-storage-20261018/README.md), parent main `ffe8cd99`, ne modifie aucun Core ni image. Journal V3 Docker ARM03 : contrôleur et audit indépendant PASS, 489 M/MD, un TU/une invocation, sept authentifications dont trois refus, huit RO et une signature publique ; anciens ARM01/02 FAILED conservés. La cohorte publique Native04 a été exportée sans API DB puis réellement transférée/importée sur le VPS : un seul check AMD64 RO, huit DB/cinq archives/deux passes/zéro nouvelle signature, mêmes tokens et racines que le manifeste macOS. CLI et State0, 21 entrées tenues et 69 entrées importées inchangées après nettoyage, audit passif Root indépendant PASS. Le reçu initial de transfert FAILED (tuple stat incluant atime) est conservé ; aucun échec historique réécrit. Native04/05 restent whole FAILED et leurs anciennes autorités ne sont pas rafraîchies.

Portée limitée aux sources/binaires/images et cohorte joints : ni portabilité universelle ou inverse vers macOS, ni restauration opérateur, reprise de duties, finalité commune ou résistance au recul simultané de toutes les autorités externes. Consensus/WAN/partitions/cycle/rotations/sync/sanctions/retraits, charge/ressources, profil EVM activé Paris/Shanghai, pruning/CI/reproductibilité/exploitation et revue indépendante non réalisée restent ouverts selon leurs critères intégraux. Défaut CLI status/audit RW identifié ; correctif/test encore non compilés, non intégrés ici. Base–Nano/bridge USDC/Doppler, réserve/conversion, termes et visibilité T0 restent distincts et ouverts. Sources publiées distinctes du runtime réseau1925 inchangé ; aucun Core/image, fonds, vente, déploiement public ou campagne privée04. Tentative privée03 FAILED consommée. Objectif ACTIF, production non qualifiée.
'''
 for n in ('PRODUCTION-GOAL.md','VALIDATOR-EIP-ROADMAP.md','EVM-PRODUCTION-EIP-ROADMAP.md','EIP-IMPLEMENTATION-STATUS.md'):add('',append.encode(),'docs/'+n)
 need(len(items)<=128 and sum(i['after']['bytes']for i in items)<=16*1024*1024,'bounded lot')
 for p,row in list(held.items()):read(p);need(held[p]==row,'held inputs unchanged')
 plan={'format':'nano-native-storage-publication-plan-v1','parent':HEAD,'receipt':str(O/'source-write-receipt.json'),'items':items,'productionQualified':False};raw=encoded(plan);write(O/'publication-plan.json',raw);print(json.dumps({'passed':True,'files':len(items),'plan':str(O/'publication-plan.json'),'pin':pin(raw),'noRepositoryWrites':True,'productionQualified':False}))
if __name__=='__main__':main()
