"""Root assembler, SOURCE ONLY until separately admitted. No subprocess/network/project imports.
External inputs: SELF_SHA SELF_BYTES AMD_RESULT_SHA AMD_RESULT_BYTES
AMD_TERMINAL_SHA AMD_TERMINAL_BYTES fresh-output-component. Never writes repository.
"""
from pathlib import Path
import hashlib, io, json, os, re, stat, sys, tarfile
E = Path(__file__).resolve().parent
R = Path('/Users/boris/Dev/blockchain-nano')
BASE = '801d793f0f05c3b46f97d34d86973f1487e36561'
Q = 'docs/qualification/rpc-header-selection-20261018/'
S = E/'rpc-header-selection-source-proposal-20261018'
D = E/'rpc-header-selection-delivery-proposal-20261018'
ARM = E/'rpc-header-selection-arm-root-20261018-02'
AUDIT = E/'rpc-header-selection-arm-scoped-reuse-audit-root-20261018-01'
AMD = E/'rpc-header-selection-amd-public-readback-root-20261018-02'
TERMINAL = E/'rpc-header-selection-amd-invocation-root-20261018-02/terminal-public.json'
IMAGE = 'sha256:fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14'
CANONICAL = '43ce557522d6a3a7ee4ccff548e98a941fab98221a505bb534fc78d64cc0c54d'
ARMCORE = {'sha256':'65c91631ffe93515e500fd1eeacb4e6fd784fa04d431a193c40ec14733077938','bytes':41306456}
UNITS = ('rpc-state-read-cost','rpc-state-block-id','rpc-get-proof')
DOCS = ('PRODUCTION-GOAL','VALIDATOR-EIP-ROADMAP','EVM-PRODUCTION-EIP-ROADMAP','EIP-IMPLEMENTATION-STATUS')
MARKER = 'nano-rpc-header-selection-20261018'
PLACEHOLDER = '**AMD02 : lancée, résultat terminal en attente — [DRAFT_AMD_TERMINAL_RECEIPTS_REQUIRED].**'
CODE = {'include/nano/storage.hpp','src/storage.cpp','include/nano/chain.hpp','src/chain.cpp','src/chain_beacon.cpp','src/rpc.cpp','tests/test_rpc_state_read_cost.cpp','tests/test_rpc_state_block_id.cpp','tests/test_rpc_get_proof.cpp','CMakeLists.txt'}
# Known named public inputs are filled by the source author, never learned from a future run.
KNOWN = {
  "rpc-header-selection-amd-preparation-failure-public-readback-root-20261018-01/failure-public.json": {
    "bytes": 262,
    "sha256": "309400bef6c21977cfc446053fbc6f9d60aa8519a10c4d1526ece7eca4e65507"
  },
  "rpc-header-selection-amd-preparation-failure-public-readback-root-20261018-01/ssh.stderr.log": {
    "bytes": 89,
    "sha256": "b15dc020f7df303c61f39571dfc3aa8080c84415ad76bd42efe07c2a979bb281"
  },
  "rpc-header-selection-amd-preparation-failure-public-readback-root-20261018-02/preparation-failure-public.json": {
    "bytes": 267,
    "sha256": "facbdd236735bdf06282127f0650e27b6346a1370249699719613ca7783107e2"
  },
  "rpc-header-selection-arm-root-20261018-01/result-public.json": {
    "bytes": 1036,
    "sha256": "78162f5e672bf1f93e11c23c011875f0acaf164c5703d83494a1e6a8c40ceb2e"
  },
  "rpc-header-selection-arm-root-20261018-02/host-final-fence-public.json": {
    "bytes": 7447,
    "sha256": "571b005b206d952c4476e96ba1679c2594a1a9bf079d02e1cd0b18e1399e4cfa"
  },
  "rpc-header-selection-arm-root-20261018-02/outputs/CXX-rpc-get-proof.exit.json": {
    "bytes": 74,
    "sha256": "5b449811ad7e88a3cd33812801e1ed67b91edb95d65ec4ba73079b219c101318"
  },
  "rpc-header-selection-arm-root-20261018-02/outputs/CXX-rpc-state-block-id.exit.json": {
    "bytes": 78,
    "sha256": "0e79c576dbd2c7ba044d9674ee87708fee984b327faf727e664a68c8e4733779"
  },
  "rpc-header-selection-arm-root-20261018-02/outputs/CXX-rpc-state-read-cost.exit.json": {
    "bytes": 79,
    "sha256": "6f08d130e062ca11a6909a5e655a52f53fca1f9dd033ccdd87a51e9632762ac6"
  },
  "rpc-header-selection-arm-root-20261018-02/outputs/LDD-rpc-get-proof.exit.json": {
    "bytes": 71,
    "sha256": "7aa3d15e96941b706a5ef9ddc7735ac0d12746210ee32c22439ef085d5196dc1"
  },
  "rpc-header-selection-arm-root-20261018-02/outputs/LDD-rpc-get-proof.stderr.log": {
    "bytes": 0,
    "sha256": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
  },
  "rpc-header-selection-arm-root-20261018-02/outputs/LDD-rpc-get-proof.stdout.log": {
    "bytes": 1371,
    "sha256": "ec1391ab34d5b35d0651e191be07cdfc81e0bd1c3a3063f28830a4e742cc6f32"
  },
  "rpc-header-selection-arm-root-20261018-02/outputs/LDD-rpc-state-block-id.exit.json": {
    "bytes": 76,
    "sha256": "30d8e1b75041d87778f5ccdb6a1d76df66ab92507b6e923fabfefb9626f890e8"
  },
  "rpc-header-selection-arm-root-20261018-02/outputs/LDD-rpc-state-block-id.stderr.log": {
    "bytes": 0,
    "sha256": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
  },
  "rpc-header-selection-arm-root-20261018-02/outputs/LDD-rpc-state-block-id.stdout.log": {
    "bytes": 1371,
    "sha256": "df4a0d165519511bb74e8e7d43d1647fabe0724e0db02d7e40bb40d15b7a1271"
  },
  "rpc-header-selection-arm-root-20261018-02/outputs/LDD-rpc-state-read-cost.exit.json": {
    "bytes": 77,
    "sha256": "4a1e0d3c593e40ca9c6d6d2cfe5bfed09f551621fb9a2a62b82171d9f5be33d7"
  },
  "rpc-header-selection-arm-root-20261018-02/outputs/LDD-rpc-state-read-cost.stderr.log": {
    "bytes": 0,
    "sha256": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
  },
  "rpc-header-selection-arm-root-20261018-02/outputs/LDD-rpc-state-read-cost.stdout.log": {
    "bytes": 1371,
    "sha256": "a169c18137bf8fe59e5d1812c099958813dcd78fe031d04e6ca9a2bfb2fb47c1"
  },
  "rpc-header-selection-arm-root-20261018-02/outputs/LINK-rpc-get-proof.exit.json": {
    "bytes": 73,
    "sha256": "b3763ceda71569a58b238174f234f580d73e86ef13e35767413292458d5c30ac"
  },
  "rpc-header-selection-arm-root-20261018-02/outputs/LINK-rpc-state-block-id.exit.json": {
    "bytes": 78,
    "sha256": "ab36554adcc443d31b7217e2b476554f0ccd6c99947e95f663effd4b39ef91f2"
  },
  "rpc-header-selection-arm-root-20261018-02/outputs/LINK-rpc-state-read-cost.exit.json": {
    "bytes": 79,
    "sha256": "ddb63d6070fc2e59f291bcaae4253950a1db5ceba95c2d1c5a0b2b1ae1994e5b"
  },
  "rpc-header-selection-arm-root-20261018-02/outputs/M-rpc-get-proof.exit.json": {
    "bytes": 70,
    "sha256": "396ccdc0d7e4323d30c5f2ddee928011c2c61e974e60dd7c0462bb585a8a9d47"
  },
  "rpc-header-selection-arm-root-20261018-02/outputs/M-rpc-state-block-id.exit.json": {
    "bytes": 75,
    "sha256": "4f0a7bf4f5dea8004d62b60fc6e10378070bb4e2abc617eb622be0ceb0c2d140"
  },
  "rpc-header-selection-arm-root-20261018-02/outputs/M-rpc-state-read-cost.exit.json": {
    "bytes": 76,
    "sha256": "9ae7e686cfeea42b0ec6d1448bebc5599e682d7c64570cf77c7e3326b486ffe4"
  },
  "rpc-header-selection-arm-root-20261018-02/outputs/case-rpc-get-proof-self-test-public.json": {
    "bytes": 832,
    "sha256": "bae1c451ca7669160bab2f5f6bd35d3f3e26576e1ce12e4c376d6c752f964d8d"
  },
  "rpc-header-selection-arm-root-20261018-02/outputs/case-rpc-state-block-id-self-test-public.json": {
    "bytes": 403,
    "sha256": "009b6a8f30ad2eb141328586385a3722b0496bf760a5f9b9133e4d247cb11a5b"
  },
  "rpc-header-selection-arm-root-20261018-02/outputs/case-rpc-state-read-cost-self-test-public.json": {
    "bytes": 405,
    "sha256": "bae0e60582aba3294b4da3288e72f9f121330aa8f073a3520dbd23c6538ab976"
  },
  "rpc-header-selection-arm-root-20261018-02/outputs/core-member-proof.json": {
    "bytes": 8693,
    "sha256": "b96735ba1f7648f068b193144045112edd0eb044668bf425a6bbf2f8d785bd5d"
  },
  "rpc-header-selection-arm-root-20261018-02/outputs/core-recompile-selection-public.json": {
    "bytes": 40915,
    "sha256": "4ccac7847d045e4953583266f631728bf2743f47efe5d6a69bb6d2ef1d82e4f4"
  },
  "rpc-header-selection-arm-root-20261018-02/outputs/result-public.json": {
    "bytes": 221120,
    "sha256": "db35f54b5e586111b7ebf47156325cc8b6c6842652660a04433c4117c48a2b65"
  },
  "rpc-header-selection-arm-root-20261018-02/outputs/test-rpc-get-proof-self-test.exit.json": {
    "bytes": 85,
    "sha256": "aec7d0379a59625025fd40bb28bc3639504d1b29ec633aa7c104ca853f41b9d9"
  },
  "rpc-header-selection-arm-root-20261018-02/outputs/test-rpc-get-proof-self-test.stderr.log": {
    "bytes": 0,
    "sha256": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
  },
  "rpc-header-selection-arm-root-20261018-02/outputs/test-rpc-get-proof-self-test.stdout.log": {
    "bytes": 41688,
    "sha256": "6140a2c49bd37114645f9652400dc0a5543ccaf5c18b66f00e5cbdd1a3a1ffd5"
  },
  "rpc-header-selection-arm-root-20261018-02/outputs/test-rpc-state-block-id-self-test.exit.json": {
    "bytes": 88,
    "sha256": "2d46af71b032190c91ae6c26985f6d1c98e798b289e85e592f9940ca4993de6c"
  },
  "rpc-header-selection-arm-root-20261018-02/outputs/test-rpc-state-block-id-self-test.stderr.log": {
    "bytes": 0,
    "sha256": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
  },
  "rpc-header-selection-arm-root-20261018-02/outputs/test-rpc-state-block-id-self-test.stdout.log": {
    "bytes": 727,
    "sha256": "7c4628a5cd30f4094b7b560a1ed7ecde1f6d08301c07082fe6a82fc4b46dd39a"
  },
  "rpc-header-selection-arm-root-20261018-02/outputs/test-rpc-state-read-cost-self-test.exit.json": {
    "bytes": 89,
    "sha256": "4c26540a7521d74ca0982f504bac86a5ead2a06fefcabf8967efa83226e8ca5b"
  },
  "rpc-header-selection-arm-root-20261018-02/outputs/test-rpc-state-read-cost-self-test.stderr.log": {
    "bytes": 0,
    "sha256": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
  },
  "rpc-header-selection-arm-root-20261018-02/outputs/test-rpc-state-read-cost-self-test.stdout.log": {
    "bytes": 407,
    "sha256": "8283b975a08a72fc75f6e61f023c442afe8c6c0afa415639b214b551719c5036"
  },
  "rpc-header-selection-arm-root-20261018-02/result-public.json": {
    "bytes": 246079,
    "sha256": "8329fe174b1f51ed4e8246b911e7087d31656cdefd03183d89fd8b39883f40f5"
  },
  "rpc-header-selection-arm-scoped-reuse-audit-root-20261018-01/core-member-proof-public.json": {
    "bytes": 8693,
    "sha256": "b96735ba1f7648f068b193144045112edd0eb044668bf425a6bbf2f8d785bd5d"
  },
  "rpc-header-selection-arm-scoped-reuse-audit-root-20261018-01/metadata-exclusions-public.json": {
    "bytes": 11191,
    "sha256": "5f7f0e1f6cd9e832912f935a689fad84253003817f263da69cc696eebc8a04b6"
  },
  "rpc-header-selection-arm-scoped-reuse-audit-root-20261018-01/result-public.json": {
    "bytes": 1390,
    "sha256": "4094966bb27ad9e043955670377d07b33afdac8a4f8ea58fdddfcd856961d798"
  },
  "rpc-header-selection-delivery-proposal-20261018/FOURDOCAPPENDS.draft.json": {
    "bytes": 9280,
    "sha256": "3505b156eb958b19a24b6a9da727f4c2197201cca6d3ec6762eeedb26932110e"
  },
  "rpc-header-selection-delivery-proposal-20261018/README.md": {
    "bytes": 7556,
    "sha256": "4b526e92fb206100f66a3b7b5a05137d1ed35ca6a1db8e48477ce5f8fe79096d"
  },
  "rpc-header-selection-delivery-proposal-20261018/appends/EIP-IMPLEMENTATION-STATUS.append.md": {
    "bytes": 1746,
    "sha256": "a65aacc5101eb38b22bc83dcaee92bb07afb549ea4e174e9f3d1d43141126523"
  },
  "rpc-header-selection-delivery-proposal-20261018/appends/EVM-PRODUCTION-EIP-ROADMAP.append.md": {
    "bytes": 1775,
    "sha256": "2f6b2e055efaed005266be62c6e9d4554bdac0350c15f2570aa84c92c0385aea"
  },
  "rpc-header-selection-delivery-proposal-20261018/appends/PRODUCTION-GOAL.append.md": {
    "bytes": 2003,
    "sha256": "713f436665509481910b3b5d04e174602b309231e24456035e363181286b73af"
  },
  "rpc-header-selection-delivery-proposal-20261018/appends/VALIDATOR-EIP-ROADMAP.append.md": {
    "bytes": 1590,
    "sha256": "40e87fbc1a4a89298d36fd16b80e7b39e5c1391a832ad034bd8c4e8da66a50c2"
  },
  "rpc-header-selection-delivery-tools-source-review-root-20261018-01.json": {
    "bytes": 1388,
    "sha256": "def14e962bb5ed3fb5470dbbc36059a5675635f6962398644073c48fabf7d632"
  },
  "rpc-header-selection-evidence-gates-20261018.md": {
    "bytes": 5602,
    "sha256": "d2e3ca70766034e908c864e5c146bc751e55e775af1ff181e14df9bea9443216"
  },
  "rpc-header-selection-host-peer-review-20261018.md": {
    "bytes": 2642,
    "sha256": "97f3d23b9225de38ccd7d90abcfa004d6e2a8a099c9cdf51efe258fc5adf4b18"
  },
  "rpc-header-selection-review-20261018.md": {
    "bytes": 10456,
    "sha256": "ded20f81dfa6bd6cba0fb0cdd69609abd508c7c8e364af4e417e8177b96cdda4"
  },
  "rpc-header-selection-source-proposal-20261018/README.md": {
    "bytes": 10184,
    "sha256": "6b8baba1386a86991bc11702cc4ac538e444703ee6824b7bbe18da93f65a7e99"
  },
  "rpc-header-selection-source-proposal-20261018/manifest.json": {
    "bytes": 8892,
    "sha256": "99815701063127fa792d80d90ca11aa7779368146645a27b1694a8466e78cec4"
  },
  "rpc-header-selection-source-proposal-20261018/prospective-oracles.json": {
    "bytes": 5593,
    "sha256": "213e98a0e060a8fb16cb1b6a927e77e708b41460833af87f422f9b1c39a496d7"
  },
  "rpc-header-selection-source-proposal-20261018/source.patch": {
    "bytes": 76570,
    "sha256": "338d42b71cb5f108a191adea920991cae7bf61f60cf526d6e4ce6aee89555626"
  },
  "rpc-header-selection-source-proposal-20261018/source/CMakeLists.txt": {
    "bytes": 178472,
    "sha256": "94322e0abdc5737e81ff84b73e249728cccfecca1f1307d4161f5869f76d808a"
  },
  "rpc-header-selection-source-proposal-20261018/source/include/nano/chain.hpp": {
    "bytes": 8098,
    "sha256": "f0bdadfbc8aa1b0ca40086df0f03fa498fef3b05e12541d9a82213721bc2b9eb"
  },
  "rpc-header-selection-source-proposal-20261018/source/include/nano/storage.hpp": {
    "bytes": 11521,
    "sha256": "8a40f0abc7325a720f7345bec9391d4c49f893cb13e3e0f37d2cba7a8f678275"
  },
  "rpc-header-selection-source-proposal-20261018/source/src/chain.cpp": {
    "bytes": 44580,
    "sha256": "bcc95dd8d652d66c0f3523366079d373ebc80b2395ebdd98d059a1132ce11892"
  },
  "rpc-header-selection-source-proposal-20261018/source/src/chain_beacon.cpp": {
    "bytes": 7327,
    "sha256": "29a2dde2f926faf37eac9d78a09f6deda0ca06a4ade4a78247be7d4a01dd76ca"
  },
  "rpc-header-selection-source-proposal-20261018/source/src/rpc.cpp": {
    "bytes": 94036,
    "sha256": "169f7b9d90f99e476abee8be04e8fb55add0143d70f2b5ba69f5d0835200ba12"
  },
  "rpc-header-selection-source-proposal-20261018/source/src/storage.cpp": {
    "bytes": 75021,
    "sha256": "4d6f1be7b17c9ecd616297d85d6c6bbb2f858c6323b0444e9ca0b440ba02091f"
  },
  "rpc-header-selection-source-proposal-20261018/source/tests/test_rpc_get_proof.cpp": {
    "bytes": 29357,
    "sha256": "cda74ce13042f743eb05ac0a6b85cb61efda32e5b6aafe626e58d4051ab55890"
  },
  "rpc-header-selection-source-proposal-20261018/source/tests/test_rpc_state_block_id.cpp": {
    "bytes": 28276,
    "sha256": "349cba0c38be82c63220b699121df5f5139b5b8ddffdaea9811ac5634d554a08"
  },
  "rpc-header-selection-source-proposal-20261018/source/tests/test_rpc_state_read_cost.cpp": {
    "bytes": 24870,
    "sha256": "2d1379d04e8c867397e73f82f753ee2b5799e413c6fa5ce47b6e84b664c611a2"
  }
}


