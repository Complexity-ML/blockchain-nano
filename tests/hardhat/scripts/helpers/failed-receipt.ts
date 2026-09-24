import assert from 'node:assert/strict';
import type {BrowserProvider,TransactionResponse} from 'ethers';

// Submission must succeed. The failure must be mined without exhausting
// the top-level transaction gas limit; nested failures are checked by callers.
export async function failedReceipt(provider:BrowserProvider,label:string,send:()=>Promise<TransactionResponse>) {
 const tx=await send();
 await assert.rejects(()=>tx.wait(),(error:any)=>error.code==='CALL_EXCEPTION'&&error.receipt?.status===0&&error.receipt?.hash===tx.hash);
 const receipt=await provider.send('eth_getTransactionReceipt',[tx.hash]);
 assert(receipt,'failed execution has persisted receipt');
 assert.equal(receipt.status,'0x0');assert.equal(receipt.transactionHash,tx.hash);
 assert.deepEqual(receipt.logs,[],'reverted execution leaves no logs');
 const gas=BigInt(receipt.gasUsed);assert(gas>0n&&gas<tx.gasLimit,'failure is not exhaustion of transaction gas');
 const block=await provider.send('eth_getBlockByHash',[receipt.blockHash,false]);
 assert.equal(block.number,receipt.blockNumber);
 assert.equal(block.transactions[Number(BigInt(receipt.transactionIndex))],tx.hash,'failed transaction included at claimed index');
 assert.equal(BigInt(await provider.send('eth_getTransactionCount',[tx.from,'latest'])),BigInt(tx.nonce)+1n,'failed execution consumes sender nonce');
 return {label,hash:tx.hash,blockHash:receipt.blockHash,blockNumber:receipt.blockNumber,status:receipt.status,gasUsed:receipt.gasUsed,gasLimit:'0x'+tx.gasLimit.toString(16),logs:0};
}
