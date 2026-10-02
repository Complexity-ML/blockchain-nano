import subprocess, copy, importlib.util, json, tempfile, unittest, xml.etree.ElementTree as E
from pathlib import Path
spec=importlib.util.spec_from_file_location('review',Path(__file__).with_name('review.py')); m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
REPO=Path('/Users/boris/Dev/blockchain-nano')
BASE=REPO/'docs/qualification/full-arm-terminal-0f53b42d-20261002'
REF=m.read_json(Path(__file__).with_name('source-manifest.json.gz'))
BASELINE={'source':m.read_json(BASE/'source-before.json.gz'),'registry':m.read_json(BASE/'registry.json.gz')}
LIVE=Path('/tmp/nano-rpc-full-arm64-reports-20261002')
PREFIX='/build/rpc-full-d1597d9a-arm64'
class ReviewControls(unittest.TestCase):
    def setUp(self):
        self.tmp=tempfile.TemporaryDirectory();self.p=Path(self.tmp.name)
        self.write('source-revision.txt',m.REVISION+'\n')
        for n in ['source-manifest.json','source-manifest-after-build.json','source-manifest-after-tests.json']:self.write(n,REF)
        for n in ['registry.json','registry-before-tests.json']:self.write(n,m.read_json(LIVE/'registry.json'))
        for n in ['executables.json','executables-before-tests.json','executables-after-tests.json']:self.write(n,m.read_json(LIVE/'executables.json'))
        root=E.Element('testsuite',tests='427',failures='0',errors='0',skipped='0',disabled='0')
        for t in BASELINE['registry']['tests']:E.SubElement(root,'testcase',name=t['name'],status='run')
        E.ElementTree(root).write(self.p/'results.xml')
        for n in ['ctest-exit.txt','post-test-manifest-exit.txt','audit-exit.txt']:self.write(n,'0\n')
    def tearDown(self):self.tmp.cleanup()
    def write(self,n,v): (self.p/n).write_text(json.dumps(v) if not isinstance(v,str) else v)
    def check(self):return m.review(self.p,REF,BASELINE,PREFIX)
    def reject(self):
        with self.assertRaises((ValueError,FileNotFoundError)):self.check()
    def mutate_json(self,n,fn):
        v=m.read_json(self.p/n);fn(v);self.write(n,v)
    def test_synthetic_positive(self):self.assertFalse(self.check()['productionQualified'])
    def test_missing_terminal(self):(self.p/'audit-exit.txt').unlink();self.reject()
    def test_wrong_revision(self):self.write('source-revision.txt','0'*40);self.reject()
    def test_source_drift(self):self.mutate_json('source-manifest-after-tests.json',lambda v:v.update({'src/evm.cpp':'0'*64}));self.reject()
    def test_program_drift(self):self.mutate_json('executables-after-tests.json',lambda v:v.update({PREFIX+'/nano':'0'*64}));self.reject()
    def test_stale_helper(self):
        for n in ['executables.json','executables-before-tests.json','executables-after-tests.json']:
            self.mutate_json(n,lambda v:v.update({'/build/network/nano-altair-probe':'0'*64}))
        self.reject()
    def test_missing_probe(self):
        for n in ['executables.json','executables-before-tests.json','executables-after-tests.json']:
            self.mutate_json(n,lambda v:v.pop(PREFIX+'/network/nano-altair-probe'))
        self.reject()
    def test_raised_timeout(self):
        def change(v):next(p for p in v['tests'][0]['properties'] if p['name']=='TIMEOUT')['value']=999
        for n in ['registry.json','registry-before-tests.json']:self.mutate_json(n,change)
        self.reject()
    def test_skip_property(self):
        for n in ['registry.json','registry-before-tests.json']:
            self.mutate_json(n,lambda v:v['tests'][0]['properties'].append({'name':'SKIP_RETURN_CODE','value':77}))
        self.reject()
    def test_missing_case(self):
        r=E.parse(self.p/'results.xml');r.getroot().remove(r.getroot().find('testcase'));r.write(self.p/'results.xml');self.reject()
    def test_duplicate_case(self):
        r=E.parse(self.p/'results.xml');r.getroot().append(copy.deepcopy(r.getroot().find('testcase')));r.write(self.p/'results.xml');self.reject()
    def test_unexecuted_case(self):
        r=E.parse(self.p/'results.xml');r.getroot().find('testcase').set('status','notrun');r.write(self.p/'results.xml');self.reject()
    def test_hidden_failure(self):
        r=E.parse(self.p/'results.xml');E.SubElement(r.getroot().find('testcase'),'failure');r.write(self.p/'results.xml');self.reject()
    def test_nonzero_ctest(self):self.write('ctest-exit.txt','8\n');self.reject()
    def test_registry_changed(self):self.mutate_json('registry-before-tests.json',lambda v:v['tests'].pop());self.reject()
class ShellPrefixControls(unittest.TestCase):
    def invoke(self,prefix):
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp);output=root/'audit'
            result=subprocess.run(['bash',str(Path(__file__).with_name('run-review.sh')),tmp,tmp,str(output),prefix,'sha256:'+'a'*64,'existing-volume',tmp],capture_output=True,text=True)
            return result.returncode,output.exists(),result.stderr
    def test_current_paths_reach_full_report_gate(self):
        for arch in ['arm64','amd64']:
            code,created,error=self.invoke('/build/rpc-full-d1597d9a-'+arch)
            self.assertEqual(code,1);self.assertTrue(created)
    def test_old_path_refused_before_review(self):
        code,created,_=self.invoke('/build/full-network-9e9ebed3-arm64-20261002')
        self.assertNotEqual(code,0);self.assertFalse(created)
if __name__=='__main__':unittest.main(verbosity=2)