def need(ok, why):
    if not ok:
        raise ValueError(why)


def pin(b):
    return {'sha256':hashlib.sha256(b).hexdigest(),'bytes':len(b)}


def valid_pin(p):
    return isinstance(p,dict) and set(p)=={'sha256','bytes'} and isinstance(p['sha256'],str) and re.fullmatch('[0-9a-f]{64}',p['sha256']) is not None and type(p['bytes']) is int and 0 <= p['bytes'] <= 512*1024*1024


def pairs(xs):
    d = {}
    for k,v in xs:
        need(k not in d,'duplicate JSON key')
        d[k]=v
    return d


def obj(b):
    return json.loads(b.decode('utf-8'),object_pairs_hook=pairs,parse_constant=lambda _: (_ for _ in ()).throw(ValueError('nonfinite JSON')))


def typed_equal(a,b):
    if type(a) is not type(b):return False
    if isinstance(a,dict):return set(a)==set(b) and all(typed_equal(a[k],b[k]) for k in a)
    if isinstance(a,list):return len(a)==len(b) and all(typed_equal(x,y) for x,y in zip(a,b))
    return a==b


def encoded(x):
    return (json.dumps(x,ensure_ascii=False,indent=2,sort_keys=True)+'\n').encode()


def nine(s):
    return tuple(getattr(s,k) for k in ('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns'))


