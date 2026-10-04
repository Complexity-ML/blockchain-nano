import {readFile,writeFile} from 'node:fs/promises';
import {join} from 'node:path';
import {validateStoppedValidators} from '/tmp/nano-rpc-wan-harness-candidate-20261002/tests/multihost/validator-gate.mjs';
import {stoppedReceipt,validateRestoredFinality} from '/tmp/nano-rpc-wan-harness-candidate-20261002/tests/multihost/restore-config.mjs';
import {restoredFinalityAuditRequest,verifyRestoredFinalityAudit} from '/tmp/nano-rpc-wan-harness-candidate-20261002/tests/multihost/restore-offline-audit.mjs';
const output='/tmp/nano-rpc-wan-restore-d1597d9a-20261004-b',prepared=join(output,'prepared');
const json=async path=>JSON.parse(await readFile(path,'utf8'));
const prior=await json(join(output,'restore/stop-receipt.json'));
const terminal=stoppedReceipt(await readFile(join(output,'vps.log'),'utf8'));
const stable=await json(join(output,'observations/result.json'));
const validators=await validateStoppedValidators(output,'capella');
validateRestoredFinality(prior,stable,terminal);
const request=restoredFinalityAuditRequest(await json(join(prepared,'vps-docker/node.json')),prior,stable,terminal);
if(process.argv[2]==='prepare') {
 await writeFile(join(output,'restore/completion-finalized-audit-request.json'),JSON.stringify(request,null,2)+'\n');
 console.log(JSON.stringify({validators,stable,restoredProposals:terminal.validator.proposalsAdmitted,restoredAttestations:terminal.validator.attestationsAdmitted,independentSignerPins:prior.validatorKeys.length,request},null,2));
} else if(process.argv[2]==='verify') {
 const saved=await json(join(output,'restore/completion-finalized-audit-request.json'));
 if(JSON.stringify(saved)!==JSON.stringify(request))throw Error('Independent request changed');
 const audit=await json(join(output,'restore/completion-finalized-audit.json'));
 verifyRestoredFinalityAudit(request,terminal,audit);
 const result={stoppedValidatorHealthVerified:true,independentSignerPins:prior.validatorKeys.length,restoredProposals:terminal.validator.proposalsAdmitted,restoredAttestations:terminal.validator.attestationsAdmitted,stable,finalizedAncestryVerified:true,blocks:audit.blocksIncludingTrustedAnchor,transactions:audit.transactions,originalHarnessExit:1,terminalCompletionVerified:true,networkReplayed:false,productionQualified:false};
 await writeFile(join(output,'restore/completion-result.json'),JSON.stringify(result,null,2)+'\n');console.log(JSON.stringify(result,null,2));
} else throw Error('prepare or verify required');
