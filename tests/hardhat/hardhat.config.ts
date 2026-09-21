import { defineConfig } from "hardhat/config";
import { createRequire } from "node:module";
const require = createRequire(import.meta.url);
const url = process.env.NANO_TEST_RPC;
if (!url || new URL(url).hostname !== "127.0.0.1") throw new Error("NANO_TEST_RPC must target the local test node");
export default defineConfig({
  solidity: { version: "0.8.28", path: require.resolve("solc/soljson.js"), settings: { evmVersion: "paris", optimizer: { enabled: true, runs: 200 } } },
  networks: { nano: { type: "http", url, chainId: 1337 } },
});