HELD = {}
INPUT_BYTES = 0

def read(p, want=None, cap=2*1024*1024):
    global INPUT_BYTES
    p=Path(p)
    need(p.is_absolute() and (p.is_relative_to(E) or p.is_relative_to(R)) and '.private' not in str(p) and p.resolve(strict=True)==p,'physical named public input')
    before=p.lstat()
    need(stat.S_ISREG(before.st_mode) and before.st_nlink==1 and before.st_size<=cap,'bounded single-link regular public input')
    fd=os.open(p,os.O_RDONLY|os.O_NOFOLLOW)
    try:
        need(nine(os.fstat(fd))==nine(before),'input changed before open')
        with os.fdopen(fd,'rb',closefd=False) as f:
            b=f.read(cap+1)
        need(len(b)<=cap and nine(os.fstat(fd))==nine(before) and nine(p.lstat())==nine(before),'input changed during read')
    finally:
        os.close(fd)
    if want is not None:
        need(valid_pin(want) and pin(b)==want,'public input does not equal externally known pin: '+str(p))
    row=(nine(before),pin(b))
    if p in HELD:
        need(HELD[p]==row,'held input changed')
    else:
        HELD[p]=row;INPUT_BYTES+=len(b)
        need(INPUT_BYTES<=32*1024*1024,'aggregate input cap')
    return b


