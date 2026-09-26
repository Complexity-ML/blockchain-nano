// Test keys only. Regenerate from repository root with the installed Hardhat ethers.
const fs = require('fs');
const { SigningKey, version, toBeHex, computeAddress } = require('../../tests/hardhat/node_modules/ethers');
const n = BigInt('0xfffffffffffffffffffffffffffffffebaaedce6af48a03bbfd25e8cd0364141');
const vectors = [];
for (const key of [1n, 2n, n-1n]) {
  const privateKey = toBeHex(key,32), signer = new SigningKey(privateKey);
  for (const digest of [0n,1n,n-1n,n,n+1n,(1n<<256n)-1n]) {
    const hash=toBeHex(digest,32), sig=signer.sign(hash);
    vectors.push({privateKey,hash,publicKey:signer.publicKey,address:computeAddress(signer.publicKey),r:sig.r,s:sig.s,v:sig.yParity});
  }
}
fs.writeFileSync('tests/fixtures/secp256k1-ethers.json',JSON.stringify({generator:`ethers ${version} SigningKey; public test keys`,vectors},null,2)+'\n');
