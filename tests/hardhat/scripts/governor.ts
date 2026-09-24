import {network,artifacts} from 'hardhat';
import {BrowserProvider,Wallet,ContractFactory,id,ZeroHash} from 'ethers';
import assert from 'node:assert/strict';
import {failedReceipt} from './helpers/failed-receipt.ts';
const connection=await network.create('nano');
try {
 const provider=new BrowserProvider(connection.provider,undefined,{cacheTimeout:-1});provider.pollingInterval=100;
 const owner=new Wallet('0x'+'1'.padStart(64,'0'),provider),other=new Wallet('0x'+'2'.padStart(64,'0'),provider);
 const deploy=async(name:string,...args:any[])=>{const a=await artifacts.readArtifact(name);const c=await new ContractFactory(a.abi,a.bytecode,owner).deploy(...args);await c.waitForDeployment();return c as any;};
 const token=await deploy('DeveloperVotes'),timelock=await deploy('DeveloperTimelock',owner.address),governor=await deploy('DeveloperGovernor',await token.getAddress(),await timelock.getAddress()),target=await deploy('DeveloperGovernedTarget',await timelock.getAddress());
 const outsider=governor.connect(other) as any,foreignToken=token.connect(other) as any,foreignLock=timelock.connect(other) as any;
 const governorAddress=await governor.getAddress(),targetAddress=await target.getAddress();
 const receipts:any[]=[],failures:any[]=[],gas={gasLimit:700000};
 const mined=async(label:string,tx:any)=>{const sent=await tx,r=await sent.wait();assert.equal(r.status,1);receipts.push({label,hash:sent.hash,gasLimit:sent.gasLimit.toString(),gasUsed:r.gasUsed.toString(),blockNumber:r.blockNumber});return r;};
 for(const role of [await timelock.PROPOSER_ROLE(),await timelock.EXECUTOR_ROLE(),await timelock.CANCELLER_ROLE()]){
  await mined('grant-governor-role',timelock.grantRole(role,governorAddress));await mined('revoke-owner-role',timelock.revokeRole(role,owner.address));assert.equal(await timelock.hasRole(role,owner.address),false);assert.equal(await timelock.hasRole(role,governorAddress),true);
 }
 await mined('renounce-admin',timelock.renounceRole(ZeroHash,owner.address));assert.equal(await timelock.hasRole(ZeroHash,owner.address),false);
 await mined('delegate-owner',token.delegate(owner.address));
 const targets=[targetAddress],values=[0],calldatas=[target.interface.encodeFunctionData('setValue',[73])],description='Nano local successful proposal',descriptionHash=id(description);
 const badDescription='Nano local no-quorum proposal',badHash=id(badDescription),badCalls=[target.interface.encodeFunctionData('setValue',[99])];
 failures.push(await failedReceipt(provider,'below-proposal-threshold',()=>outsider.propose(targets,values,calldatas,'unauthorized proposer',gas)));
 const proposal=await mined('propose',governor.propose(targets,values,calldatas,description));
 const proposalId=await governor.hashProposal(targets,values,calldatas,descriptionHash);
 assert(proposal.logs.some((l:any)=>{try{const e=governor.interface.parseLog(l);return e?.name==='ProposalCreated'&&e.args.proposalId===proposalId;}catch{return false;}}));
 assert.equal(await governor.state.staticCall(proposalId,{blockTag:proposal.blockNumber}),0n);
 await assert.rejects(()=>governor.castVote.staticCall(proposalId,1,{blockTag:proposal.blockNumber}));
 await mined('propose-no-quorum',governor.propose(targets,values,badCalls,badDescription));const badId=await governor.hashProposal(targets,values,badCalls,badHash);
 const snapshot=await governor.proposalSnapshot(proposalId),badSnapshot=await governor.proposalSnapshot(badId);
 const head=async()=>BigInt(await provider.send('eth_blockNumber',[]));
 const advance=async(height:bigint)=>{for(let i=0;await head()<=height;i++){assert(i<40,'block progress stalled');await mined('advance-local-block',owner.sendTransaction({to:owner.address,value:0}));}};
 await advance(snapshot>badSnapshot?snapshot:badSnapshot);
 assert.equal(await governor.state(proposalId),1n);assert.equal(await token.getPastVotes(owner.address,snapshot),1000000n);assert.equal(await token.getPastVotes(other.address,snapshot),0n);
 await mined('cast-owner-vote',governor.castVote(proposalId,1));
 failures.push(await failedReceipt(provider,'duplicate-vote',()=>governor.castVote(proposalId,1,gas)));
 await mined('transfer-after-snapshot',token.transfer(other.address,750000));await mined('delegate-recipient',foreignToken.delegate(other.address));
 assert.equal(await token.getVotes(owner.address),250000n);assert.equal(await token.getVotes(other.address),750000n);
 await mined('cast-zero-historical-weight',outsider.castVote(proposalId,1));
 assert.equal(await governor.hasVoted(proposalId,owner.address),true);assert.equal(await governor.hasVoted(proposalId,other.address),true);
 assert.deepEqual(Array.from(await governor.proposalVotes(proposalId)),[0n,1000000n,0n]);
 assert.equal(await token.getPastVotes(owner.address,snapshot),1000000n);assert.equal(await token.getPastVotes(other.address,snapshot),0n);
 const deadline=await governor.proposalDeadline(proposalId),badDeadline=await governor.proposalDeadline(badId);await advance(deadline>badDeadline?deadline:badDeadline);
 assert.equal(await governor.state(proposalId),4n);assert.equal(await governor.state(badId),3n);
 assert.deepEqual(Array.from(await governor.proposalVotes(badId)),[0n,0n,0n]);assert.equal(await target.value(),0n);
 failures.push(await failedReceipt(provider,'queue-without-quorum',()=>governor.queue(targets,values,badCalls,badHash,gas)));
 failures.push(await failedReceipt(provider,'execute-before-queue',()=>outsider.execute(targets,values,calldatas,descriptionHash,gas)));
 const queued=await mined('queue',outsider.queue(targets,values,calldatas,descriptionHash));assert.equal(await governor.state(proposalId),5n);
 await assert.rejects(()=>outsider.execute.staticCall(targets,values,calldatas,descriptionHash,{blockTag:queued.blockNumber}));
 failures.push(await failedReceipt(provider,'direct-timelock-bypass',()=>foreignLock.execute(targetAddress,0,calldatas[0],ZeroHash,descriptionHash,gas)));
 const eta=await governor.proposalEta(proposalId);
 for(let i=0;BigInt((await provider.send('eth_getBlockByNumber',['latest',false])).timestamp)<eta;i++){assert(i<10);await new Promise(r=>setTimeout(r,1000));await mined('advance-timelock',owner.sendTransaction({to:owner.address,value:0}));}
 const executed=await mined('execute',outsider.execute(targets,values,calldatas,descriptionHash));assert.equal(await governor.state(proposalId),7n);assert.equal(await target.value(),73n);
 assert(executed.logs.some((l:any)=>{try{return governor.interface.parseLog(l)?.name==='ProposalExecuted';}catch{return false;}}));
 failures.push(await failedReceipt(provider,'replayed-proposal-execution',()=>outsider.execute(targets,values,calldatas,descriptionHash,gas)));
 assert.equal(await target.value(),73n);assert.equal(await governor.state(proposalId),7n);assert.equal(await governor.state(badId),3n);
 assert.equal(await token.getPastVotes(owner.address,snapshot),1000000n);assert.equal(await token.getPastVotes(other.address,snapshot),0n);
 console.log(JSON.stringify({scenario:'hardhat-governor-lifecycle',fork:process.env.NANO_TEST_EVM_FORK,openzeppelin:'5.0.2',proposalId:proposalId.toString(),defeatedProposalId:badId.toString(),snapshot:snapshot.toString(),historicalVotes:['1000000','0'],currentVotes:['250000','750000'],countedVotes:['0','1000000','0'],quorum:'500000',successfulState:7,defeatedState:3,finalTargetValue:'73',adminRenounced:true,governorOnlyTimelockRoles:true,earlyVoteAndExecutionCallsRejected:true,defaultGasEstimation:true,receipts,failedReceipts:failures,scope:'Two local Governor proposals: successful historical-weight vote and no-quorum defeat; no production governance deployment or complete Governor extension catalogue'}));
} finally {await connection.close();}
