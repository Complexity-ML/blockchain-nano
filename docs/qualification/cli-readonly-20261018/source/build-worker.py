"""SOURCE_ONLY: host-Python C8 batch; explicit comment-only Core reuse, no Docker lifecycle."""
import hashlib,json,os,pathlib,posixpath,re,shlex,stat,time
F=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')
OBSERVE = 'set -eu\nexport LC_ALL=C\nfor p do\n r=$(/usr/bin/realpath -e -- "$p"); [ -f "$r" ] && [ ! -L "$r" ]\n printf \'%s\\0\' "$r"\n /usr/bin/stat --printf=\'%d\\0%i\\0%f\\0%h\\0%u\\0%g\\0%s\\0%y\\0%z\\0\' -- "$r"\n h=$(/usr/bin/sha256sum < "$r"); printf \'%s\\0\' "$h"\n /usr/bin/stat --printf=\'%d\\0%i\\0%f\\0%h\\0%u\\0%g\\0%s\\0%y\\0%z\\0\' -- "$r"\n [ "$r" = "$(/usr/bin/realpath -e -- "$p")" ]\ndone\n'

def run_qualification(c):
 def need(ok,why):
  if not ok:raise ValueError(why)
 fields={'hostOutput','dockerPrefix','projectPins','knownGuestPins','guestUid','guestGid','timeoutSeconds','execute','verifyConsumed','beforeRun','runRoot','typedOwners','coreUnit','units','cases','archiveTool','retainedCore','proveArchiveSingleton'}
 need(type(c)is dict and set(c)==fields,'exact C8 worker configuration')
 out=pathlib.Path(c['hostOutput']);need(out.is_absolute()and out.resolve(strict=True)==out and out.is_dir(),'fresh physical worker output')
 need(type(c['timeoutSeconds'])is int and 600<=c['timeoutSeconds']<=3600,'bounded worker budget');deadline=time.monotonic()+c['timeoutSeconds']
 prefix=c['dockerPrefix'];need(type(prefix)is list and len(prefix)>=3 and prefix[-2]=='exec'and re.fullmatch('[0-9a-f]{64}',prefix[-1]),'Root owned Docker exec prefix')
 need(all(callable(c[k])for k in('execute','verifyConsumed','beforeRun','proveArchiveSingleton')),'Root bounded capabilities')
 project=dict(c['projectPins']);known=dict(c['knownGuestPins']);guest={};aliases={};products={};steps=[];serial=0;first=None;secondary=[];owner_preparations=[];typed_owners=c['typedOwners']
 need(type(typed_owners)is dict and set(typed_owners)==set(project)|set(known)and all(type(p)is str and p.startswith(('/work/','/case/','/old/','/retained/'))and type(v)is dict and set(v)=={'uid','gid'}and all(type(v[k])is int and 0<=v[k]<=65535 for k in('uid','gid'))for p,v in typed_owners.items()),'closed typed owners derived by host from original/created rows')
 counts={k:0 for k in('independentM','cppCompilations','links','loaderChecks','runs','CoreM','CoreCpp','archiveUpdates')};mmd={};case_results=[];archive_proof=None;completed=False
 runroot=c['runRoot'];need(type(runroot)is str and re.fullmatch('/state/cli-readonly-[0-9a-f]{32}',runroot),'fresh sibling-only test root')
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
 def material(p,body=False,empty=False):
  need(p.is_absolute()and p.resolve(strict=True)==p,'physical worker file');fd=os.open(p,os.O_RDONLY|os.O_NOFOLLOW|os.O_NONBLOCK)
  try:
   a=os.fstat(fd);need(stat.S_ISREG(a.st_mode)and a.st_nlink==1 and (0<=a.st_size if empty else 0<a.st_size)and a.st_size<=134217728,'bounded single-link worker file');row={k:getattr(a,k)for k in F};h=hashlib.sha256();parts=[];size=0
   while b:=os.read(fd,65536):
    size+=len(b);need(size<=a.st_size,'file grew');h.update(b)
    if body:parts.append(b)
   need(size==a.st_size and row=={k:getattr(os.fstat(fd),k)for k in F}=={k:getattr(p.lstat(),k)for k in F},'worker full9/path changed');return dict(row,sha256=h.hexdigest()),b''.join(parts)if body else None
  finally:os.close(fd)
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
    row=rows[physical];expected=project.get(literal)or known.get(literal);actual=pinner(row);canonical=not literal.startswith(('/work/','/case/','/old/','/retained/'))or expected is not None and literal==physical
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
    if literal.startswith(('/old/','/retained/')):need(literal in known and literal==physical,'known old provider only')
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
 def admit_products(names,label,executable=False):
  need(all(str(out/name)not in products for name in names),'fresh products only');paths=['/outputs/'+n for n in names];initial,_=observe(paths);pins={p:pinner(initial[p])for p in paths}
  run(label+'-mode',['/bin/chmod','500'if executable else'400',*paths]);start=time.monotonic();previous=None;fixed=None;streak=0;since=0;n=0
  while True:
   rows,links=observe(paths);table={}
   for p in paths:
    host,_=material(out/p.removeprefix('/outputs/'));need(links[p]==p and pinner(rows[p])==pins[p]==pinner(host),'new product bytes known and stable');need(rows[p]['materialGNU9'][3]=='1'and int(rows[p]['materialGNU9'][4])==c['guestUid']and int(rows[p]['materialGNU9'][5])==c['guestGid'],'new product owner');table[p]={'hostFD9':host,'guestStatSha':rows[p]}
   n+=1;save(label+'-sample-'+str(n)+'.private.json',{'products':table,'elapsedMs':round((time.monotonic()-start)*1000)});identity={p:{k:v for k,v in r['hostFD9'].items()if k!='st_ctime_ns'}for p,r in table.items()}
   if fixed is None:fixed=identity
   need(identity==fixed,'new product eight-field identity changed');now=time.monotonic();need(now<min(deadline,start+30),'new product admission deadline')
   if table==previous:streak+=1
   else:streak=1;since=now
   if streak>=3 and now-since>=2:break
   previous=table;time.sleep(1.05)
  for p,r in table.items():products[str(out/p.removeprefix('/outputs/'))]=r
 def deps(name,target):
  row,body=material(out/name,True);need(row==products[str(out/name)]['hostFD9']and len(body)<=4194304,'retained dependency file');mt,sep,text=body.decode().replace('\\\n',' ').partition(':');need(sep and shlex.split(mt)==[target],'one exact dependency MT')
  values=sorted(set(posixpath.normpath(posixpath.join('/outputs',x))for x in shlex.split(text)));need(values and all(x.startswith('/')for x in values),'normalized dependency paths');return values
 def json_body(raw):
  def unique(pairs):
   d={}
   for k,v in pairs:need(k not in d,'duplicate JSON key');d[k]=v
   return d
  def invalid(_):raise ValueError('nonfinite JSON')
  return json.loads(raw,object_pairs_hook=unique,parse_constant=invalid)
 def same(a,b):return json.dumps(a,sort_keys=True,allow_nan=False)==json.dumps(b,sort_keys=True,allow_nan=False)
 def recipe(flags):
  need(type(flags)is list and flags and flags[0]=='/usr/bin/clang++'and all(type(x)is str and x and '\0'not in x for x in flags),'explicit compiler recipe')
  need(not any(x in flags for x in('-M','-MM','-MD','-MMD','-MF','-MT','-o','-c')),'one complete fresh unit recipe')
  need(not any('/case/include'in x for x in flags),'documentation header must not enter compiler include search')
 def compile_unit(name,source,flags,objname,core=False):
  recipe(flags);need(source in project and source.startswith(('/work/','/case/')),'known declared TU')
  obj='/outputs/'+objname;need(all(not os.path.lexists(out/n)for n in(objname+'.M',objname+'.d',objname)),'fresh object outputs')
  counts['CoreM'if core else'independentM']+=1;run(name+'-M',flags+['-M','-MF',obj+'.M','-MT',obj,source]);admit_products([objname+'.M'],name+'-M')
  names=deps(objname+'.M',obj);need(source in names and '/work/include/nano/chain.hpp'in names and '/case/include/nano/chain.hpp'not in names,'declared TU and original compiled header only');c['verifyConsumed'](names)
  for at in range(0,len(names),512):admit_inputs(names[at:at+512],name+'-M-input-'+str(at))
  counts['CoreCpp'if core else'cppCompilations']+=1;run(name+'-CXX',flags+['-MD','-MF',obj+'.d','-MT',obj,'-o',obj,'-c',source],300);admit_products([objname,objname+'.d'],name+'-object')
  actual=deps(objname+'.d',obj);c['verifyConsumed'](actual);need(actual==names,'literal M equals MD per TU')
  for at in range(0,len(actual),512):admit_inputs(actual[at:at+512],name+'-MD-input-'+str(at))
  need(sorted({aliases[p]for p in names})==sorted({aliases[p]for p in actual}),'canonical M equals MD per TU')
  mmd[name]={'normalizedMEqualsMd':True,'literalMEqualsMd':True,'dependencyCount':len({aliases[p]for p in names}),'target':obj,'source':source}
  return obj
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
 def loader(name,exe):
  counts['loaderChecks']+=1;raw,err=run(name+'-LDD',['/usr/bin/ldd',exe],30);need(not err and b'not found'not in raw,'loader resolved with empty stderr');closure=[]
  for line in raw.decode().splitlines():
   value=line.strip()
   if re.fullmatch(r'linux-vdso\.so\.[0-9]+ \(0x[0-9a-f]+\)',value):continue
   match=re.fullmatch(r'(?:[^ \t]+ => )?(/[^ \t]+) \(0x[0-9a-f]+\)',value);need(match is not None,'recognized absolute loader provider');closure.append(match[1])
  need(closure,'nonempty loader closure');admit_inputs(closure,name+'-loader-inputs');save(name+'-loader-closure-public.json',{'paths':closure,'canonicalPaths':{p:aliases[p]for p in closure}})
 try:
  core=c['coreUnit'];need(type(core)is dict and set(core)=={'source','compilerFlags','object','member'}and core['source']=='/case/src/chain.cpp'and core['object']==core['member']=='chain.cpp.o','one exact changed Core TU/member')
  required={'/case/src/chain.cpp':{'sha256':'bc0a192263a4192c7363f4a65f126586504cbb74aff1af6084e69face8b78b46','bytes':44702},'/case/src/main.cpp':{'sha256':'b4433387abb92445ca4b50540c83a7f515f2054281919c46ff2750c2bbbf6e70','bytes':8164},'/case/include/nano/chain.hpp':{'sha256':'d19297f3c55bb9f98ed8ab5cfdbd976951e100b41483e2968e90d4d44382628e','bytes':8250},'/case/tests/test_cli_readonly.cpp':{'sha256':'6431cdca3cb643c4f25de1ac4a1508432af05302d98e0365b208c685f01cb35c','bytes':11239},'/case/tests/qualification/cli_readonly_probe.cpp':{'sha256':'84a17dadd9aa80b0bbccc44cfdaff218e01938708651af4ff57c589953448f2e','bytes':19202}}
  need(all(project.get(p)==v for p,v in required.items()),'exact proposed C8 production and readonly test bytes')
  units=c['units'];cases=c['cases'];names=set();executables=set();allowed_names={'nano','readonly','cli-probe','chain','block-auth','bootstrap-startup','beacon-gate','recovery-audit'}
  need(type(units)is list and 3<=len(units)<=8 and type(cases)is list and 2<=len(cases)<=24,'bounded explicit C8 batch')
  for u in units:
   need(type(u)is dict and set(u)=={'name','source','compilerFlags','linkPrefix','linkTail','executable'}and u['name']in allowed_names and u['name']not in names,'closed unique unit role');names.add(u['name'])
   need(type(u['executable'])is str and re.fullmatch('[a-z][a-z0-9-]{0,63}',u['executable'])and u['executable']not in executables and u['executable']not in('libnano_core.a','chain.cpp.o'),'unique executable basename');executables.add(u['executable']);recipe(u['compilerFlags'])
   for k in('linkPrefix','linkTail'):need(type(u[k])is list and u[k]and all(type(p)is str and p and '\0'not in p for p in u[k]),'explicit unit link recipe')
   need(u['linkTail'].count('/outputs/libnano_core.a')==1 and not any(p==c['retainedCore']for p in u['linkPrefix']+u['linkTail']),'all leaves link exactly new Core archive')
  need({'nano','readonly','cli-probe'}<=names,'main/new Core-test/real CLI probe required');by_name={u['name']:u for u in units}
  source_roles={'nano':'/case/src/main.cpp','readonly':'/case/tests/test_cli_readonly.cpp','cli-probe':'/case/tests/qualification/cli_readonly_probe.cpp','chain':'/case/tests/test_chain.cpp','block-auth':'/case/tests/test_block_auth.cpp','bootstrap-startup':'/case/tests/test_capella_bootstrap_startup.cpp','beacon-gate':'/case/tests/test_beacon_execution_gate.cpp','recovery-audit':'/case/tests/test_recovery_audit.cpp'}
  need(all(u['source']==source_roles[u['name']]for u in units),'exact source per public unit role')
  need(by_name['nano']['source']=='/case/src/main.cpp'and by_name['readonly']['source']=='/case/tests/test_cli_readonly.cpp'and by_name['cli-probe']['source'].startswith('/case/'),'new main/Core-test/CLI probe source roles')
  case_names=set();covered=set()
  for case in cases:
   need(type(case)is dict and set(case)=={'name','unit','args','timeoutSeconds','oracle'}and type(case['name'])is str and re.fullmatch('[a-z][a-z0-9-]{0,63}',case['name'])and case['name']not in case_names and case['unit']in names-{'nano'},'closed unique non-main case');case_names.add(case['name']);covered.add(case['unit'])
   need(type(case['args'])is list and all(type(x)is str and '\0'not in x for x in case['args'])and type(case['timeoutSeconds'])is int and 1<=case['timeoutSeconds']<=240,'explicit bounded arguments')
   if case['unit']=='chain':need(type(case['oracle'])is dict and case['oracle'].get('kind')=='chain-count-line','chain count is source-derived, not a guessed exact value')
   if case['unit']=='readonly':need(case['args']==['--self-test'],'exact readonly Core selector')
   if case['unit']=='cli-probe':need(case['args']==['/outputs/'+by_name['nano']['executable'],'$NANO_SHA','$NANO_BYTES','$PROBE_ROOT'],'exact actual CLI-probe arguments')
   if case['unit']=='bootstrap-startup':need(case['args']==['--self-test'],'exact startup guard selector')
   if case['unit']in('chain','block-auth','beacon-gate'):need(case['args']==[],'exact existing default selector')
   if case['unit']=='recovery-audit':need(case['args']in[['Paris'],['Shanghai']],'only active recovery-audit profiles')
  need(covered==names-{'nano'},'every compiled test unit has a case')
  need(c['retainedCore']=='/retained/libnano_core.a'and c['retainedCore']in known and c['archiveTool']=='/usr/bin/ar','one known retained Core and GNU ar')
  tools=['/usr/bin/clang++',c['archiveTool'],'/bin/cp','/usr/bin/ldd','/bin/sh','/bin/mkdir','/usr/bin/realpath','/usr/bin/stat','/usr/bin/sha256sum','/usr/bin/find','/bin/chmod']
  case_inputs=sorted(p for p in project if p.startswith('/case/'));need(len(case_inputs)<=256 and project.get('/work/include/nano/chain.hpp')=={'sha256':'f0bdadfbc8aa1b0ca40086df0f03fa498fef3b05e12541d9a82213721bc2b9eb','bytes':8098},'bounded fresh case leaves and original compiled header pin')
  admit_inputs([*tools,*known,*case_inputs],'initial')
  obj=compile_unit('core-chain',core['source'],core['compilerFlags'],core['object'],True)
  need(not os.path.lexists(out/'libnano_core.a'),'fresh candidate Core archive');run('Core-copy',['/bin/cp','--',c['retainedCore'],'/outputs/libnano_core.a']);run('Core-writable',['/bin/chmod','600','/outputs/libnano_core.a'])
  counts['archiveUpdates']+=1;run('Core-replace',[c['archiveTool'],'r','/outputs/libnano_core.a',obj],60);admit_products(['libnano_core.a'],'Core-archive')
  archive_proof=c['proveArchiveSingleton'](c['retainedCore'],'/outputs/libnano_core.a',core['member'],obj)
  need(type(archive_proof)is dict and set(archive_proof)=={'passed','memberCount','changedMembers','unchangedMembers','replacedMember','retainedArchivePin','newArchivePin','objectPin'}and same({k:archive_proof[k]for k in('passed','memberCount','changedMembers','unchangedMembers','replacedMember')},{'passed':True,'memberCount':143,'changedMembers':1,'unchangedMembers':142,'replacedMember':'chain.cpp.o'}),'Root pure singleton143 proof')
  need(archive_proof['retainedArchivePin']==known[c['retainedCore']]and archive_proof['newArchivePin']==pinner(products[str(out/'libnano_core.a')]['hostFD9'])and archive_proof['objectPin']==pinner(products[str(out/core['object'])]['hostFD9']),'proof joins original/new/object bytes');save('core-singleton-proof-public.json',archive_proof)
  for u in units:
   obj=compile_unit(u['name'],u['source'],u['compilerFlags'],u['name']+'.o');exe='/outputs/'+u['executable'];need(not os.path.lexists(out/u['executable']),'fresh leaf executable')
   admit_inputs([p for p in u['linkPrefix']+u['linkTail']if p.startswith('/')],u['name']+'-absolute-link-recipe');counts['links']+=1;run(u['name']+'-LINK',u['linkPrefix']+[obj,'-o',exe]+u['linkTail']);admit_products([u['executable']],u['name']+'-executable',True);loader(u['name'],exe)
  c['beforeRun']();run('test-root',['/bin/sh','-ec','test ! -e "$1"; /bin/mkdir -m 700 -- "$1"; /bin/mkdir -m 700 -- "$1/tmp"','fresh-test-root',runroot],15)
  nano=products[str(out/by_name['nano']['executable'])]['hostFD9'];replacement={'$NANO_SHA':nano['sha256'],'$NANO_BYTES':str(nano['st_size']),'$PROBE_ROOT':runroot+'/synthetic-cli-readonly-'+runroot.rsplit('-',1)[-1]}
  for case in cases:
   c['beforeRun']();args=[replacement.get(x,x)for x in case['args']];counts['runs']+=1;raw,err=run('case-'+case['name'],['/outputs/'+by_name[case['unit']]['executable'],*args],case['timeoutSeconds'],testing=True);rows=oracle(raw,err,case['oracle']);case_results.append({'name':case['name'],'unit':case['unit'],'passed':True,'records':rows})
  completed=True
 except BaseException as exc:error(exc,'qualification')
 finally:
  if aliases:
   try:rows,links=observe(list(aliases),True);need(rows==guest and links==aliases,'all selected guest inputs/aliases unchanged')
   except BaseException as exc:error(exc,'guest-final-fence')
  for path,want in products.items():
   try:
    host,_=material(pathlib.Path(path));g='/outputs/'+pathlib.Path(path).name;rows,links=observe([g],True);need(host==want['hostFD9']and links[g]==g and rows[g]==want['guestStatSha'],'new product final host/guest fence')
   except BaseException as exc:error(exc,'product-final-fence:'+path)
  authority={'products':products,'guestStatSha':guest,'guestAliases':aliases,'guestFdIdentityVerified':False}
  try:save('final-observations.private.json',authority)
  except BaseException as exc:error(exc,'observation-publication')
  report={'format':'nano-cli-readonly-worker-v1','passed':first is None and completed,'counts':counts,'counterKind':'attemptedCommands','mmd':mmd,'archiveProof':archive_proof,'caseResults':case_results,'freshBuildProducts':len(products),'ownerPreparationSamples':owner_preparations,'typedOwnerCount':len(typed_owners),'firstFailure':first,'secondaryFailures':secondary,'steps':steps,'CppExitSource':'owned docker exec actual exit and closed host process group','keepaliveStateExitCodeIsCppExitCode':False,'semanticCommentOnlyCoreReuse':True,'oldHeaderCompiled':'core-chain'in mmd,'headerDocumentationConsumed':False,'strictE9SelectorClaimed':False,'retainedCoreObjects':142,'changedCoreObjects':1,'CoreCleanBuildQualified':False,'newImages':0,'runtimeDutiesAuthorized':False,'productionQualified':False}
  try:save('result-public.json',report)
  except BaseException as exc:error(exc,'result-publication');report['passed']=False;report['firstFailure']=first
 return report,authority