def fences():
    for p,(identity,want) in list(HELD.items()):
        need(nine(p.lstat())==identity,'final full-nine input fence: '+str(p))
        need(pin(read(p,want,cap=max(2*1024*1024,want['bytes'])))==want,'final input bytes')


def known(rel):
    return read(E/rel,KNOWN[rel])


def test_files():
    out=['result-public.json','host-final-fence-public.json','outputs/result-public.json','outputs/core-recompile-selection-public.json','outputs/core-member-proof.json']
    for u in UNITS:
        out += ['outputs/test-'+u+'-self-test.'+x for x in ('stdout.log','stderr.log','exit.json')]
        out += ['outputs/case-'+u+'-self-test-public.json']
        out += ['outputs/'+prefix+'-'+u+'.exit.json' for prefix in ('M','CXX','LINK','LDD')]
        out += ['outputs/LDD-'+u+'.'+x for x in ('stdout.log','stderr.log')]
    return out


def code0(x, why):
    need(type(x) is int and x==0,why)


def check_pinlike(p, why):
    need(valid_pin(p) and p['bytes']>0,why)


def validate_core(w, files):
    need(w.get('format')=='nano-linux-rpc-state-proof-runtime-units-qualification-v1' and w.get('passed') is True and w.get('allConfiguredCasesPassed') is True and w.get('coreMode')=='incremental','worker actual qualification')
    expected={'archiveUpdates':1,'compiledTranslationUnits':47,'coreCompiledTranslationUnits':44,'coreIndependentM':143,'independentM':146,'links':3,'loaderChecks':3,'reusedSubmitObjects':0,'runtimeCliInvocations':0,'runtimeCompiledTranslationUnits':0,'runtimeIndependentM':0,'testCompiledTranslationUnits':3,'testIndependentM':3,'testInvocations':3,'unitInvocations':3}
    need(w.get('counts')==expected and all(type(x) is int for x in w['counts'].values()),'exact prospective 143M+3M/44Core+3TU counts')
    p=obj(files['outputs/core-member-proof.json']);s=obj(files['outputs/core-recompile-selection-public.json'])
    need(w.get('coreProof')==p and w.get('selection')==s,'worker joins separate proof and selection')
    need(all(type(p.get(k)) is int for k in ('members','selectedReplacementCount','unchangedOutsideReplacementSet','actualChangedCount')),'integer Core cohort counts')
    need(p.get('format')=='nano-object-set-archive-member-proof-v1' and p.get('passed') is True and p.get('memberOrderPreserved') is True and p.get('members')==143 and p.get('selectedReplacementCount')==44 and p.get('unchangedOutsideReplacementSet')==99 and 0<=p.get('actualChangedCount')<=44,'143 members/44 replaced/99 exact retained')
    changed=p.get('actualChangedMembers');replacements=p.get('replacementMembers')
    need(isinstance(changed,list) and isinstance(replacements,list) and len(replacements)==44 and len(set(replacements))==44 and len(changed)==p['actualChangedCount'] and len(set(changed))==len(changed) and set(changed)<=set(replacements) and isinstance(p.get('newObjects'),dict) and set(p['newObjects'])==set(replacements),'descriptive actual change list within exact replacement cohort')
    check_pinlike(p['newArchive'],'new archive pin');check_pinlike(p['oldArchive'],'old archive pin')
    need(type(s.get('coreSourceCount')) is int and type(s.get('independentCoreScans')) is int and s.get('coreSourceCount')==143 and s.get('independentCoreScans')==143 and set(s.get('changedProjectPaths',[]))==CODE and len(s['changedProjectPaths'])==10 and set(s.get('changedHeaderPaths',[]))=={'include/nano/chain.hpp','include/nano/storage.hpp'} and set(s.get('modifiedCoreSourcePaths',[]))=={'src/chain.cpp','src/chain_beacon.cpp','src/rpc.cpp','src/storage.cpp'},'actual exact Core selection scope')
    rows=s.get('rows');need(isinstance(rows,list) and len(rows)==143 and len({r['member'] for r in rows})==143 and all(type(r['compileSelected']) is bool for r in rows),'143 distinct selected/retained rows')
    selected=[r for r in rows if r['compileSelected']]
    need(len(selected)==44 and {r['member'] for r in selected}==set(p['replacementMembers']) and {r['source'] for r in selected}==set(s['selectedSources']) and len(s['selectedSources'])==44,'selected source/member join')
    need(isinstance(w.get('mmd'),dict) and len(w['mmd'])==47 and all(v.get('normalizedMEqualsMd') is True and type(v.get('literalMEqualsMd')) is bool for v in w['mmd'].values()),'47 M/MD closures')
    need(isinstance(w.get('steps'),list) and w['steps'] and all(type(t.get('exitCode')) is int and t['exitCode']==0 for t in w['steps']),'all worker terminal steps')
    phases=[t['phase'] for t in w['steps']]
    for prefix,count in (('M-',146),('M-core-',143),('CXX-',47),('CXX-core-',44),('LINK-',3),('LDD-',3),('test-',3)):
        need(sum(p.startswith(prefix) for p in phases)==count,'actual worker phase count '+prefix)
    need(phases.count('replace-selected-Core-objects')==1,'one actual archive replacement')
    for k in ('productionQualified','rootIndependentProofVectorsVerified','runtimeDutiesResumed','runtimeServeOrRpcQualified','externalNetworkRequired'):
        need(w.get(k) is False,'worker scope remains false: '+k)
    need(type(w.get('coreFullBuilds')) is int and type(w.get('cmakeConfigures')) is int and w.get('coreFullBuilds')==0 and w.get('cmakeConfigures')==0,'incremental only, no configure/full build')
    return p


