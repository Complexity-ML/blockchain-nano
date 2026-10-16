"""Pure index of already-read a740 compiler-proxy receipts. No I/O or CLI."""
import posixpath
import re

NINE = ('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')
CATEGORIES = ('compileMEqualsMd','forward','returnNonzero','refusal','incomplete')

class ReceiptIndexError(ValueError):
    def __init__(self, record_id, code):
        self.record_id, self.code = record_id, code
        super().__init__(code)

def index_compiler_receipts(records):
    """Preserve references and every observation; never select a winning producer.

    Each record has exactly id, command, executed, dependenciesBefore, terminal,
    refusal. Missing receipt values are None. ROOT owns file admission and reads.
    An indexed resolvedPath is a producer assertion, not proof of a current file.
    """
    observed, by_path = [], {}
    categories = {key:[] for key in CATEGORIES}
    ids = set()
    def check(ok, code):
        if not ok: raise ReceiptIndexError(record_id, code)
    def exact(value, keys, code):
        check(isinstance(value,dict) and set(value) == set(keys), code)
    def integer(value): return type(value) is int
    def sha(value): return isinstance(value,str) and re.fullmatch('[0-9a-f]{64}',value) is not None
    def path(value):
        return isinstance(value,str) and value.startswith('/') and value == posixpath.normpath(value) and not re.search(r'[\x00\r\n]',value)
    def row(value, code):
        exact(value,('resolvedPath','aliases','material','sha256','bytes'),code)
        exact(value['material'],NINE,code+'-nine')
        check(path(value['resolvedPath']) and isinstance(value['aliases'],dict) and sha(value['sha256']) and integer(value['bytes']) and value['bytes'] >= 0,code+'-identity')
        check(all(integer(value['material'][key]) for key in NINE) and value['material']['st_size'] == value['bytes'] and value['material']['st_nlink'] >= 1 and value['material']['st_mode'] & 0o170000 == 0o100000,code+'-regular')
    def events(value):
        check(isinstance(value,list), 'events-list')
        for event in value:
            exact(event,('phase','exitCode','elapsedMs','stdoutBytes','stderrBytes','stdoutSha256','stderrSha256'),'event-schema')
            check(isinstance(event['phase'],str) and (event['exitCode'] is None or integer(event['exitCode'])) and all(integer(event[k]) and event[k] >= 0 for k in ('elapsedMs','stdoutBytes','stderrBytes')) and sha(event['stdoutSha256']) and sha(event['stderrSha256']),'event-values')
    record_id = None
    check(isinstance(records,(list,tuple)), 'records-list')
    for record in records:
        record_id = record.get('id') if isinstance(record,dict) else None
        exact(record,('id','command','executed','dependenciesBefore','terminal','refusal'),'record-schema')
        check(isinstance(record_id,str) and 0 < len(record_id) <= 4096 and not re.search(r'[\x00\r\n]',record_id) and record_id not in ids,'unique-record-id')
        ids.add(record_id)
        command, executed, deps, terminal, refusal = (record[k] for k in ('command','executed','dependenciesBefore','terminal','refusal'))
        check(not (terminal is not None and refusal is not None),'terminal-refusal-conflict')
        if command is not None:
            exact(command,('format','role','cwd','argv','controlSha256','controlBytes','controlMaterial','environmentStored'),'command-schema')
            check(command['format'] == 'nano-p2p-compiler-call-command-v1' and command['role'] in ('cc','cxx') and path(command['cwd']) and isinstance(command['argv'],list) and command['argv'] and all(isinstance(x,str) for x in command['argv']) and sha(command['controlSha256']) and integer(command['controlBytes']) and command['controlBytes'] > 0 and command['environmentStored'] is False,'command-values')
            row(command['controlMaterial'],'control-material')
            check((command['controlMaterial']['sha256'],command['controlMaterial']['bytes']) == (command['controlSha256'],command['controlBytes']),'command-control-pin')
        if executed is not None:
            exact(executed,('kind','actualArgv','promotedMmdToMd','addedActualMd'),'executed-schema')
            check(executed['kind'] in ('compile','forward') and isinstance(executed['actualArgv'],list) and executed['actualArgv'] and all(isinstance(x,str) for x in executed['actualArgv']) and type(executed['promotedMmdToMd']) is bool and type(executed['addedActualMd']) is bool,'executed-values')
            if command is not None: check(executed['actualArgv'][0] == command['argv'][0],'executed-driver')
        if deps is not None:
            exact(deps,('files','metadataObservations','independentDependencyFile'),'dependencies-schema')
            check(isinstance(deps['files'],dict) and 0 < len(deps['files']) <= 20000,'dependency-files')
            for name,value in deps['files'].items():
                check(path(name),'dependency-path'); row(value,'dependency-row')
            exact(deps['metadataObservations'],('samples','spanMs','shaReadsBeforeMetadataSamples'),'dependency-observations-schema')
            check(integer(deps['metadataObservations']['samples']) and deps['metadataObservations']['samples'] >= 3 and integer(deps['metadataObservations']['spanMs']) and deps['metadataObservations']['spanMs'] >= 2000 and integer(deps['metadataObservations']['shaReadsBeforeMetadataSamples']) and deps['metadataObservations']['shaReadsBeforeMetadataSamples'] == 1,'dependency-observations')
            independent = deps['independentDependencyFile']; exact(independent,('path','sha256','bytes'),'independent-file-schema')
            check(path(independent['path']) and sha(independent['sha256']) and integer(independent['bytes']) and independent['bytes'] > 0,'independent-file-pin')
        produced = None; missing = []
        if terminal is not None:
            exact(terminal,('format','passed','role','kind','exitCode','compiler','controlSha256','dependencyQualification','independentMEqualsActualMd','dependencyCount','actualDependencyFile','producedOutput','outputMetadataObservations','events','environmentStored','wholeTransitiveDagQualified','historicalAuthorityRecovered','productionQualified'),'terminal-schema')
            check(terminal['format'] == 'nano-p2p-compiler-call-terminal-v1' and terminal['role'] in ('cc','cxx') and terminal['kind'] in ('compile','forward') and integer(terminal['exitCode']) and type(terminal['passed']) is bool and terminal['passed'] == (terminal['exitCode'] == 0) and sha(terminal['controlSha256']),'terminal-values')
            check(all(terminal[k] is False for k in ('environmentStored','wholeTransitiveDagQualified','historicalAuthorityRecovered','productionQualified')),'terminal-scope')
            for k in ('dependencyQualification','independentMEqualsActualMd'): check(type(terminal[k]) is bool,'terminal-dependency-booleans')
            events(terminal['events'])
            expected_phases = ['independent-m','actual'] if terminal['kind'] == 'compile' else ['actual']
            check([x['phase'] for x in terminal['events']] == expected_phases and terminal['events'][-1]['exitCode'] == terminal['exitCode'],'terminal-event-join')
            if terminal['kind'] == 'compile': check(terminal['events'][0]['exitCode'] == 0,'independent-m-exit')
            compiler = terminal['compiler']; exact(compiler,('path','argv0','sha256','bytes','material'),'compiler-schema')
            exact(compiler['material'],(*NINE,'sha256'),'compiler-nine-schema')
            check(path(compiler['path']) and compiler['argv0'] == ('clang' if terminal['role'] == 'cc' else 'clang++') and sha(compiler['sha256']) and integer(compiler['bytes']) and compiler['bytes'] > 0 and compiler['material']['sha256'] == compiler['sha256'] and all(integer(compiler['material'][k]) for k in NINE) and compiler['material']['st_size'] == compiler['bytes'],'compiler-values')
            check(compiler['material']['st_nlink'] >= 1 and compiler['material']['st_mode'] & 0o170000 == 0o100000,'compiler-regular')
            if command is not None: check(terminal['role'] == command['role'] and terminal['controlSha256'] == command['controlSha256'] and compiler['argv0'] == command['argv'][0],'terminal-command-join')
            if executed is not None: check(terminal['kind'] == executed['kind'],'terminal-executed-join')
            produced = terminal['producedOutput']
            if produced is not None:
                row(produced,'produced-output'); check(produced['material']['st_nlink'] == 1 and terminal['passed'],'produced-output-owner-contract')
                if command is not None: check(all(produced['material'][k] == command['controlMaterial']['material'][k] for k in ('st_uid','st_gid')),'produced-owner-join')
                samples = terminal['outputMetadataObservations']; exact(samples,('samples','spanMs','firstMetadataAdmissionOnly'),'output-observations-schema')
                check(integer(samples['samples']) and samples['samples'] >= 3 and integer(samples['spanMs']) and samples['spanMs'] >= 2000 and samples['firstMetadataAdmissionOnly'] is True,'output-observations')
            else: check(terminal['outputMetadataObservations'] is None,'absent-output-observations')
            compile_pass = terminal['kind'] == 'compile' and terminal['passed']
            check(terminal['dependencyQualification'] == compile_pass and terminal['independentMEqualsActualMd'] == compile_pass,'terminal-dependency-claim')
            if terminal['kind'] == 'compile':
                check(integer(terminal['dependencyCount']) and terminal['dependencyCount'] > 0,'compile-dependency-count')
                if deps is not None: check(terminal['dependencyCount'] == len(deps['files']),'dependency-count-join')
            else: check(terminal['dependencyCount'] is None and terminal['actualDependencyFile'] is None and deps is None,'forward-no-dependency-claim')
            if compile_pass:
                check(produced is not None and terminal['actualDependencyFile'] is not None,'compile-output-md-required')
                row(terminal['actualDependencyFile'],'actual-md')
                check(terminal['actualDependencyFile']['material']['st_nlink'] == 1 and terminal['actualDependencyFile']['resolvedPath'] != produced['resolvedPath'],'distinct-object-md')
                check(all(terminal['actualDependencyFile']['material'][k] == produced['material'][k] for k in ('st_uid','st_gid')),'md-owner-join')
                check([x['phase'] for x in terminal['events']] == ['independent-m','actual'] and all(x['exitCode'] == 0 for x in terminal['events']),'compile-events')
            elif terminal['kind'] == 'compile': check(terminal['actualDependencyFile'] is None,'failed-compile-no-md-output')
            missing = [key for key,value in (('command',command),('executed',executed),('dependenciesBefore',deps if compile_pass else True)) if value is None]
            category = 'incomplete' if missing else ('returnNonzero' if not terminal['passed'] else ('compileMEqualsMd' if compile_pass else 'forward'))
        elif refusal is not None:
            exact(refusal,('format','passed','phase','errorType','reason','detailsSuppressed','events','environmentStored','productionQualified'),'refusal-schema')
            check(refusal['format'] == 'nano-p2p-compiler-call-refused-v1' and refusal['passed'] is False and refusal['detailsSuppressed'] is True and refusal['environmentStored'] is False and refusal['productionQualified'] is False and all(isinstance(refusal[k],str) for k in ('phase','errorType','reason')),'refusal-values')
            events(refusal['events']); category = 'refusal'
        else: category = 'incomplete'; missing = ['terminalOrRefusal']
        entry = {'id':record_id,'category':category,'record':record,'producedOutput':produced,'missingReceipts':missing}
        observed.append(entry); categories[category].append(entry)
        if produced is not None: by_path.setdefault(produced['resolvedPath'],[]).append(entry)
    divergences = {}
    for name,entries in by_path.items():
        pins = {(x['producedOutput']['sha256'],x['producedOutput']['bytes']) for x in entries}
        identities = {tuple(x['producedOutput']['material'][k] for k in NINE)+(x['producedOutput']['sha256'],) for x in entries}
        divergences[name] = {'producerObservations':len(entries),'distinctContentPins':len(pins),'distinctMaterialPins':len(identities),'contentDiverges':len(pins) > 1,'materialDiverges':len(identities) > 1}
    return {'observedRecords':observed,'byProducedPath':by_path,'categories':categories,'divergences':divergences,'wholeSdkQualified':False,'wholeTransitiveDagQualified':False,'currentPhysicalObjectsVerified':False}
