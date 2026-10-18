#!/usr/bin/env python3
"""Apply a reviewed byte-pinned source/public-evidence plan; no Git mutation."""
import hashlib, json, os, pathlib, stat, subprocess, sys
R = pathlib.Path('/Users/boris/Dev/blockchain-nano')
GIT = '/Library/Developer/CommandLineTools/usr/bin/git'
F = ('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')

def need(ok, why):
    if not ok: raise RuntimeError(why)

def read(p):
    need(p.is_absolute() and p.resolve(strict=True) == p, 'physical absolute input')
    fd = os.open(p, os.O_RDONLY | os.O_NOFOLLOW | os.O_CLOEXEC)
    try:
        s = os.fstat(fd); row = {k:getattr(s,k) for k in F}
        need(stat.S_ISREG(s.st_mode) and s.st_nlink == 1 and 0 <= s.st_size <= 8*1024*1024, 'bounded single-link regular input')
        raw=b''
        while part:=os.read(fd,65536): raw += part; need(len(raw) <= 8*1024*1024, 'bounded input')
        need(row == {k:getattr(os.fstat(fd),k) for k in F} == {k:getattr(p.lstat(),k) for k in F}, 'stable FD/path')
        return raw, dict(row,sha256=hashlib.sha256(raw).hexdigest())
    finally: os.close(fd)

def content(row): return {'bytes':row['st_size'],'sha256':row['sha256']}
def git(*args): return subprocess.run([GIT,'--no-optional-locks','-c','core.fsmonitor=false',*args],cwd=R,check=True,capture_output=True,text=True).stdout.strip()

def existing_ancestry(parent):
    need(parent.is_relative_to(R) and R.resolve(strict=True)==R,'physical repository ancestor')
    current=R
    for part in parent.relative_to(R).parts:
        current=current/part
        if not os.path.lexists(current): break
        need(stat.S_ISDIR(current.lstat().st_mode) and current.resolve(strict=True)==current,'no symlink or nondirectory existing ancestor')

def main():
    need(len(sys.argv)==4,'usage: publisher PLAN SHA256 BYTES')
    os.umask(0o077); planpath=pathlib.Path(sys.argv[1]); raw, planrow=read(planpath)
    need(content(planrow)=={'sha256':sys.argv[2],'bytes':int(sys.argv[3])},'frozen plan pin')
    d=json.loads(raw); need(d['format']=='nano-journal-batch-publication-plan-v1','exact plan format')
    need(git('branch','--show-current')=='main' and git('rev-parse','HEAD')==d['parent'] and not git('status','--porcelain') and not git('diff','--cached','--name-only'),'clean expected checkout/index')
    receipt=pathlib.Path(d['receipt']); need(receipt.parent==planpath.parent and not os.path.lexists(receipt),'fresh receipt beside plan')
    normative={'docs/'+n for n in ('PRODUCTION-GOAL.md','VALIDATOR-EIP-ROADMAP.md','EVM-PRODUCTION-EIP-ROADMAP.md','EIP-IMPLEMENTATION-STATUS.md')}
    need(type(d['items'])is list and 1<=len(d['items'])<=128,'bounded item count')
    need({i['destination']for i in d['items']if i['mode']=='append'}==normative and [i['destination']for i in d['items']if i['mode']=='replace']==['tests/qualification/beacon_journal_physical_rollback.cpp'],'exact four normative appends and only journal TU replacement')
    replacement=next(i for i in d['items']if i['mode']=='replace')
    need(replacement['before']=={'sha256':'b6760c6ddf98ec1cbd8968351e37155848d5eab752c4c11e2c0bf96b8bb901d6','bytes':24367}and replacement['after']==replacement['input']=={'sha256':'2e5ef451f13cca770cf02128bbb389eaae60379974252859829e197d14521acd','bytes':28801},'fixed V2-to-V3 journal source bytes')
    prepared_bytes=0
    prepared=[]; held={str(planpath):planrow}; destinations=set()
    for item in d['items']:
        relative=pathlib.PurePosixPath(item['destination']); need(not relative.is_absolute() and '..' not in relative.parts and '.git' not in relative.parts,'repository destination only')
        dst=R/relative; existing_ancestry(dst.parent); need(str(dst)not in destinations,'unique destination'); destinations.add(str(dst))
        source=pathlib.Path(item['source']); b,row=read(source); need(content(row)==item['input'],'exact public/source bytes'); held[str(source)]=row
        before=None
        if item['mode']=='new':
            need(item['destination'].startswith('docs/qualification/real-journal-rollback-v3-20261018/'),'new V3 public evidence/document scope')
            need(not os.path.lexists(dst),'new path only'); output=b
        else:
            old,before=read(dst); need(content(before)==item['before'],'original destination bytes')
            need(item['mode']in('replace','append'),'closed mutation mode'); output=old+b if item['mode']=='append' else b
            if item['mode']=='append': need(output[:len(old)]==old,'entire normative byte prefix retained')
        need({'sha256':hashlib.sha256(output).hexdigest(),'bytes':len(output)}==item['after'],'planned output bytes')
        prepared_bytes+=len(output);need(prepared_bytes<=16*1024*1024,'bounded total prepared payload')
        prepared.append((item,dst,output,before))
    need(all(read(pathlib.Path(p))[1]==row for p,row in held.items()),'all held inputs unchanged before writes')
    written=[]
    for item,dst,output,before in prepared:
        if before is not None: need(read(dst)[1]==before,'destination unchanged before mutation')
        else: need(not os.path.lexists(dst),'new destination remains absent')
        existing_ancestry(dst.parent)
        dst.parent.mkdir(parents=True,exist_ok=True,mode=0o755)
        need(dst.parent.resolve(strict=True)==dst.parent,'no destination parent symlink')
        tmp=dst.with_name(dst.name+'.journal-publication-new'); fd=os.open(tmp,os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,0o600)
        try:
            with os.fdopen(fd,'wb')as f: f.write(output); f.flush(); os.fchmod(f.fileno(),0o644); os.fsync(f.fileno())
            need((before is None and not os.path.lexists(dst))or(before is not None and read(dst)[1]==before),'destination fence before atomic replacement')
            os.replace(tmp,dst)
            need(content(read(dst)[1])==item['after'],'written exact payload')
            written.append({'destination':item['destination'],'mode':item['mode'],'before':item.get('before'),'after':item['after'],'entirePriorBytePrefixPreserved':item['mode']=='append'})
        finally:
            if tmp.exists():tmp.unlink()
    need(all(read(pathlib.Path(p))[1]==row for p,row in held.items()),'all source/evidence inputs unchanged after writes')
    result={'format':'nano-journal-batch-source-write-receipt-v1','parent':d['parent'],'plan':content(planrow),'files':written,'passed':True,'newBuilds':0,'newRuntimeInvocations':0,'gitCommitCreated':False,'githubPushed':False,'productionQualified':False}
    fd=os.open(receipt,os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,0o400)
    with os.fdopen(fd,'w')as f:json.dump(result,f,sort_keys=True,indent=2);f.write('\n');f.flush();os.fsync(f.fileno())
    print(json.dumps({'passed':True,'files':len(written),'gitCommitCreated':False,'productionQualified':False}))

if __name__=='__main__':main()
