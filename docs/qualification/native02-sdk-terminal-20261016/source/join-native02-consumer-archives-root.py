"""ROOT-only, post-build value coverage of the archives actually linked.

Never invokes a compiler or node. Historical output rows remain per call, with
all equal candidates retained. No ar causal/whole-DAG/runtime claim is possible.
"""
from pathlib import Path
import collections,hashlib,json,os,stat,time,sys,re
E=Path('/Users/boris/Documents/Codex/2026-09-20/users-boris-dev-blockchain-python/qualification-resume-20261012')
B=Path('/Users/boris/Dev/blockchain-nano/.deps/p2p-source-20261016-native-02')
C=B/'source-sdk-consumer-qualification-v1'
OUT=E/'native02-consumer-archive-value-join-root-20261016'
K=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')
held={};prior={};created=False
def need(v,m):
    if not v:raise RuntimeError(m)
def nine(s):return {k:getattr(s,k)for k in K}
def read(path,expected=None,admit=True):
    path=Path(path);need(path.resolve(strict=True)==path,'physical input path')
    fd=os.open(path,os.O_RDONLY|os.O_NOFOLLOW)
    try:
        before=os.fstat(fd);need(stat.S_ISREG(before.st_mode)and before.st_nlink>=1 and before.st_size<=256*1024*1024,'ordinary bounded input')
        parts=[]
        while chunk:=os.read(fd,1048576):parts.append(chunk)
        raw=b''.join(parts);need(nine(before)==nine(os.fstat(fd))==nine(path.lstat()),'FD/path changed')
        row={**nine(before),'sha256':hashlib.sha256(raw).hexdigest()}
        for authority in (expected,prior.get(str(path)),held.get(str(path))):
            if authority is not None:need(row==authority,'existing authority mismatch; never refresh')
        if admit:held[str(path)]=row
        return raw,row
    finally:os.close(fd)
def load(path,expected=None):return json.loads(read(path,expected)[0])
def load_pin(path,pin):
    raw,row=read(path);need(set(pin)=={'sha256','bytes'}and row['sha256']==pin['sha256']and len(raw)==pin['bytes'],'producer byte commitment mismatch');return json.loads(raw)
def save(name,value):
    raw=(json.dumps(value,sort_keys=True,separators=(',',':'))+'\n').encode();fd=os.open(OUT/name,os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,0o400)
    try:
        view=memoryview(raw)
        while view:view=view[os.write(fd,view):]
        os.fsync(fd)
    finally:os.close(fd)
    need((OUT/name).read_bytes()==raw,'output readback')
    return {'sha256':hashlib.sha256(raw).hexdigest(),'bytes':len(raw)}
