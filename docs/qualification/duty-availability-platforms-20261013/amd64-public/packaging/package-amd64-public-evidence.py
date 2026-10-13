#!/usr/bin/env python3
"""Copy explicitly allowlisted public compiler/test evidence without historical edits."""
from pathlib import Path
import json,hashlib,os,stat
BASE=Path(__file__).parent
OLD=BASE/'duty-availability-coherent-amd64-20261013'
PORT=BASE/'duty-availability-portability-amd64-20261013'
DEST=BASE/'duty-availability-amd64-final-public-evidence-20261013'
sha=lambda b:hashlib.sha256(b).hexdigest()
def read(p):
 s=p.lstat();assert stat.S_ISREG(s.st_mode) and not p.is_symlink()
 b=p.read_bytes();assert p.stat().st_ino==s.st_ino and len(b)==s.st_size
 return b
old_manifest=read(OLD/'public-file-manifest.json')
assert sha(old_manifest)=='01696b994cd98186dcd2df8ae1ad30fe958db299345a80e40b42dbb977b0a526'
old=json.loads(old_manifest)
port_manifest=read(PORT/'remote-public-transfer-manifest.json')
assert sha(port_manifest)=='e1226b0a5a626a6bf0ac5e33417dc004cd81b6021950e9fbd780eea1a9556939'
assert not DEST.exists();DEST.mkdir(mode=0o700)
rows=[]
def copy(p,name,expected=None):
 b=read(p)
 if expected is not None:assert sha(b)==expected['sha256'] and len(b)==expected['bytes']
 assert Path(name).is_relative_to(Path('.')) and '..' not in Path(name).parts
 assert Path(name).suffix not in {'.o','.a','.so','.dylib','.tar','.gz','.ssz','.env'}
 q=DEST/name;q.parent.mkdir(parents=True,exist_ok=True)
 fd=os.open(q,os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,0o600)
 with os.fdopen(fd,'wb') as f:f.write(b)
 rows.append({'path':name,'bytes':len(b),'sha256':sha(b),'sourcePath':str(p)})
old_rows=old['files']
assert len(old_rows)==209
for r in old_rows:copy(OLD/r['path'],'original-coherent-amd64/'+r['path'],r)
for name in ['public-file-manifest.json','duty-availability-coherent-amd64-driver-20261013.py','canonical-arm-production-reference.json','upload-public-controller-receipt.json']:
 copy(OLD/name,'original-coherent-amd64/'+name)
for r in json.loads(port_manifest)['files']:copy(PORT/r['path'],'portable-test-only/'+r['path'],r)
for name in ['remote-public-transfer-manifest.json','public-transfer-validation.json','duty-availability-portability-amd64-driver-20261013.py','upload-controller-public-receipt.json']:
 copy(PORT/name,'portable-test-only/'+name)
copy(BASE/'collect-amd64-portability-public.py','packaging/collect-amd64-portability-public.py')
copy(Path(__file__),'packaging/package-amd64-public-evidence.py')
source=Path('/Users/boris/Dev/blockchain-nano/tests/test_capella_duty_availability.cpp')
assert sha(read(source))=='44febdb90047ea8a7de5e78002d73ed96b82cc67259dd356e98c7ba096f6647d'
copy(source,'portable-test-only/source-snapshot/test_capella_duty_availability.cpp')
def generated(name,data):
 b=data.encode();q=DEST/name;q.parent.mkdir(parents=True,exist_ok=True);q.write_bytes(b)
 rows.append({'path':name,'bytes':len(b),'sha256':sha(b),'sourcePath':'generated-public-description'})
generated('portable-test-only/observed-session-receipt.json',json.dumps({
 'format':'nano-observed-public-portability-exec-receipt-v1',
 'uploadOriginalSession':57053,'uploadTerminalChunk':'6e38f7','uploadExitCode':0,
 'driverSha256':'2882430a42033bdc8316efcebe2332247756d372b8878df825f82ae3a85ed796',
 'compileLinkOriginalSession':36813,'launchChunk':'411f8e','terminalChunk':'bdf30c','exitCode':0,
 'collectionTerminalChunk':'f3d20c','collectionExitCode':0,
 'result':'compile0/link0; object and ELF identical; no test rerun',
 'privateRecoveryDataExported':False},indent=2)+'\n')
