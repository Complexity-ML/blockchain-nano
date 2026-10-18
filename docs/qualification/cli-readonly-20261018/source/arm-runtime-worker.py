"""SOURCE_ONLY: C8 runtime-only continuation; retained build RO, no compiler/linker/loader execution."""
import hashlib,json,os,pathlib,posixpath,re,shlex,stat,time
F=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')
OBSERVE = 'set -eu\nexport LC_ALL=C\nfor p do\n r=$(/usr/bin/realpath -e -- "$p"); [ -f "$r" ] && [ ! -L "$r" ]\n printf \'%s\\0\' "$r"\n /usr/bin/stat --printf=\'%d\\0%i\\0%f\\0%h\\0%u\\0%g\\0%s\\0%y\\0%z\\0\' -- "$r"\n h=$(/usr/bin/sha256sum < "$r"); printf \'%s\\0\' "$h"\n /usr/bin/stat --printf=\'%d\\0%i\\0%f\\0%h\\0%u\\0%g\\0%s\\0%y\\0%z\\0\' -- "$r"\n [ "$r" = "$(/usr/bin/realpath -e -- "$p")" ]\ndone\n'

def run_qualification(c):
 def need(ok,why):
  if not ok:raise ValueError(why)
 fields={'hostOutput','dockerPrefix','projectPins','knownGuestPins','guestUid','guestGid','timeoutSeconds','execute','verifyConsumed','beforeRun','runRoot','typedOwners','units','cases','retainedProductPins','retainedBuildProof'}
 need(type(c)is dict and set(c)==fields,'exact runtime-only worker configuration')
 out=pathlib.Path(c['hostOutput']);need(out.is_absolute()and out.resolve(strict=True)==out and out.is_dir(),'new physical host-only runtime output')
 need(type(c['timeoutSeconds'])is int and 600<=c['timeoutSeconds']<=1200,'bounded runtime-only budget');deadline=time.monotonic()+c['timeoutSeconds']
 prefix=c['dockerPrefix'];need(type(prefix)is list and len(prefix)>=3 and prefix[-2]=='exec'and re.fullmatch('[0-9a-f]{64}',prefix[-1]),'Root owned Docker exec prefix')
 need(all(callable(c[k])for k in('execute','verifyConsumed','beforeRun')),'Root bounded retained-input capabilities')
 project=dict(c['projectPins']);known=dict(c['knownGuestPins']);guest={};aliases={};steps=[];serial=0;first=None;secondary=[];owner_preparations=[];typed_owners=c['typedOwners'];case_results=[];completed=False;build_verified=False
 need(type(typed_owners)is dict and set(typed_owners)==set(project)|set(known)and all(type(p)is str and p.startswith(('/work/','/case/','/old/','/retained/','/outputs/'))and type(v)is dict and set(v)=={'uid','gid'}and all(type(v[k])is int and 0<=v[k]<=65535 for k in('uid','gid'))for p,v in typed_owners.items()),'host-derived original owners, no invented guest identity')
 counts={k:0 for k in('independentM','cppCompilations','links','loaderChecks','runs','CoreM','CoreCpp','archiveUpdates')}
 runroot=c['runRoot'];need(type(runroot)is str and re.fullmatch('/state/cli-readonly-runtime-[0-9a-f]{32}',runroot),'fresh sibling-only runtime root')
 def error(exc,stage):
  nonlocal first
  row={'stage':stage,'type':type(exc).__name__,'message':str(exc)}
  if first is None:first=row
  else:secondary.append(row)
 def save(name,value):
  with(out/name).open('x')as f:json.dump(value,f,sort_keys=True,indent=2,allow_nan=False);f.write('\n');f.flush();os.fsync(f.fileno())
 def raw_save(name,b):
  with(out/name).open('xb')as f:f.write(b);f.flush();os.fsync(f.fileno())
 def pinner(row):return {'sha256':row['sha256'],'bytes':row.get('bytes',row.get('st_size'))}
 def run(label,args,timeout=180,closing=False,testing=False):
  limit=min(timeout,deadline+(60 if closing else 0)-time.monotonic());need(limit>0,'physical worker deadline')
  argv=prefix[:-1]+['--workdir','/outputs','--env','HOME=/tmp','--env','TMPDIR='+(runroot+'/tmp'if testing else'/tmp'),'--env','LC_ALL=C','--env','LANG=C','--env','TZ=UTC',prefix[-1],*args]
  began=time.monotonic();rc,stdout,stderr=c['execute']('worker-'+label,argv,limit,closing)
  need(len(stdout)<=16777216 and len(stderr)<=16777216,'worker capture bound');raw_save(label+'.stdout.log',stdout);raw_save(label+'.stderr.log',stderr)
  event={'phase':label,'actualExitCode':rc,'elapsedMs':round((time.monotonic()-began)*1000)};steps.append(event);save(label+'.exit-public.json',event);need(rc==0,'physical command failed: '+label);return stdout,stderr
 def observe(paths,closing=False):
  nonlocal serial
  paths=sorted(set(paths));need(len(paths)<=20000,'guest observation count');rows={};resolved={}
  for at in range(0,len(paths),128):
   batch=paths[at:at+128];need(all(p.startswith('/')and not any(x in p for x in('\0','\n','\r'))for p in batch),'absolute guest paths');serial+=1
   raw,_=run('observe-'+str(serial),['/bin/sh','-c',OBSERVE,'selected-inputs',*batch],90,closing);need(len(raw)<=1048576 and raw.endswith(b'\0'),'guest observation bound');parts=raw[:-1].decode().split('\0');need(len(parts)==20*len(batch),'guest observation field count')
   for i,literal in enumerate(batch):
    values=parts[i*20:(i+1)*20];physical,a,h,b=values[0],values[1:10],values[10],values[11:20]
    need(physical.startswith('/')and a==b and re.fullmatch('[0-9a-f]{64}  -',h),'guest stat/hash/stat coherence');need(all(re.fullmatch('[0-9]+',a[j])for j in(0,1,3,4,5,6))and re.fullmatch('[0-9a-f]+',a[2])and stat.S_ISREG(int(a[2],16))and 0<=int(a[6])<=134217728,'guest regular GNU9')
    row={'materialGNU9':a,'sha256':h[:64],'bytes':int(a[6])};need(physical not in rows or rows[physical]==row,'repeated guest identity changed');rows[physical]=row;resolved[literal]=physical
  return rows,resolved
 def prepare_known_owners(paths,label):
  selected=sorted(set(paths));new_known=[p for p in selected if p in typed_owners and(p not in aliases or aliases[p]not in guest)]
  if not new_known:return
  start=time.monotonic();fixed=None;probes=0;previous_owner=None;owner_changes=0
  while True:
   rows,links=observe(selected);checks={};owners_ready=True
   for literal,physical in links.items():
    row=rows[physical];expected=project.get(literal)or known.get(literal);actual=pinner(row);canonical=not literal.startswith(('/work/','/case/','/old/','/retained/','/outputs/'))or expected is not None and literal==physical
    want=typed_owners.get(literal);observed={'uid':int(row['materialGNU9'][4]),'gid':int(row['materialGNU9'][5])};matched=want is None or observed==want;owners_ready=owners_ready and matched
    checks[literal]={'expectedPin':expected,'actualPin':actual,'canonicalProjectProviderPath':canonical,'knownBytesMatch':expected is None or actual==expected,'expectedOwner':want,'observedOwner':observed,'expectedOwnerReached':matched}
   probes+=1;save(label+'-unadmitted-owner-preparation-'+str(probes)+'.private.json',{'rows':rows,'aliases':links,'checks':checks,'unadmitted':True,'guestAuthorityCreated':False,'elapsedMs':round((time.monotonic()-start)*1000)})
   need(all(v['canonicalProjectProviderPath']and v['knownBytesMatch']for v in checks.values()),'unadmitted owner preparation known bytes/aliases')
   for literal,physical in links.items():
    need(literal not in aliases or aliases[literal]==physical,'held alias changed; no owner wait/reset');need(physical not in guest or guest[physical]==rows[physical],'held guest row changed; no owner wait/reset')
   # Only UID/GID can change during this preparation. No observation becomes a row authority.
   identity=({p:(r['materialGNU9'][:4]+r['materialGNU9'][6:],r['sha256'],r['bytes'])for p,r in rows.items()},links)
   if fixed is None:fixed=identity
   need(identity==fixed,'unadmitted dev/ino/mode/nlink/size/mtime/ctime/SHA/aliases fixed')
   observed_owners={p:(r['materialGNU9'][4],r['materialGNU9'][5])for p,r in rows.items()}
   if previous_owner is not None:owner_changes+=sum(previous_owner[p]!=v for p,v in observed_owners.items())
   previous_owner=observed_owners;now=time.monotonic();need(now<min(deadline,start+30)and probes<=30,'bounded unadmitted owner preparation including durable samples')
   if owners_ready:
    owner_preparations.append({'phase':label,'probes':probes,'expectedOwnerPaths':len(new_known),'observedOwnerChanges':owner_changes,'elapsedMs':round((now-start)*1000),'admitted':False});return
   time.sleep(1.05)
 def admit_inputs(paths,label):
  prepare_known_owners(paths,label)
  start=time.monotonic();previous=None;streak=0;since=0;byte_pins={};n=0;fixed=None
  while True:
   rows,links=observe(paths);n+=1;save(label+'-sample-'+str(n)+'.private.json',{'rows':rows,'aliases':links,'elapsedMs':round((time.monotonic()-start)*1000)})
   for literal,physical in links.items():
    row=rows[physical];need(literal not in aliases or aliases[literal]==physical,'old guest alias changed');need(physical not in guest or guest[physical]==row,'old guest row changed')
    expected=project.get(literal)or known.get(literal)
    if literal in typed_owners:need({'uid':int(row['materialGNU9'][4]),'gid':int(row['materialGNU9'][5])}==typed_owners[literal],'original host-derived owner required before first eight-field anchor')
    if literal.startswith(('/work/','/case/')):need(literal in project and literal==physical,'project dependency exact path')
    if physical.startswith(('/work/','/case/')):need(physical in project and pinner(row)==project[physical],'resolved project pin')
    if literal.startswith(('/old/','/retained/','/outputs/')):need(literal in known and literal==physical,'known old provider only')
    need(expected is None or pinner(row)==expected,'known dependency bytes changed');need(physical not in byte_pins or byte_pins[physical]==pinner(row),'prospective SDK bytes changed');byte_pins.setdefault(physical,pinner(row))
   identity=({p:(r['materialGNU9'][:8],r['sha256'],r['bytes'])for p,r in rows.items()},links)
   if fixed is None:fixed=identity
   need(identity==fixed,'new guest eight fields/bytes/aliases unchanged');now=time.monotonic();need(now<min(deadline,start+30),'guest first-admission deadline')
   if(rows,links)==previous:streak+=1
   else:streak=1;since=now
   if streak>=3 and now-since>=2:break
   previous=(rows,links);time.sleep(1.05)
  for p,r in rows.items():guest.setdefault(p,r)
  for p,r in links.items():aliases.setdefault(p,r)
 def json_body(raw):
  def unique(pairs):
   d={}
   for k,v in pairs:need(k not in d,'duplicate JSON key');d[k]=v
   return d
  def invalid(_):raise ValueError('nonfinite JSON')
  return json.loads(raw,object_pairs_hook=unique,parse_constant=invalid)
 def same(a,b):return json.dumps(a,sort_keys=True,allow_nan=False)==json.dumps(b,sort_keys=True,allow_nan=False)
 def oracle(raw,err,spec):
  need(type(spec)is dict and type(spec.get('kind'))is str,'explicit source-derived case oracle')
  if spec['kind']=='chain-count-line':
   need(set(spec)=={'kind','minimum','maximum'}and type(spec['minimum'])is int and type(spec['maximum'])is int and 1<=spec['minimum']<=spec['maximum']<=999999 and err==b'','closed bounded chain-count oracle');match=re.fullmatch(rb'([1-9][0-9]{0,5}) chain checks passed\n',raw);need(match is not None and spec['minimum']<=int(match[1])<=spec['maximum'],'exact typed chain count line');return {'chainChecks':int(match[1])}
  if spec['kind']=='exact':
   need(set(spec)=={'kind','stdout','stderr'}and type(spec['stdout'])is str and type(spec['stderr'])is str,'closed literal oracle');need(raw==spec['stdout'].encode()and err==spec['stderr'].encode(),'exact case output');return None
  need(spec['kind']=='json-records'and set(spec)=={'kind','records'}and err==b''and type(spec['records'])is list and 1<=len(spec['records'])<=32,'closed JSON records oracle')
  rows=[json_body(line)for line in raw.splitlines()];need(len(rows)==len(spec['records']),'exact record count')
  for row,want in zip(rows,spec['records']):
   need(type(want)is dict and set(want)=={'fixed','integers'}and type(want['fixed'])is dict and type(want['integers'])is dict and not(set(want['fixed'])&set(want['integers'])),'closed typed record specification')
   need(type(row)is dict and set(row)==set(want['fixed'])|set(want['integers'])and same({k:row[k]for k in want['fixed']},want['fixed']),'typed exact fixed fields')
   for key,bounds in want['integers'].items():need(type(bounds)is list and len(bounds)==2 and all(type(x)is int for x in bounds)and bounds[0]<=bounds[1]and type(row[key])is int and bounds[0]<=row[key]<=bounds[1],'typed bounded observed integer')
  return rows
 def valid_pin(v):return type(v)is dict and set(v)=={'sha256','bytes'}and type(v['sha256'])is str and re.fullmatch('[0-9a-f]{64}',v['sha256'])and type(v['bytes'])is int and 0<=v['bytes']<=134217728
 try:
  units=c['units'];cases=c['cases'];need(type(units)is list and len(units)==8 and type(cases)is list and len(cases)==8,'exact eight retained units/eight new RUNs')
  roles={'nano','readonly','cli-probe','chain','block-auth','bootstrap-startup','beacon-gate','recovery-audit'};by_name={};executables=set()
  for u in units:
   need(type(u)is dict and set(u)=={'name','source','compilerFlags','linkPrefix','linkTail','executable'}and u['name']in roles and u['name']not in by_name,'closed original unit role')
   need(type(u['executable'])is str and re.fullmatch('[a-z][a-z0-9-]{0,63}',u['executable'])and u['executable']not in executables,'unique retained executable basename');by_name[u['name']]=u;executables.add(u['executable'])
  expected_cases={('readonly',('--self-test',)),('cli-probe',('/outputs/'+by_name['nano']['executable'],'$NANO_SHA','$NANO_BYTES','$PROBE_ROOT')),('chain',()),('block-auth',()),('bootstrap-startup',('--self-test',)),('beacon-gate',()),('recovery-audit',('Paris',)),('recovery-audit',('Shanghai',))};found=set();names=set()
  for case in cases:
   need(type(case)is dict and set(case)=={'name','unit','args','timeoutSeconds','oracle'}and type(case['name'])is str and re.fullmatch('[a-z][a-z0-9-]{0,63}',case['name'])and case['name']not in names,'closed unique case name');names.add(case['name'])
   need(type(case['args'])is list and all(type(x)is str and '\0'not in x for x in case['args'])and type(case['timeoutSeconds'])is int and 1<=case['timeoutSeconds']<=240,'bounded explicit runtime arguments')
   key=(case['unit'],tuple(case['args']));need(key in expected_cases and key not in found,'each exact case only once');found.add(key)
   if case['unit']=='chain':need(type(case['oracle'])is dict and case['oracle'].get('kind')=='chain-count-line','source-derived count line, no learned oracle')
  need(found==expected_cases,'eight expected runtime selectors')
  retained=c['retainedProductPins'];need(type(retained)is dict and len(retained)==36 and all(type(p)is str and p.startswith('/outputs/')and posixpath.normpath(p)==p and '/'not in p.removeprefix('/outputs/')and valid_pin(v)for p,v in retained.items()),'exact known36 retained product pins')
  expected_products={'/outputs/chain.cpp.o.M','/outputs/chain.cpp.o','/outputs/chain.cpp.o.d','/outputs/libnano_core.a'}
  for u in units:expected_products.update('/outputs/'+n for n in(u['name']+'.o.M',u['name']+'.o',u['name']+'.o.d',u['executable']))
  need(set(retained)==expected_products and all(known.get(p)==v for p,v in retained.items()),'same36 known original products, fixed guest layout')
  proof=c['retainedBuildProof'];need(type(proof)is dict and set(proof)=={'archiveProof','mmd','retainedGuestInputs','consumedPaths'},'Root proof from pinned original authority/result, not current logs')
  ap=proof['archiveProof'];need(type(ap)is dict and set(ap)=={'passed','memberCount','changedMembers','unchangedMembers','replacedMember','retainedArchivePin','newArchivePin','objectPin'}and same({k:ap[k]for k in('passed','memberCount','changedMembers','unchangedMembers','replacedMember')},{'passed':True,'memberCount':143,'changedMembers':1,'unchangedMembers':142,'replacedMember':'chain.cpp.o'}),'retained143/142/1 singleton proof')
  need(ap['newArchivePin']==retained['/outputs/libnano_core.a']and ap['objectPin']==retained['/outputs/chain.cpp.o']and ap['retainedArchivePin']==known.get('/retained/libnano_core.a'),'proof joins original retained products/Core')
  mmd=proof['mmd'];need(type(mmd)is dict and set(mmd)==roles|{'core-chain'},'nine original MMD proofs')
  for name,v in mmd.items():
   need(type(v)is dict and set(v)=={'normalizedMEqualsMd','literalMEqualsMd','dependencyCount','target','source'}and v['normalizedMEqualsMd']is True and v['literalMEqualsMd']is True and type(v['dependencyCount'])is int and 1<=v['dependencyCount']<=20000,'closed original MMD evidence')
   need(v['target']=='/outputs/'+('chain.cpp.o'if name=='core-chain'else name+'.o')and v['source']==('/case/src/chain.cpp'if name=='core-chain'else by_name[name]['source']),'exact retained MT/source role')
  consumed=proof['consumedPaths'];need(type(consumed)is list and consumed and len(consumed)<=20000 and len(set(consumed))==len(consumed)and all(type(p)is str and p.startswith('/')and posixpath.normpath(p)==p for p in consumed),'original closed MMD dependency union');c['verifyConsumed'](consumed)
  inputs=proof['retainedGuestInputs'];need(type(inputs)is dict and set(inputs)=={'aliases','pins'}and type(inputs['aliases'])is dict and type(inputs['pins'])is dict and inputs['aliases']and len(inputs['aliases'])<=20000,'known original path routing/bytes only')
  routing=inputs['aliases'];pins=inputs['pins'];need(set(routing.values())==set(pins)and all(type(p)is str and p.startswith('/')and posixpath.normpath(p)==p and type(q)is str and q.startswith('/')and posixpath.normpath(q)==q for p,q in routing.items())and all(valid_pin(v)for v in pins.values()),'closed known routing and physical byte pins')
  # Import bytes and routing only. No old materialGNU9 is ever copied or substituted.
  for literal,physical in routing.items():
   want=pins[physical];need(literal not in project or project[literal]==want,'original project bytes match retained routing');need(literal not in known or known[literal]==want,'known mounted bytes match original routing');known.setdefault(literal,want)
  for p,v in pins.items():need(p not in known or known[p]==v,'canonical known bytes consistent');known.setdefault(p,v)
  initial=sorted(set(routing)|set(retained)|{p for p in project if p.startswith('/case/')});admit_inputs(initial,'runtime-initial')
  need(all(aliases[p]==q for p,q in routing.items())and all(pinner(guest[aliases[p]])==v for p,v in retained.items()),'new guest admission matches original routing/known product bytes')
  c['verifyConsumed'](consumed);c['beforeRun']();build_verified=True
  run('test-root',['/bin/sh','-ec','test ! -e "$1"; /bin/mkdir -m 700 -- "$1"; /bin/mkdir -m 700 -- "$1/tmp"','fresh-runtime-root',runroot],15)
  nano=retained['/outputs/'+by_name['nano']['executable']];replacement={'$NANO_SHA':nano['sha256'],'$NANO_BYTES':str(nano['bytes']),'$PROBE_ROOT':runroot+'/synthetic-cli-readonly-'+runroot.rsplit('-',1)[-1]}
  for case in cases:
   c['beforeRun']();counts['runs']+=1;raw,err=run('case-'+case['name'],['/outputs/'+by_name[case['unit']]['executable'],*[replacement.get(x,x)for x in case['args']]],case['timeoutSeconds'],testing=True);rows=oracle(raw,err,case['oracle']);case_results.append({'name':case['name'],'unit':case['unit'],'passed':True,'records':rows})
  completed=True
 except BaseException as exc:error(exc,'runtime-qualification')
 finally:
  if aliases:
   try:rows,links=observe(list(aliases),True);need(rows==guest and links==aliases,'all new per-run guest rows/aliases unchanged')
   except BaseException as exc:error(exc,'guest-final-fence')
  try:c['beforeRun']()
  except BaseException as exc:error(exc,'original-host-input-final-fence')
  authority={'guestStatSha':guest,'guestAliases':aliases,'retainedProductPins':c['retainedProductPins'],'oldGuestRowsReadopted':False,'historicalGuestFD9Qualified':False}
  try:save('runtime-observations.private.json',authority)
  except BaseException as exc:error(exc,'observation-publication')
  report={'format':'nano-cli-readonly-runtime-worker-v1','passed':first is None and completed,'counts':counts,'counterKind':'attemptedCommands','caseResults':case_results,'retainedBuildProducts':36 if build_verified else 0,'newBuildProducts':0,'retainedBuildProofVerified':build_verified,'originalCarrierRequalified':False,'ownerPreparationSamples':owner_preparations,'typedOwnerCount':len(typed_owners),'oldGuestRowsReadopted':False,'historicalGuestFD9Qualified':False,'firstFailure':first,'secondaryFailures':secondary,'steps':steps,'CppExitSource':'owned docker exec actual exit and closed host process group','keepaliveStateExitCodeIsCppExitCode':False,'CoreRebuilt':False,'newImages':0,'runtimeDutiesAuthorized':False,'productionQualified':False}
  try:save('result-public.json',report)
  except BaseException as exc:error(exc,'result-publication');report['passed']=False;report['firstFailure']=first
 return report,authority
