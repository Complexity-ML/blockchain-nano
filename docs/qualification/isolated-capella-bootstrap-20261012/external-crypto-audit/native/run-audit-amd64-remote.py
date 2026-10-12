from pathlib import Path
import hashlib,json,os,stat,subprocess,time
ROOT=Path(__file__).parent
BUILD=ROOT/'build'
OUTPUT=ROOT/'runtime'
FIXTURE=Path('/home/ubuntu/nanobees-qualification/isolated-fixture-exporter-amd64-public-copy-repair-20261012/runtime-parent/private/export-v1')
IMAGE='fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14'
VOLUME='nano-clean-rpc-build-20261002'
PARENT='/build/coherent-e33ad89e-amd64'
CORE='/build/publication-cancel-boundary-amd64-20261007/libnano_core.a'
CORE_SHA='783ccae2127905741238dd58618124f9582aae380625ef3fe592523a58a056dd'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
binary=BUILD/'outputs/private-binding-audit'
build=json.loads((BUILD/'binary-manifest.json').read_text())
assert sha(binary)==build['helperSha256'] and build['sourceSha256']=='25223c9adc0ca0afffa01a40ef5e0dcc9eee5cb4fc81d23daee65d77ef99a312'
OUTPUT.mkdir(mode=0o700,exist_ok=False)
def inventory(root):
    records=[]
    for p in sorted(root.rglob('*')):
        s=p.lstat();assert not stat.S_ISLNK(s.st_mode)
        records.append({'path':str(p.relative_to(root)),'uid':s.st_uid,'gid':s.st_gid,'mode':s.st_mode&0o7777,
                        'bytes':s.st_size,'mtimeNs':s.st_mtime_ns,'sha256':sha(p) if p.is_file() else None})
    return records
before=inventory(FIXTURE)
prefix=['sudo','-n','docker','run','--rm','--pull=never','--platform','linux/amd64','--network','none',
        '--read-only','--cap-drop','ALL','--security-opt','no-new-privileges','--user','1000:1000',
        '--cpus','1','--memory','2g','--memory-swap','2g','--pids-limit','128',
        '--mount','type=volume,source='+VOLUME+',target=/build,readonly','--workdir',PARENT]
parents=json.loads((BUILD/'parent-hashes-before.json').read_text())
def current_parents():
    result=subprocess.run([*prefix,IMAGE,'sha256sum',*parents],capture_output=True,text=True,check=True)
    return dict((path,digest) for digest,path in (line.split('  ',1) for line in result.stdout.splitlines()))
assert current_parents()==parents and parents[CORE]==CORE_SHA
args=[*prefix,'--mount','type=bind,source='+str(BUILD/'outputs')+',target=/audit,readonly',
      '--mount','type=bind,source='+str(FIXTURE)+',target=/fixture,readonly',
      IMAGE,'/audit/private-binding-audit','/fixture']
(OUTPUT/'runtime-command.json').write_text(json.dumps({'args':args,'argc':2,'journalOutputArgument':False,
                                                      'privateContentsInArguments':False},indent=2)+'\n')
start=time.monotonic()
with (OUTPUT/'audit.log').open('w') as output:
    result=subprocess.run(args,stdout=output,stderr=subprocess.STDOUT,timeout=120,
                          env={k:v for k,v in os.environ.items() if not k.startswith('NANO_')})
elapsed=time.monotonic()-start
after=inventory(FIXTURE)
current=current_parents()
report={'format':'nano-amd64-private-binding-audit-runtime-v1','exitCode':result.returncode,'elapsedSeconds':elapsed,
        'helperSha256':sha(binary),'helperSourceSha256':build['sourceSha256'],'qualifiedCoreSha256':CORE_SHA,
        'imageId':'sha256:'+IMAGE,'architecture':'amd64','fixtureInputsUnchanged':before==after,
        'parentFilesUnchanged':current==parents,'parentsReadOnly':True,'fixtureReadOnly':True,
        'privateFilesTransferred':False,'privateContentsRecorded':False,'seedRead':False,'journalsCreated':0,
        'newSignaturesCreated':0,'signerCallbacksInvoked':0,'runtimeStarted':False,
        'networkQualified':False,'productionQualified':False,'auditLogSha256':sha(OUTPUT/'audit.log')}
(OUTPUT/'run-receipt.json').write_text(json.dumps(report,indent=2)+'\n')
fd=os.open(OUTPUT/'input-bindings.private.json',os.O_WRONLY|os.O_CREAT|os.O_EXCL,0o600)
with os.fdopen(fd,'w') as output:
    output.write(json.dumps({'format':'nano-readonly-amd64-private-input-bindings-v1','sourceDirectory':str(FIXTURE),
                            'before':before,'after':after,'privateInputContentsCopied':False,'seedRead':False},indent=2)+'\n')
    output.flush();os.fsync(output.fileno())
assert before==after and current==parents and result.returncode==0
actual=json.loads((OUTPUT/'audit.log').read_text())
assert actual['passed'] and actual['inputManifestSha256']==sha(FIXTURE/'isolated-domain.json')
assert actual['validatorPrivatePublicBindings']==65 and actual['bootstrapBlsSignaturesVerified']==4
assert actual['newSignaturesCreated']==0 and actual['signerCallbacksInvoked']==0 and not actual['runtimeStarted']
assert 'offlineJournalPrimariesCreated' not in actual
print(json.dumps(report),flush=True)
print(json.dumps(actual),flush=True)