def records(b):
    need(len(b)<=128*1024 and b.endswith(b'\n'),'bounded terminated JSONL')
    return [obj(line) for line in b.splitlines()]


def validate_tests(w, files):
    results=w.get('testResults');need(isinstance(results,list) and len(results)==3,'three case reports')
    byname={r['name']:r for r in results};need(set(byname)=={u+'-self-test' for u in UNITS},'exact three case names')
    streams={}
    for u in UNITS:
        n=u+'-self-test';r=obj(files['outputs/case-'+n+'-public.json'])
        need(byname[n]==r and r.get('name')==n and r.get('unit')==u and r.get('scope')=='candidate' and r.get('semanticStreamOraclesPassed') is True and r.get('baselineExpectedFailure') is False and r.get('runtimeArityOnly') is False,'actual case scope and report join')
        code0(r.get('actualExitCode'),'case actual exit')
        need(r.get('stderrOracle')=={'policy':'empty','semanticOraclePassed':True} and files['outputs/test-'+n+'.stderr.log']==b'','empty actual test stderr')
        for stem in ('M-'+u,'CXX-'+u,'LINK-'+u,'LDD-'+u,'test-'+n):
            ex=obj(files['outputs/'+stem+'.exit.json']);code0(ex.get('exitCode'),'actual '+stem+' exit')
            need(sum(t==ex for t in w['steps'])==1,'separate exit equals unique actual worker step')
            need(ex.get('phase')==stem and type(ex.get('elapsedMs')) is int and 0<=ex['elapsedMs'] and (not stem.startswith('test-') or ex['elapsedMs']<=180000),'actual terminal phase; only test timeout is a semantic bound')
        need(b'not found' not in files['outputs/LDD-'+u+'.stdout.log'].lower(),'actual loader resolves dependencies')
        streams[u]=records(files['outputs/test-'+n+'.stdout.log'])
        policy='get-proof-record-stream-v1' if u=='rpc-get-proof' else 'json-field-records'
        need(r.get('stdoutOracle',{}).get('policy')==policy,'prospective stream policy')
        if u!='rpc-get-proof':need(r['stdoutOracle'].get('semanticOraclePassed') is True,'case stream oracle')
    c=streams['rpc-state-read-cost'];need(len(c)==1,'one cost summary')
    expect={'scenario':'state-read-cost-summary','passed':True,'groups':1,'checks':47,'expectedRefusals':23,'accounts':128,'slotsPerAccount':16,'headerSelectionCases':True,'beaconReadyCases':True,'fullBeaconAbaQualified':False,'newTestOnlyBlsSignatures':12,'maximumTestBlsSignatures':32,'maximumRssKiB':131072,'runtimeDutiesAuthorized':False,'privateInputsRead':False,'externalNetwork':False}
    need(set(c[0])==set(expect)|{'actualPeakRssKiB'} and all(c[0].get(k)==v and type(c[0][k]) is type(v) for k,v in expect.items()) and type(c[0]['actualPeakRssKiB']) is int and 0<c[0]['actualPeakRssKiB']<=131072,'prospective cost47/23/RSS/BLS oracle')
    rr=streams['rpc-state-block-id'];need(len(rr)==6,'five reader groups and summary')
    seq=[('state-readers-canonical-selectors-paris-shanghai',255,14),('state-readers-selector-error-precedence',377,126),('state-readers-detached-capture-and-real-reorg',674,150),('state-readers-real-retention-and-reopen',750,170),('state-readers-http-object-selectors-and-batch',805,179)]
    for r,(name,checks,refusals) in zip(rr,seq):
        need(r=={'scenario':name,'passed':True,'checks':checks,'expectedRefusals':refusals} and r['passed'] is True and type(r['checks']) is int and type(r['expectedRefusals']) is int,'prospective ordered reader group')
    need(typed_equal(rr[-1],{'scenario':'state-readers-summary','passed':True,'groups':5,'checks':805,'expectedRefusals':179,'selectedGroup':'all','methods':4,'externalNetwork':False,'consensusModified':False}),'prospective reader805/179 summary')
    pr=streams['rpc-get-proof'];need(len(pr)==30,'proof stream30 records')
    vectors=[r for r in pr if r.get('scenario')=='proof-vector'];reports=[r for r in pr if r.get('scenario')!='proof-vector']
    need(len(vectors)==24 and len(reports)==6 and hashlib.sha256(json.dumps(vectors,sort_keys=True,separators=(',',':')).encode()).hexdigest()==CANONICAL,'24 actual vectors equal previously independently verified canonical content')
    names=('get-proof-native-mpt-shapes-and-canonical-rlp','get-proof-account-storage-and-object-selectors','get-proof-errors-configurable-budgets-and-cancellation','get-proof-pinned-reorg-real-retention-and-reopen','get-proof-http-websocket-capacity-and-shutdown')
    last=0;lastref=0
    for r,name in zip(reports,names):
        need(set(r)=={'scenario','passed','checks','expectedRefusals'} and r['scenario']==name and r['passed'] is True and type(r['checks']) is int and last<r['checks']<=10000000 and type(r['expectedRefusals']) is int and lastref<=r['expectedRefusals']<=63,'prospective ordered proof groups')
        last=r['checks'];lastref=r['expectedRefusals']
    ps=reports[-1];need(set(ps)=={'scenario','passed','groups','checks','expectedRefusals','proofVectors','selectedGroup','externalNetwork','consensusModified','productionQualified'} and ps['scenario']=='get-proof-summary' and ps['passed'] is True and type(ps['groups']) is int and ps['groups']==5 and type(ps['expectedRefusals']) is int and type(ps['proofVectors']) is int and type(ps['checks']) is int and last<ps['checks']<=10000000 and ps['expectedRefusals']==63 and ps['proofVectors']==24 and ps['selectedGroup']=='all' and all(ps[k] is False for k in ('externalNetwork','consensusModified','productionQualified')),'proof5/63/24 prospective summary')
    po=byname['rpc-get-proof-self-test']['stdoutOracle'];need(po.get('mechanicalCppPassed') is True and po.get('proofVectors')==24 and po.get('records')==30 and po.get('scenarioReports')==5 and po.get('rootIndependentProofVectorsVerified') is False and po.get('summary')==ps,'proof actual policy joins stream')
    return c[0]['actualPeakRssKiB']


def allowed_names():
    out=set(test_files())|{'readback-index-public.json'}
    for u in UNITS:
        for stem in ('M-'+u,'CXX-'+u,'LINK-'+u,'LDD-'+u,'test-'+u+'-self-test'):
            out|={'outputs/'+stem+'.'+x for x in ('stdout.log','stderr.log','exit.json')}
    for stem in ('git-head-before','git-clean-before','image-before','name-before','create-own','created-own','start-own','running-own','git-head-after','git-clean-after','cleanup-by-cid','stop-own','cleanup-stopped','remove-own','cleanup-absence','image-final'):
        out.add(stem+'.exit-public.json')
    return out