generated('README.md','''# AMD64 coherent qualification and one-line test portability

This is a new public evidence package. Historical manifests and results remain unchanged.

The original coherent build uses clean Git revision c672a30dbdc7b43ab04186db0e54be78510e6789, the existing pinned AMD64 image fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14, and a new output directory proved empty before CMake. All 139 canonical core source units were compiled, and the archive contains exactly their 139 object members. The original compile database, generated compiler dependencies, flags and link commands were captured before test or reconfiguration. The eleven ignored MCL miscellaneous files were not consumed according to the actual dependency/command/link audit; this is not a dependency-image requalification.

The following limited selections passed: the new registered availability aggregate (three internal scenarios), existing local-sync CTest, direct prepared-block regression, and pure stopped-capture guard CTest (25 cases). Actual CTest logs and genuine JUnit files are retained. The direct prepared-block invocation has a log and exit receipt, not fabricated JUnit. The Linux fault library and all eight parent program artifacts were pinned before tests and remained identical after them.

The three new scenarios reported cumulative elapsed times 431, 1060 and 33296 ms; maximum RSS 16640, 22784 and 32512 KiB. The third used a deterministic synthetic fixture with 48 normally admitted peer blocks and 144 synthetic fixture signatures, zero local sync provider calls, and an available new-parent preparation at slot 384. These are not campaign duties or a live network. The internal 60-second and 128-MiB limits were unchanged. The legacy flat-RSS refusal remains open; the storage-only cold envelope cache was bounded to two records and 1 MiB, while decoded private-cache usage was not directly observed.

The portable one-line test fix at Git d7cc1bdb63f6d5035adc6f4b2420823f135f1bf1 changes only an initializer-list type to a size_t array with explicit conversion. The new test source SHA is 44febdb90047ea8a7de5e78002d73ed96b82cc67259dd356e98c7ba096f6647d. Only that translation unit and its executable were compiled/linked in a separate output directory against the original full coherent build mounted read-only. The 139 core objects and other parents were preserved by SHA, inode, ownership, mode and modification/change timestamps. Compilation took 15.864 seconds and linking 0.617 seconds.

Both old/new test object SHA 09b90fb2ebca602f2ef4fcf5f33f7f94e6c4a6a4ccd4b686b2acd87558b9e97a and executable SHA 32843ec2935a25797b98c5de6fbe45e6ce24f03214c753d23199023d22ef0945 are byte-identical. This comparison was recorded before the execution decision. The original three-scenario execution is therefore reused; neither the aggregate nor old regressions were rerun for the portability fix. No CMake/core/main rebuild was requested in the portability phase.

The original archive SHA is 1b5a867398bed07bbaca0994556d8948cbb2750ac333c30070e627f101aeecdb; nano SHA is 31887383452c05dec078745f02ff3be21d2579ff716c1b2b3b4c0faa35b52c18. The package contains public source/compiler/test receipts and metadata only. Compiled binaries, objects, libraries, generated synthetic scratch fixtures, databases, private capsules, seeds, keys and environments are excluded. The four separately pending private VPS recovery reports were neither read nor exported by this lot.

This qualification does not establish live restored validator duties, common-finality progress, a public deployment, full CI, dependency-image requalification or production readiness. Root delivery and any independent review are separate evidence. No historical test or qualification is promoted beyond its recorded scope.
''')
manifest={'format':'nano-public-amd64-coherent-and-portability-evidence-v1','files':rows,'payloadCount':len(rows),'payloadBytes':sum(x['bytes'] for x in rows),'original209ManifestPreservedSha256':sha(old_manifest),'port15TransferManifestPreservedSha256':sha(port_manifest),'privateVpsRecoveryDataExported':False,'compiledArtifactPayloadsIncluded':False,'productionQualified':False}
out=(json.dumps(manifest,indent=2)+'\n').encode();(DEST/'manifest.json').write_bytes(out)
assert all(sha(read(DEST/x['path']))==x['sha256'] for x in rows)
print(json.dumps({'path':str(DEST),'manifestSha256':sha(out),'payloadCount':len(rows),'payloadBytes':manifest['payloadBytes'],'allCopiesHashVerified':True}))
