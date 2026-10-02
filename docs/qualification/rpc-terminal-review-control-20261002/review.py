import gzip, hashlib, json, math, sys, xml.etree.ElementTree as E
from pathlib import Path
REVISION = 'd1597d9a6987d0839ed2f0c7ded268aead35d071'
OLD_PREFIX = '/build/full-coherent-0f53b42d-arm64-20261002'

def require(condition, message):
    if not condition:
        raise ValueError(message)

def read_json(path):
    data = path.read_bytes()
    return json.loads(gzip.decompress(data) if path.suffix == '.gz' else data)

def normalized(tests, prefix, old=False):
    def convert(value):
        if isinstance(value, str):
            if old and value.startswith('/build/network/'):
                return '/BUILD/network/' + value[len('/build/network/'):]
            if value == prefix or value.startswith(prefix + '/'):
                return '/BUILD' + value[len(prefix):]
            return value
        if isinstance(value, list): return [convert(v) for v in value]
        if isinstance(value, dict): return {k:convert(v) for k,v in value.items()}
        return value
    return {t['name']:convert({'command':t['command'], 'properties':t['properties']}) for t in tests}

def review(reports, reference, baseline, prefix):
    require((reports/'source-revision.txt').read_text().strip() == REVISION, 'wrong source revision')
    require(len(reference) == 15315, 'unexpected source inventory')
    for name in ['source-manifest.json', 'source-manifest-after-build.json', 'source-manifest-after-tests.json']:
        require(read_json(reports/name) == reference, 'source drift: '+name)
    require(reference['CMakeLists.txt'] == baseline['source']['CMakeLists.txt'], 'baseline registry belongs to different CMake source')
    registry = read_json(reports/'registry.json')
    require(registry == read_json(reports/'registry-before-tests.json'), 'registry changed during campaign')
    tests = registry['tests']; names = [t['name'] for t in tests]
    require(len(names) == len(set(names)) == 427, 'missing/duplicate registry test')
    require(normalized(tests,prefix) == normalized(baseline['registry']['tests'], OLD_PREFIX, True), 'commands/properties differ from unchanged source baseline')
    manifests = [read_json(reports/n) for n in ['executables.json','executables-before-tests.json','executables-after-tests.json']]
    programs = manifests[0]
    require(all(m == programs for m in manifests), 'executable drift')
    require(all((p.startswith(prefix+'/') or p == '/usr/bin/cmake') and isinstance(h,str) and len(h)==64 and all(c in '0123456789abcdef' for c in h) for p,h in programs.items()), 'invalid/out-of-build executable')
    for p in [prefix+'/nano', prefix+'/network/nano-p2p', prefix+'/network/nano-altair-probe']:
        require(p in programs, 'missing coherent runtime/helper: '+p)
    for t in tests:
        props = {p['name']:p['value'] for p in t['properties']}
        require(len(props) == len(t['properties']), 'duplicate property')
        require(not any(k in props for k in ['DISABLED','SKIP_RETURN_CODE','SKIP_REGULAR_EXPRESSION']), 'skip property')
        timeout = props.get('TIMEOUT')
        require(type(timeout) in (int,float) and math.isfinite(timeout) and timeout>0, 'invalid timeout')
        require(t['command'][0] in programs, 'missing test executable')
        for arg in t['command'][1:]:
            if arg.startswith('/') and Path(arg).name in ['nano','nano-altair-probe','nano-finality-crash-fixture','nano-p2p']:
                require(arg in programs, 'missing helper binding')
    suite = E.parse(reports/'results.xml').getroot(); cases = suite.findall('testcase')
    require(suite.tag == 'testsuite' and int(suite.get('tests','-1')) == 427, 'invalid JUnit suite')
    actual = [c.get('name') for c in cases]
    require(len(actual)==len(set(actual))==427 and set(actual)==set(names), 'missing/duplicate/foreign JUnit case')
    require(all(int(suite.get(k,'0'))==0 for k in ['failures','errors','skipped','disabled']), 'nonpassing suite')
    require(all(c.get('status')=='run' and not any(c.findall(k) for k in ['failure','error','skipped']) for c in cases), 'nonpassing/nonexecuted case')
    for name in ['ctest-exit.txt','post-test-manifest-exit.txt','audit-exit.txt']:
        require((reports/name).read_text().strip()=='0', 'nonzero terminal status: '+name)
    return {'revision':REVISION,'registered':427,'reported':427,'sources':len(reference),'programs':len(programs),'unchangedSourceBaselineCommandsAndOriginalTimeouts':True,'sourceAndProgramManifestsStable':True,'liveExecutableHashesRechecked':False,'staticLinkInputsAudited':False,'exactEestVariantsAudited':False,'networkSupplementAudited':False,'wanStarted':False,'productionQualified':False}

if __name__ == '__main__':
    try:
        reports, ref, old_source, old_registry, prefix = sys.argv[1:]
        result = review(Path(reports), read_json(Path(ref)), {'source':read_json(Path(old_source)), 'registry':read_json(Path(old_registry))}, prefix)
        print(json.dumps(result,indent=2))
    except Exception as error:
        print(str(error),file=sys.stderr); sys.exit(1)