def amd_files(result_pin, terminal_pin):
    result=obj(read(AMD/'result-public.json',result_pin));terminal=obj(read(TERMINAL,terminal_pin))
    need(terminal.get('format')=='nano-root-amd-rpc-header-host-terminal-v1' and terminal.get('remoteOutput')=='/home/ubuntu/rpc-header-selection-amd-root-20261018-02','actual dispatcher terminal')
    code0(terminal.get('exitCode'),'actual dispatcher terminal exit0')
    need(type(terminal.get('elapsedMs')) is int and terminal['elapsedMs']>=0,'dispatcher elapsed')
    need(result.get('format')=='nano-rpc-header-selection-amd-public-readback-v1' and result.get('passed') is True and result.get('qualificationPassed') is True,'collector copy and qualification distinct actual PASS')
    code0(result.get('sshExitCode'),'collector SSH terminal')
    need(result.get('privateAuthorityRowsReadOrExported') is False and result.get('databaseOrKeyFilesRead') is False and type(result.get('newTestsRun')) is int and result['newTestsRun']==0 and result.get('productionQualified') is False,'public-only passive readback scope')
    ssh=obj(read(AMD/'ssh.exit-public.json'));code0(ssh.get('exitCode'),'separate collector SSH exit')
    need(read(AMD/'ssh.stderr.log')==b'','empty collector SSH stderr')
    archive=read(AMD/'public-evidence.tar',result['archivePin'],cap=10*1024*1024)
    files={};aggregate=0
    with tarfile.open(fileobj=io.BytesIO(archive),mode='r:') as tf:
        for m in tf.getmembers():
            need(m.name in allowed_names() and m.name not in files and m.isfile() and m.mode==0o400 and 0<=m.size<=2*1024*1024,'strict public tar member')
            aggregate+=m.size;need(len(files)<128 and aggregate<=8*1024*1024,'public tar aggregate cap')
            f=tf.extractfile(m);need(f is not None,'regular tar content');b=f.read(m.size+1)
            need(len(b)==m.size,'complete tar content');files[m.name]=b
            need(read(AMD/'evidence'/m.name,pin(b))==b,'actual extracted readback equals pinned archive')
    need(set(test_files())|{'readback-index-public.json'} <= set(files),'complete actual public test artifacts')
    index=obj(files['readback-index-public.json'])
    need(index.get('format')=='nano-rpc-header-selection-public-readback-v1' and index.get('hostPassed') is True and index.get('fencePassed') is True and index.get('prospectivePublicOutputsOnly') is True and index.get('privateAuthorityRowsReadOrExported') is False and index.get('databaseOrKeyFilesRead') is False,'public index actual host/fence scope')
    need(index.get('files')=={n:pin(b) for n,b in files.items() if n!='readback-index-public.json'} and type(result.get('files')) is int and result['files']==len(files) and type(result.get('evidenceBytes')) is int and result['evidenceBytes']==aggregate,'archive/index/result byte joins')
    host=obj(files['result-public.json']);fence=obj(files['host-final-fence-public.json']);w=obj(files['outputs/result-public.json'])
    need(host.get('format')=='nano-rpc-header-selection-incremental-host-result-v1' and host.get('passed') is True and host.get('actualHead')==BASE and host.get('worker')==w and host.get('firstFailure') is None and host.get('secondaryFailures')==[],'actual host PASS no hidden first/secondary failure')
    need(fence.get('passed') is True and fence.get('changes')==[] and fence.get('oldRowsRefreshed') is False and fence.get('afterCleanupAndImageFinal') is True and host.get('hostFinalFencePublished') is True and host.get('hostFinalFenceAfterCleanup') is True,'final fence after cleanup/image, no old-row refresh')
    need(host.get('imageFinalFence',{}).get('passed') is True and host.get('cleanup',{}).get('passed') is True and host['cleanup'].get('ownedContainerAbsent') is True and host.get('imageFinalFence')=={'architecture':'amd64','image':IMAGE,'passed':True},'actual owned cleanup and exact image final fence')
    for k in ('networkTransport','newCampaignStarted','runtimeDutiesResumed','privateCampaignInputsRead','sdkGlobalQualified','productionQualified'):
        need(host.get(k) is False,'host scope false: '+k)
    for k in ('newImagesBuilt','cmakeConfigures','oldCppTestsReplayed'):
        need(type(host.get(k)) is int and host[k]==0,'host zero '+k)
    need(host.get('affectedCoreCompilations')==44 and host.get('affectedCoreIndependentM')==143 and host.get('archiveUpdates')==1 and host.get('newCppCompilations')==47 and host.get('retainedCoreMembers')==143,'host actual compile cohort')
    p=validate_core(w,files);rss=validate_tests(w,files)
    return files,result,terminal,p,rss


def write_new(p,b):
    p.parent.mkdir(parents=True,exist_ok=True)
    with p.open('xb') as f:
        f.write(b);f.flush();os.fsync(f.fileno())
    p.chmod(0o400)
    need(p.stat().st_nlink==1 and p.stat().st_mode&0o777==0o400 and p.read_bytes()==b,'frozen output bytes')