def ar_members(raw):
    need(raw[:8]==b'!<arch>\n','ordinary BSD ar format');offset=8;out=[]
    while offset<len(raw):
        need(offset+60<=len(raw),'ar header bounds');header=raw[offset:offset+60];need(header[58:60]==b'`\n','ar header marker')
        size=int(header[48:58].strip());need(size>=0,'ar member size');start=offset+60;need(start+size<=len(raw),'ar member bounds')
        name=header[:16].decode('ascii').strip();data=raw[start:start+size]
        if name.startswith('#1/'):
            length=int(name[3:]);need(0<=length<=size,'BSD long name bounds');name,data=data[:length].rstrip(b'\0').decode('utf-8'),data[length:]
        else:name=name.rstrip('/')
        need(name and not name.startswith('/')and name!='//','unparsed name table')
        if name not in ('__.SYMDEF','__.SYMDEF SORTED','__.SYMDEF_64','__.SYMDEF_64 SORTED'):out.append({'ordinal':len(out),'name':name,'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest()})
        offset=start+size+(size%2)
    need(offset==len(raw),'ar final boundary');return out
def main():
    global created
    # Root passes the commitment printed from memory by the consumer producer,
    # never a self-hash invented by rereading this untrusted result path.
    need(len(sys.argv)==3 and re.fullmatch('[0-9a-f]{64}',sys.argv[1])and re.fullmatch('[1-9][0-9]*',sys.argv[2]),'Root-observed consumer result SHA/bytes required')
    result=load_pin(C/'result-public.json',{'sha256':sys.argv[1],'bytes':int(sys.argv[2])})
    need(result['format']=='nano-native02-source-sdk-consumer-functional-public-v1'and result['passed']is True and result['genuineMainMEqualsMd']is True,'successful actual new SDK consumer required')
    need(not OUT.exists(),'new output namespace only');OUT.mkdir(mode=0o700);created=True
    old_path=E/'native02-boringssl-installed-member-join-root-20261016-attempt02/member-join.private.json'
    raw,row=read(old_path);need(row['sha256']=='052f03a4d06fdd7c56df27810342cb9929e2b56bb6f6515af569c3abb04ec65c'and len(raw)==10660646,'original BoringSSL input authority')
    old=json.loads(raw);prior.update(old['receiptInputs'])
    before=load_pin(C/'inputs-before-build.private.json',result['privateInputsBeforePin']);after=load_pin(C/'inputs-post-fence.private.json',result['privateInputsAfterPin'])
    need(after['allHeldFullNineAndShaUnchanged']is True,'consumer strict post-fence')
    need(all(after['files'].get(path)==row for path,row in before['files'].items()),'consumer before/after retained rows disagree')
    for path,row in before['files'].items():
        if path in prior:need(prior[path]==row,'two retained input authorities disagree')
        prior[path]=row
    source_terminal=load(B/'source-build-terminal-public.json');need(source_terminal['passed']is True and source_terminal['exitCode']==0 and source_terminal['sourceOnlyRequested']is True,'source SDK successful terminal')
    helper=E/'fresh-p2p-compiler-receipt-index-source-20261016/compiler_receipt_index.py';raw,row=read(helper)
    need(row['sha256']=='2f47b78018a16a69bb9cd4cf392a661fc21394d99e816e6b1f5ac8160ea2e8a7'and len(raw)==12709,'reviewed pure index')
    namespace={'__name__':'root_owned_pure_receipt_index'};exec(compile(raw,str(helper),'exec'),namespace);index=namespace['index_compiler_receipts']
    categories=collections.Counter();refusals=collections.Counter();by_value=collections.defaultdict(list);calls={};by_output_path=collections.defaultdict(list);by_md_path=collections.defaultdict(list)
    for directory in sorted((B/'calls').iterdir()):
        need(directory.is_dir()and not directory.is_symlink(),'owned call directory')
        record={'id':directory.name}
        for field,name in {'command':'command.json','executed':'executed-command.json','dependenciesBefore':'dependencies-before.json','terminal':'terminal.json','refusal':'refusal.json'}.items():
            path=directory/name;record[field]=load(path)if path.is_file()else None
        indexed=index([record]);entry=indexed['observedRecords'][0];category=entry['category'];categories[category]+=1
        need(category!='incomplete','SDK compiler corpus contains incomplete call')
        if record['command']is not None:need(record['command']['controlSha256']=='c76079388eece6974091c7b73ce6e1a3954a7178f878050f1ba1b59848ea2fc8','actual SDK control namespace')
        if category=='refusal':refusals[record['refusal']['phase']+':'+record['refusal']['reason']]+=1
        terminal=record['terminal'];output=entry['producedOutput']
        call={'category':category,'producedOutputAtCall':output,'dependencyCount':None if terminal is None else terminal['dependencyCount'],'independentMEqualsActualMd':False if terminal is None else terminal['independentMEqualsActualMd'],'actualDependencyFileAtCall':None if terminal is None else terminal['actualDependencyFile'],'commandReceiptPath':str(directory/'command.json')}
        calls[directory.name]=call
        if call['actualDependencyFileAtCall']is not None:by_md_path[call['actualDependencyFileAtCall']['resolvedPath']].append(directory.name)
        if output is not None:
            by_value[(output['bytes'],output['sha256'])].append(directory.name)
            by_output_path[output['resolvedPath']].append(directory.name)
    archive_results=[];totals=collections.Counter();selected_ids=set()
    for archive_occurrence,path in enumerate(before['orderedArchivePaths']):
        physical=Path(path).resolve(strict=True);need(physical.is_relative_to(B/'deps'),'actual consumer archive leaves fresh SDK')
        need(str(physical)in before['files'],'consumer archive authority missing')
        raw,row=read(physical,before['files'][str(physical)]);members=ar_members(raw);joined=[]
        for member in members:
            candidates=by_value.get((member['bytes'],member['sha256']),[]);qualified=[i for i in candidates if calls[i]['category']=='compileMEqualsMd']
            selected_ids.update(candidates);totals['memberOccurrences']+=1;totals['withAnyValueCandidate']+=bool(candidates);totals['withQualifiedCompileValueCandidate']+=bool(qualified);totals['withOneObservedQualifiedCompileCandidate']+=len(qualified)==1;totals['withMultipleQualifiedCompileCandidates']+=len(qualified)>1;totals['withoutQualifiedCompileValueCandidate']+=not bool(qualified)
            joined.append({**member,'allValueCandidateIds':candidates,'qualifiedCompileValueCandidateIds':qualified,'causalArchiveProducerProven':False})
        archive_results.append({'archiveOccurrence':archive_occurrence,'path':str(physical),'material':row,'orderedNonIndexMembers':joined,'actualArchiverExecutionProven':False})
    # Current observations never replace historical per-call output identities.
    current={};output_paths={calls[i]['producedOutputAtCall']['resolvedPath']for i in selected_ids}
    for name in sorted(output_paths):
        path=Path(name);observations=by_output_path[name]
        if not path.exists():current[name]={'present':False,'historicalCandidateIds':observations};continue
        _,now=read(path,admit=False)
        matches=[]
        for i in observations:
            old=calls[i]['producedOutputAtCall'];historical={**old['material'],'sha256':old['sha256']}
            matches.append({'id':i,'contentMatches':(now['sha256'],now['st_size'])==(old['sha256'],old['bytes']),'fullNineAndShaMatches':now==historical})
        current[name]={'present':True,'observation':now,'allHistoricalComparisons':matches,'historicalAuthoritiesUpdated':False}
    current_md={};md_paths={calls[i]['actualDependencyFileAtCall']['resolvedPath']for i in selected_ids if calls[i]['actualDependencyFileAtCall']is not None}
    for name in sorted(md_paths):
        path=Path(name);observations=by_md_path[name]
        if not path.exists():current_md[name]={'present':False,'historicalCandidateIds':observations};continue
        _,now=read(path,admit=False);matches=[]
        for i in observations:
            old=calls[i]['actualDependencyFileAtCall'];historical={**old['material'],'sha256':old['sha256']}
            matches.append({'id':i,'contentMatches':(now['sha256'],now['st_size'])==(old['sha256'],old['bytes']),'fullNineAndShaMatches':now==historical})
        current_md[name]={'present':True,'observation':now,'allHistoricalComparisons':matches,'historicalAuthoritiesUpdated':False}
    started=time.monotonic()
    for sample in range(3):
        if sample:time.sleep(1.05)
        for path,row in held.items():need(nine(Path(path).lstat())=={k:row[k]for k in K},'receipt/archive metadata changed during first observations')
    input_span_ms=round((time.monotonic()-started)*1000);need(input_span_ms>=2000,'input first observation span')
    for path,row in list(held.items()):read(Path(path),row)
    descriptor={'format':'nano-native02-consumer-archive-value-coverage-private-v1','receiptAndArchiveInputs':held,'compilerCalls':calls,'orderedArchives':archive_results,'currentObjectObservations':current,'currentMdObservations':current_md,'categories':dict(categories),'refusals':dict(refusals),'totals':dict(totals),'inputSamples':3,'inputSpanMs':input_span_ms,'oldAuthoritiesRefreshed':False,'causalArchiveProducersQualified':False,'wholeDagQualified':False}
    pin=save('join.private.json',descriptor)
    public={'format':'nano-native02-consumer-archive-value-coverage-public-v1','sourceSdkCompleted':True,'consumerCompleted':True,'orderedArchiveOccurrences':len(archive_results),'distinctArchiveFiles':len({x['path']for x in archive_results}),'compilerCallCategories':dict(categories),'compilerRefusals':dict(refusals),**dict(totals),'selectedHistoricalCompilerCallCandidates':len(selected_ids),'currentSelectedObjectPathsPresent':sum(x['present']for x in current.values()),'currentSelectedObjectPathsAbsent':sum(not x['present']for x in current.values()),'currentSelectedMdPathsPresent':sum(x['present']for x in current_md.values()),'currentSelectedMdPathsAbsent':sum(not x['present']for x in current_md.values()),'privateDescriptor':pin,'scope':'byte value coverage of actual consumer archive member occurrences; counts are not translation-unit counts or proof of causal ar producers','archiverExecutionQualified':False,'generatedSourceProducersAllQualified':False,'wholeDagQualified':False,'runtimeQualified':False,'campaignAdmission':False,'oldAuthoritiesRefreshed':False,'productionQualified':False}
    result_pin=save('result-public.json',public);print(json.dumps({'result':public,'pin':result_pin}),flush=True)
if __name__=='__main__':
    try:main()
    except BaseException as error:
        if created:
            save('failure.private.json',{'errorType':type(error).__name__,'reason':str(error),'heldInputs':held,'oldAuthoritiesRefreshed':False})
            save('failure-public.json',{'passed':False,'errorType':type(error).__name__,'reason':str(error),'oldAuthoritiesRefreshed':False,'productionQualified':False})
        raise