def main():
    need(len(sys.argv)==8,'self pin, AMD result pin, AMD terminal pin, fresh namespace')
    pp=[]
    for i in (1,3,5):
        need(re.fullmatch('[0-9a-f]{64}',sys.argv[i]) is not None and re.fullmatch('[0-9]+',sys.argv[i+1]) is not None,'external SHA/size arguments')
        p={'sha256':sys.argv[i],'bytes':int(sys.argv[i+1])};need(valid_pin(p),'external pin');pp.append(p)
    self_bytes=read(Path(__file__).absolute(),pp[0])
    need(re.fullmatch(r'rpc-header-selection-public-package-root-20261018-[A-Za-z0-9-]{1,40}',sys.argv[7]) is not None,'fresh bounded output component')
    output=E/sys.argv[7];need(not output.exists(),'never overwrite a previous package')
    inputs={rel:known(rel) for rel in KNOWN}
    source_mf=obj(inputs[str((S/'manifest.json').relative_to(E))])
    need(source_mf.get('status')=='FROZEN_SOURCE_ONLY' and source_mf.get('baseCommit')==BASE and set(source_mf['sourceFiles'])==CODE and len(source_mf['payloads'])==13,'frozen C7 parent')
    sources={}
    for row in source_mf['payloads']:
        p=S/row['path'];b=inputs[str(p.relative_to(E))]
        need(pin(b)=={'sha256':row['sha256'],'bytes':row['bytes']} and p.stat().st_mode&0o777==0o400,'all13 exact frozen source payloads')
        sources[row['path']]=b
    baselines={r['path']:{'sha256':r['sha256'],'bytes':r['bytes']} for r in source_mf['baselinePins']}
    need(set(baselines)==CODE,'ten parent byte pins')
    for rel,want in baselines.items():read(R/rel,want)
    attrs=read(R/'.gitattributes')
    drafts=obj(inputs[str((D/'FOURDOCAPPENDS.draft.json').relative_to(E))])
    need(drafts.get('sourceBase')==BASE and drafts.get('requiredMarker')==MARKER and drafts.get('preserveEntireCurrentBodyAsByteExactPrefix') is True and drafts.get('appendOnlyIfMarkerAbsent') is True and len(drafts['documents'])==4,'draft append contract')
    prefixes={};suffixes={}
    for row in drafts['documents']:
        rel=row['path'];need(rel in {'docs/'+n+'.md' for n in DOCS} and rel not in prefixes,'four distinct normative documents')
        prefixes[rel]=read(R/rel,row['before'])
        b=inputs[str((D/row['appendSource']).relative_to(E))]
        need(b.decode()==row['appendText'] and pin(prefixes[rel]+b)==row['after'] and b.startswith(b'\n') and MARKER.encode() not in prefixes[rel] and b.count(PLACEHOLDER.encode())==1,'exact original-prefix draft suffix')
        suffixes[rel]=b.decode()
    arm={n:inputs[str((ARM/n).relative_to(E))] for n in test_files()}
    ah=obj(arm['result-public.json']);af=obj(arm['host-final-fence-public.json']);aw=obj(arm['outputs/result-public.json'])
    need(ah.get('passed') is False and ah.get('worker')==aw and af.get('passed') is False and af.get('oldRowsRefreshed') is False,'ARM02 whole FAILED retained')
    ap=validate_core(aw,arm);validate_tests(aw,arm);need(ap['newArchive']==ARMCORE and ap['actualChangedCount']==4 and set(ap['actualChangedMembers'])=={'chain.cpp.o','chain_beacon.cpp.o','rpc.cpp.o','storage.cpp.o'},'known scoped ARM archive and actual four changed object bodies')
    audit=obj(inputs[str((AUDIT/'result-public.json').relative_to(E))])
    need(audit.get('passed') is True and audit.get('candidateCorePin')==ARMCORE and audit.get('originalWholeInvocationPassed') is False and audit.get('wholeSnapshotFailurePreserved') is True and audit.get('snapshotMetadataUnchanged') is False and type(audit.get('onlyUnusedSourceCtimeChanges')) is int and audit['onlyUnusedSourceCtimeChanges']==26 and audit.get('changedPathsIntersectConsumedMaterials') is False and audit.get('allConsumedHostOriginalRowsStillExact') is True and audit.get('allSourceBytePinsStillExact') is True and audit.get('actual24VectorsEqualPreviouslyIndependentlyVerified') is True and audit.get('canonical24VectorSha256')==CANONICAL and audit.get('originalProductRowsStillExact')==244 and audit.get('selectedGuestInputFiles')==2578 and audit.get('sourceInventoryFiles')==2094 and audit.get('archiveMembers')==143 and audit.get('replacedMembers')==44 and audit.get('unchangedMembers')==99,'exact scoped ARM audit without whole promotion')
    for k in ('newCompilations','newLinks','newTests'):need(type(audit.get(k)) is int and audit[k]==0,'ARM audit no new work')
    need(audit.get('independentVerifierReexecuted') is False and audit.get('productionQualified') is False and audit.get('fullSdkOrReleaseQualified') is False,'ARM scoped exclusions')
    amd,collector,terminal,cp,rss=amd_files(pp[1],pp[2])
    # Terminal AMD facts are rendered only here. The drafts themselves remain untouched.
    amd_sentence=('**AMD02 : PASS terminal, dispatcher exit 0 et collecte publique qualifiée.** '
                  'Le worker a effectué 143 scans M Core et trois scans de tests, 44 compilations Core et trois unités, '
                  'une mise à jour d’archive, trois liens, trois vérifications du chargeur et trois tests. '
                  'Les oracles restent 47/23, 805/179 et cinq groupes/63 refus/24 vecteurs. '
                  'Les 143 membres comprennent 44 remplacés et 99 inchangés ; clôture après nettoyage et image AMD exacte conservées. '
                  'Les 24 vecteurs égalent le contenu canonique précédemment vérifié, sans nouveau vérificateur indépendant.')
    amd_section=('## AMD02 — reçu terminal et collecte publique\n\n'+amd_sentence+'\n\n'
                 'Core AMD SHA `'+cp['newArchive']['sha256']+'`, '+str(cp['newArchive']['bytes'])+' octets. '
                 +str(cp['actualChangedCount'])+' corps d’objets diffèrent dans les 44 remplacements, constat descriptif sans conclusion sémantique supplémentaire. '
                 'Pic RSS du test coût : '+str(rss)+' KiB, sous 131 072 KiB. '
                 'La sortie dispatcher vaut 0, comme la collecte SSH ; le conteneur détenu est absent après nettoyage. '
                 'L’image reste `'+IMAGE+'`. Le reçu brut [hôte AMD](amd/result-public.json), '
                 'le [worker](amd/outputs/result-public.json), le [fence final](amd/host-final-fence-public.json) '
                 'et le [reçu de collecte](amd/readback-result-public.json) gardent leurs rôles distincts. '
                 'Aucun build d’image, ancien test rejoué, duty, réseau de campagne ou lancement public. '
                 'Le refus préparatoire AMD01 concernant l’ancien provider KZG généré est conservé (zéro CXX, test et Docker). '
                 'Le premier lecteur Root a ensuite refusé une comparaison de stat incluant atime, sans exporter d’octets ; '
                 'les champs initiaux n’ont pas été publiés et la cause metadata précise n’est pas caractérisée. Son échec et la lecture ultérieure fondée sur les neuf champs prévus restent séparés, sans rafraîchissement d’autorité.\n\n')
    text=inputs[str((D/'README.md').relative_to(E))].decode()
    text=text.replace('# Sélection RPC des headers — livraison en préparation','# Sélection RPC des headers — qualification C7 bornée')
    text=text.replace('**DRAFT NON GELÉ — AMD02 terminale en attente. Ce fichier ne doit pas être importé comme qualification finale.**','Paquet public gelé proposé à Root pour import ; aucune publication Git ni mise à jour du binaire réseau n’est effectuée par son assemblage.')
    start=text.index('## AMD — résultats à renseigner seulement sur reçus terminaux');end=text.index('## Autres limites',start)
    text=text[:start]+amd_section+text[end:]
    text=text.replace('../rpc-header-selection-arm-scoped-reuse-audit-root-20261018-01/result-public.json','arm/scoped-reuse/result-public.json')
    text=text[:text.index('## Critères d’intégration Root')]+('## Import et limites de la livraison\n\n'
        'Le plan épinglé importe dix fichiers source et les preuves publiques, puis ajoute quatre suffixes aux corps normatifs conservés intégralement. '
        'Root doit encore contrôler le parent Git exact, le diff et les blobs avant commit/push/pull VPS. '
        'Aucun commit futur ni qualification Native C7 n’est présumé. L’assemblage et la collecte ne relancent ni C++ ni Node. '
        'Le manifeste source historique reste FROZEN_SOURCE_ONLY ; les reçus de qualification sont archivés séparément.\n')
    need('DRAFT_' not in text and 'en attente au présent draft' not in text,'no terminal placeholders in final README')
    final_suffix={rel:s.replace(PLACEHOLDER,amd_sentence).replace('Le candidat C7 de sélection des headers est un overlay de dix sources fondé sur main `801d793f…` / C6 ; il n’est pas encore importé par ce draft.','Ce lot fournit les dix blobs C7 de sélection des headers, fondés sur le parent main `801d793f…` / C6.').replace('Implémentation **candidate C7**, non importée par ce draft :','Ce lot fournit dix blobs **C7**, fondés sur le parent main801d/C6 :').replace('qualification partielle ARM','réutilisation ARM distincte et qualification AMD').replace('preuve locale partielle ARM','preuve locale ARM/AMD bornée').replace('ARM partiel','ARM distinct et AMD qualifié').replace('qualification ARM bornée','qualification ARM/AMD bornée') for rel,s in suffixes.items()}
    need(all('DRAFT_' not in s and 'par ce draft' not in s and 'par cet assemblage' not in s for s in final_suffix.values()),'no final suffix placeholder or pre-import fiction')
    fences()  # All source, prefix and terminal data validated before any output is created.
    output.mkdir(mode=0o700)
    payloads={};mappings={};append={}
    def add(rel,b,public=True):
        need(not Path(rel).is_absolute() and '..' not in Path(rel).parts and '.private' not in rel and rel not in payloads,'unique public package path')
        p=output/'payloads'/rel;write_new(p,b);payloads[rel]=pin(b)
        if public:mappings[Q+rel]={'source':str(p),'pin':pin(b)}
        return p
    add('README.md',text.encode())
    add('source/manifest.json',inputs[str((S/'manifest.json').relative_to(E))])
    for rel,b in sources.items():
        p=add('source/'+rel,b)
        if rel.startswith('source/'):
            code=rel[len('source/'):];need(code in CODE,'exact ten source mappings');mappings[code]={'source':str(p),'pin':pin(b)}
    for n,b in arm.items():add('arm/whole02/'+n,b)
    for name in ('result-public.json','core-member-proof-public.json','metadata-exclusions-public.json'):
        add('arm/scoped-reuse/'+name,inputs[str((AUDIT/name).relative_to(E))])
    a1='rpc-header-selection-arm-root-20261018-01/result-public.json';add('arm/preparation01/result-public.json',inputs[a1])
    for rel in (
        'rpc-header-selection-amd-preparation-failure-public-readback-root-20261018-02/preparation-failure-public.json',
        'rpc-header-selection-amd-preparation-failure-public-readback-root-20261018-01/failure-public.json',
        'rpc-header-selection-amd-preparation-failure-public-readback-root-20261018-01/ssh.stderr.log'):
        attempt='preparation01' if rel.split('/')[0].endswith('-02') else 'preparation-readback-refusal01'
        add('amd/'+attempt+'/'+Path(rel).name,inputs[rel])
    for n in test_files():add('amd/'+n,amd[n])
    add('amd/readback-index-public.json',amd['readback-index-public.json'])
    add('amd/readback-result-public.json',read(AMD/'result-public.json',pp[1]))
    add('amd/dispatcher-terminal-public.json',read(TERMINAL,pp[2]))
    add('amd/readback-ssh-exit-public.json',read(AMD/'ssh.exit-public.json'))
    for rel,b in inputs.items():
        if rel.startswith('rpc-header-selection-') and rel.endswith(('.md','.json')) and '/' not in rel:
            add('reviews/'+Path(rel).name,b)
    add('controllers/assemble-public-package.py',self_bytes)
    for rel,s in final_suffix.items():
        p=output/'appends'/(Path(rel).stem+'.append.md');b=s.encode();write_new(p,b)
        append[rel]={'source':str(p),'pin':pin(b),'basePin':pin(prefixes[rel])}
    public_mf={'format':'nano-rpc-header-selection-public-payload-manifest-v1','sourceBase':BASE,'files':payloads,'armWholeInvocationPassed':False,'armScopedReusePassed':True,'amdQualificationPassed':True,'sourceChanges':10,'nativeC7Qualified':False,'fullBeaconAbaQualified':False,'productionQualified':False,'privateAuthorityRowsIncluded':False}
    add('payload-manifest.json',encoded(public_mf))
    plan={'format':'nano-rpc-header-selection-public-delivery-plan-v1','repository':str(R),'baseHead':BASE,'baseMinimumAncestor':BASE,'actualHead':None,'sourceChanges':10,'sourceBaselines':baselines,'files':mappings,'append':append,'attributesBasePin':pin(attrs),'preserveEntireCurrentBodyAsByteExactPrefix':True,'appendOnlyIfMarkerAbsent':True,'requiredMarker':MARKER,'productionQualified':False}
    need(len(mappings)<=512 and sum(v['pin']['bytes'] for v in mappings.values())<=8*1024*1024 and sorted(p for p in mappings if p.endswith('.patch'))==[Q+'source/source.patch'],'exact importer scope/count/size/patch')
    fences()
    write_new(output/'plan.json',encoded(plan))
    entries={str(p.relative_to(output)):{**pin(p.read_bytes()),'mode':'0400'} for p in output.rglob('*') if p.is_file()}
    manifest={'format':'nano-rpc-header-selection-delivery-package-v1','status':'FROZEN_PUBLIC_ROOT_IMPORT_PROPOSAL','files':entries,'planPin':pin((output/'plan.json').read_bytes()),'sourceManifestPin':KNOWN[str((S/'manifest.json').relative_to(E))],'amdReadbackResultPin':pp[1],'amdDispatcherTerminalPin':pp[2],'inputFullNineFencesPassed':True,'sourceBase':BASE,'repositoryWritten':False,'productionQualified':False}
    fences()
    write_new(output/'manifest.json',encoded(manifest))
    print(json.dumps({'output':str(output),'manifestPin':pin((output/'manifest.json').read_bytes()),'planPin':manifest['planPin'],'files':len(entries),'passed':True,'productionQualified':False}))


if __name__=='__main__':
    try:
        main()
    except BaseException as error:
        # No retry/cleanup or reclassification. An incomplete fresh namespace remains evidence.
        print(json.dumps({'format':'nano-rpc-header-selection-assembly-refusal-v1','passed':False,'errorType':type(error).__name__,'message':str(error)[:2048],'productionQualified':False}),file=sys.stderr)
        raise
