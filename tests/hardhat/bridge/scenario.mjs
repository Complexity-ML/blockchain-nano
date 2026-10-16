// Synthetic protocol scenario only. The caller supplies all execution backends.
// No RPC, filesystem, signer, installation or backend process is created here.
export async function runScenario({backend, artifacts, ethers}) {
  const need = (ok, why) => { if (!ok) throw Error('Bridge scenario: ' + why); };
  const cases = [], calls = {deploy: 0, read: 0, send: 0, events: 0, reload: 0};
  const checkCase = async (name, body) => {
    need(cases.length < 160 && name.length <= 120, 'bounded case list');
    await body(); cases.push({name, passed: true});
  };
  const invoke = async (method, ...args) => {
    ++calls[method];
    need(calls.read < 4800 && Object.values(calls).reduce((a, b) => a + b, 0) < 5000,
      'backend/read request budget');
    return await backend[method](...args);
  };
  for (const name of ['deploy', 'read', 'send', 'events', 'reload']) {
    need(typeof backend?.[name] === 'function', 'backend ' + name);
  }
  const names = ['MockProofVerifier', 'BridgeToken', 'MockCanonicalEscrow',
    'MockMintBurnGateway', 'BridgeReentrantRecipient'];
  for (const name of names) need(Array.isArray(artifacts?.[name]?.abi), 'artifact ' + name);
  const abi = ethers.AbiCoder.defaultAbiCoder(), interfaces = Object.fromEntries(
    names.map(name => [name, new ethers.Interface(artifacts[name].abi)]));
  const accounts = [1, 2, 3].map(n => new ethers.Wallet('0x' + String(n).padStart(64, '0')).address);
  const lower = value => String(value).toLowerCase(), equalAddress = (a, b) => lower(a) === lower(b);
  const zero = ethers.ZeroAddress, zeroHash = ethers.ZeroHash, cap = 1000n;
  const chainIds = [31337n, 31338n]; // backend indices remain 0 and 1.
  const domains = ['nano-bridge-synthetic-origin-v1', 'nano-bridge-synthetic-destination-v1']
    .map(x => ethers.keccak256(ethers.toUtf8Bytes(x)));
  const lockDomain = ethers.keccak256(ethers.toUtf8Bytes('synthetic-lock-verifier-v1'));
  const burnDomain = ethers.keccak256(ethers.toUtf8Bytes('synthetic-burn-verifier-v1'));
  const typeString = 'NanoBridgeMessageV1(uint8 kind,uint256 sourceChainId,bytes32 sourceDomain,address sourceEndpoint,uint256 destinationChainId,bytes32 destinationDomain,address destinationEndpoint,address sourceToken,address destinationToken,uint256 nonce,address sender,address recipient,uint256 amount,bytes32 verifierDomain)';
  const typeHash = ethers.keccak256(ethers.toUtf8Bytes(typeString));
  const fields = ['kind', 'sourceChainId', 'sourceDomain', 'sourceEndpoint', 'destinationChainId',
    'destinationDomain', 'destinationEndpoint', 'sourceToken', 'destinationToken', 'nonce',
    'sender', 'recipient', 'amount', 'verifierDomain'];
  const types = ['uint8', 'uint256', 'bytes32', 'address', 'uint256', 'bytes32', 'address',
    'address', 'address', 'uint256', 'address', 'address', 'uint256', 'bytes32'];
  const message = value => Object.fromEntries(fields.map((name, i) => [name, value[name] ?? value[i]]));
  const digest = value => ethers.keccak256(abi.encode(['bytes32', ...types],
    [typeHash, ...fields.map(name => value[name])]));
  const key = value => ethers.keccak256(abi.encode(['uint8', 'uint256', 'bytes32', 'address', 'uint256'],
    [value.kind, value.sourceChainId, value.sourceDomain, value.sourceEndpoint, value.nonce]));
  const proof = (value, alternate = false) => abi.encode(alternate ? ['bytes32', 'uint256'] : ['bytes32'],
    alternate ? [digest(value), 1n] : [digest(value)]);
  const json = value => JSON.stringify(value, (_, v) => typeof v === 'bigint' ? v.toString() : v);
  const scalar = async (c, address, name, method, args = []) => {
    const result = await invoke('read', c, address, name, method, args);
    need(result && Object.hasOwn(result, '0'), 'decoded scalar Result for ' + method);
    return result[0];
  };
  let gasUsed = 0n, expectedRefusals = 0;
  const send = async (c, address, name, method, args = [], from = 0) => {
    const receipt = await invoke('send', c, address, name, method, args, from);
    need(typeof receipt?.success === 'boolean' && Array.isArray(receipt.logs) &&
      typeof receipt.gasUsed === 'string' && /^(?:0|[1-9][0-9]*)$/.test(receipt.gasUsed),
    'real receipt shape for ' + method);
    gasUsed += BigInt(receipt.gasUsed); return receipt;
  };
  const ok = async (...args) => { const r = await send(...args); need(r.success, 'successful ' + args[3]); return r; };
  const pendingLocks = new Map(), pendingBurns = new Map(), reloadChecks = [];
  const predicted = ethers.getCreateAddress({from: accounts[0], nonce: 2});
  let origin, destination;
  const configFor = (c, verifier, token) => ({localChainId: chainIds[c], localDomain: domains[c],
    remoteChainId: chainIds[1 - c], remoteDomain: domains[1 - c], remoteEndpoint: predicted,
    localToken: token, remoteToken: ethers.getCreateAddress({from: accounts[0], nonce: 1}),
    inboundVerifierDomain: c === 0 ? burnDomain : lockDomain,
    outboundVerifierDomain: c === 0 ? lockDomain : burnDomain,
    verifier, maxProofBytes: 64n, maxOutstanding: cap});
  const makeMessage = (kind, nonce = 1n, amount = 1n, recipient = accounts[1]) => {
    const source = kind === 0 ? origin : destination, target = kind === 0 ? destination : origin;
    return {kind, sourceChainId: chainIds[source.c], sourceDomain: domains[source.c],
      sourceEndpoint: source.endpoint, destinationChainId: chainIds[target.c],
      destinationDomain: domains[target.c], destinationEndpoint: target.endpoint,
      sourceToken: source.token, destinationToken: target.token, nonce, sender: accounts[0],
      recipient, amount, verifierDomain: kind === 0 ? lockDomain : burnDomain};
  };
  const endpointRead = (role, method, args = []) => scalar(role.c, role.endpoint, role.name, method, args);
  const tokenRead = (role, method, args = []) => scalar(role.c, role.token, 'BridgeToken', method, args);
  const approveMessage = async (value, target = Number(value.kind) === 0 ? destination : origin) => {
    await ok(target.c, target.verifier, 'MockProofVerifier', 'approveDigest', [digest(value), true]);
  };
  const modes = async (role, transfer = 0, transferFrom = 0, mint = 0, burn = 0) =>
    await ok(role.c, role.token, 'BridgeToken', 'configureModes', [transfer, transferFrom, mint, burn]);
  const parseEvent = (role, logs, eventName) => {
    const found = [];
    for (const log of logs) if (equalAddress(log.address, role.endpoint)) {
      const decoded = interfaces[role.name].parseLog(log);
      need(decoded, 'decodable endpoint event');
      if (decoded.name === eventName) found.push(decoded);
    }
    need(found.length === 1, 'one actual ' + eventName + ' event');
    const m = message(found[0].args.message);
    need(found[0].args.digest === digest(m) && found[0].args.sourceKey === key(m),
      'event commitment independently encoded'); return m;
  };
  const snapshot = async (role, inbound = null, receiver = null) => {
    const nextNonce = BigInt(await endpointRead(role, 'nextNonce'));
    const state = {outstanding: await endpointRead(role, 'outstanding'), nextNonce,
      supply: await tokenRead(role, 'totalSupply'), balances: [], allowances: [], outbox: []};
    const sink = await tokenRead(role, 'SUPPLY_SINK');
    const addresses = [...new Set([...accounts, role.endpoint, sink, ...(receiver ? [receiver] : [])].map(lower))];
    for (const address of addresses) state.balances.push([address, await tokenRead(role, 'balanceOf', [address])]);
    for (const account of accounts) state.allowances.push(await tokenRead(role, 'allowance', [account, role.endpoint]));
    for (const nonce of [...new Set([nextNonce, nextNonce > 1n ? nextNonce - 1n : 1n])]) {
      state.outbox.push([nonce, await endpointRead(role, 'outbox', [nonce])]);
    }
    if (inbound) state.consumed = [key(inbound), await endpointRead(role, 'consumed', [key(inbound)])];
    state.endpointLogs = await invoke('events', role.c, role.endpoint);
    state.tokenLogs = await invoke('events', role.c, role.token);
    if (receiver) {
      state.receiver = [];
      for (const getter of ['callbackCount', 'reentryAttempted', 'reentrySucceeded', 'reentryResult', 'lastFrom', 'lastAmount']) {
        state.receiver.push(await scalar(role.c, receiver, 'BridgeReentrantRecipient', getter));
      }
    }
    return json(state);
  };
  const refused = async (label, role, method, args, {from = 0, inbound = null, receiver = null} = {}) => {
    await checkCase(label, async () => {
      const before = await snapshot(role, inbound, receiver);
      const result = await send(role.c, role.endpoint, role.name, method, args, from);
      need(result.success === false && result.logs.length === 0, 'status false and zero logs: ' + label);
      need(await snapshot(role, inbound, receiver) === before, 'full relevant rollback: ' + label);
      ++expectedRefusals;
    });
  };
  const sum = entries => [...entries.values()].reduce((n, m) => n + BigInt(m.amount), 0n);
  const conservation = async () => {
    const reserve = BigInt(await endpointRead(origin, 'outstanding'));
    const supply = BigInt(await tokenRead(destination, 'totalSupply'));
    const gateway = BigInt(await endpointRead(destination, 'outstanding'));
    const actual = BigInt(await tokenRead(origin, 'balanceOf', [origin.endpoint]));
    need(reserve === supply + sum(pendingLocks) + sum(pendingBurns), 'R = S + pending LOCK + pending BURN');
    need(gateway === supply && actual >= reserve, 'gateway supply and real backing');
    return {reserve: reserve.toString(), supply: supply.toString(), pendingLocks: sum(pendingLocks).toString(),
      pendingBurns: sum(pendingBurns).toString(), actualEscrowBalance: actual.toString()};
  };
  const checkHashes = async (role, m) => {
    need(await endpointRead(role, 'messageDigest', [m]) === digest(m), 'contract/Node digest equality');
    need(await endpointRead(role, 'sourceKey', [m]) === key(m), 'contract/Node sourceKey equality');
  };
  const lock = async (amount, recipient = accounts[1], label = 'lock') => {
    let m;
    await checkCase(label, async () => {
      const receipt = await ok(origin.c, origin.endpoint, origin.name, 'lock', [amount, recipient]);
      m = parseEvent(origin, receipt.logs, 'Locked'); pendingLocks.set(digest(m), m);
      need(BigInt(m.amount) === amount && equalAddress(m.recipient, recipient), 'actual lock terms');
      need(await endpointRead(origin, 'outbox', [m.nonce]) === digest(m), 'actual lock outbox');
      await checkHashes(origin, m); await conservation();
    }); return m;
  };
  const mint = async (m, label = 'mint', alternate = false) => {
    await checkCase(label, async () => {
      need(pendingLocks.has(digest(m)), 'mint corresponds to actual pending lock');
      const receipt = await ok(destination.c, destination.endpoint, destination.name, 'mint', [m, proof(m, alternate)]);
      need(digest(parseEvent(destination, receipt.logs, 'Minted')) === digest(m), 'mint event message');
      need(await endpointRead(destination, 'consumed', [key(m)]) === true, 'mint source consumed');
      pendingLocks.delete(digest(m)); await conservation();
    });
  };
  const burn = async (amount, recipient = accounts[0], label = 'burn') => {
    let m;
    await checkCase(label, async () => {
      const receipt = await ok(destination.c, destination.endpoint, destination.name, 'burnForRelease', [amount, recipient], 1);
      m = parseEvent(destination, receipt.logs, 'Burned'); pendingBurns.set(digest(m), m);
      need(BigInt(m.amount) === amount && equalAddress(m.sender, accounts[1]) && equalAddress(m.recipient, recipient), 'actual burn terms');
      need(await endpointRead(destination, 'outbox', [m.nonce]) === digest(m), 'actual burn outbox');
      await checkHashes(destination, m); await conservation();
    }); return m;
  };
  const release = async (m, label = 'release') => {
    await checkCase(label, async () => {
      need(pendingBurns.has(digest(m)), 'release corresponds to actual pending burn');
      const receipt = await ok(origin.c, origin.endpoint, origin.name, 'release', [m, proof(m)], 2);
      need(digest(parseEvent(origin, receipt.logs, 'Released')) === digest(m), 'release event message');
      need(await endpointRead(origin, 'consumed', [key(m)]) === true, 'burn source consumed');
      pendingBurns.delete(digest(m)); await conservation();
    });
  };
  const reload = async role => {
    const before = await snapshot(role);
    const result = await invoke('reload', role.c);
    if (result?.notStateReload === true) {
      need(result.sameRoot === false, 'sentinel cannot claim a State reload');
      reloadChecks.push({chainIndex: role.c, stateReload: false, backendSentinelOnly: true});
    } else {
      need(result?.sameRoot === true, 'backend reload sameRoot');
      reloadChecks.push({chainIndex: role.c, stateReload: true, backendSentinelOnly: false});
    }
    need(await snapshot(role) === before, 'application state/log readback after backend reload check');
  };
  try {
    await checkCase('deploy two isolated domain pairs with equal numeric endpoint addresses', async () => {
      const roles = [];
      for (const c of [0, 1]) {
        const verifier = await invoke('deploy', c, 'MockProofVerifier', []);
        const token = await invoke('deploy', c, 'BridgeToken', [c === 0 ? 'Canonical Fixture' : 'Representation Fixture', c === 0 ? 'CFIX' : 'RFIX', c === 0]);
        const config = configFor(c, verifier, token), name = c === 0 ? 'MockCanonicalEscrow' : 'MockMintBurnGateway';
        const endpoint = await invoke('deploy', c, name, [config]);
        need(equalAddress(endpoint, predicted), 'endpoint nonce two prediction');
        roles.push({c, verifier, token, endpoint, config, name});
      }
      [origin, destination] = roles;
      need(equalAddress(origin.endpoint, destination.endpoint) && equalAddress(origin.token, destination.token), 'same numeric addresses cross-domain');
      need(BigInt(await tokenRead(destination, 'totalSupply')) === 0n, 'representation initial supply zero');
    });
    await ok(1, destination.token, 'BridgeToken', 'setBridge', [destination.endpoint]);
    await ok(0, origin.token, 'BridgeToken', 'faucet', [accounts[0], 4000n]);
    await ok(0, origin.token, 'BridgeToken', 'approve', [origin.endpoint, 4000n]);
    await ok(1, destination.token, 'BridgeToken', 'approve', [destination.endpoint, 2000n], 1);
    const initialLock = makeMessage(0, 1n, 200n), initialBurn = makeMessage(1, 1n, 1n, accounts[0]);
    await approveMessage(initialLock); await approveMessage(initialBurn);
    await refused('closed route rejects lock', origin, 'lock', [1n, accounts[1]]);
    await refused('closed route rejects mint', destination, 'mint', [initialLock, proof(initialLock)], {inbound: initialLock});
    await refused('closed route rejects burn', destination, 'burnForRelease', [1n, accounts[0]], {from: 1});
    await refused('closed route rejects release', origin, 'release', [initialBurn, proof(initialBurn)], {inbound: initialBurn});
    await ok(0, origin.endpoint, origin.name, 'setEnabled', [true]);
    await ok(1, destination.endpoint, destination.name, 'setEnabled', [true]);
    for (const [field, value] of [['localChainId', 31339n], ['localDomain', zeroHash],
      ['remoteDomain', zeroHash], ['maxProofBytes', 0n], ['maxOutstanding', 0n],
      ['localToken', accounts[1]], ['verifier', accounts[1]]]) {
      await checkCase('constructor refuses invalid ' + field, async () => {
        let rejected = false;
        try { await invoke('deploy', 0, origin.name, [{...origin.config, [field]: value}]); }
        catch (error) { if (error?.constructorReverted !== true) throw error; rejected = true; }
        need(rejected, 'actual CREATE failure for ' + field); ++expectedRefusals;
      });
    }
    const first = await lock(200n, accounts[1], 'actual lock creates reserve and pending mint');
    await approveMessage(first);
    const mutations = [1, 31339n, zeroHash, accounts[2], 31339n, zeroHash, accounts[2], accounts[2],
      accounts[2], BigInt(first.nonce) + 1n, accounts[2], accounts[2], BigInt(first.amount) + 1n, zeroHash];
    for (let i = 0; i < fields.length; ++i) {
      const bad = {...first, [fields[i]]: mutations[i]};
      await refused('unchanged proof refuses tampered ' + fields[i], destination, 'mint', [bad, proof(first)], {inbound: bad});
    }
    for (const i of [0, 1, 2, 3, 4, 5, 6, 7, 8, 13]) {
      const bad = {...first, [fields[i]]: mutations[i]}; await approveMessage(bad, destination);
      await refused('even approved proof refuses wrong route ' + fields[i], destination, 'mint', [bad, proof(bad)], {inbound: bad});
    }
    for (const [field, value] of [['amount', 0n], ['sender', zero], ['recipient', zero],
      ['recipient', destination.endpoint], ['nonce', 0n]]) {
      const bad = {...first, [field]: value}; await approveMessage(bad);
      await refused('approved message refuses invalid ' + field + ' ' + String(value), destination, 'mint', [bad, proof(bad)], {inbound: bad});
    }
    for (const [label, bytes] of [['empty', '0x'], ['oversize', '0x' + '11'.repeat(65)], ['altered', '0x' + '22'.repeat(32)]]) {
      await refused(label + ' proof refuses mint', destination, 'mint', [first, bytes], {inbound: first});
    }
    await mint(first, 'mint consumes actual lock exactly once');
    await refused('same caller cannot replay lock', destination, 'mint', [first, proof(first)], {inbound: first});
    await refused('other caller cannot replay lock', destination, 'mint', [first, proof(first)], {from: 2, inbound: first});
    await refused('alternate valid proof cannot replay lock', destination, 'mint', [first, proof(first, true)], {inbound: first});
    const conflicting = {...first, sender: accounts[2], recipient: accounts[2], amount: 201n};
    await approveMessage(conflicting);
    need(key(conflicting) === key(first) && digest(conflicting) !== digest(first), 'payload-independent source key');
    need(await scalar(1, destination.verifier, 'MockProofVerifier', 'verifyMessage', [digest(conflicting), proof(conflicting)]) === true,
      'second contradictory payload actually approved by mock');
    await refused('approved contradictory payload cannot consume source nonce twice', destination, 'mint', [conflicting, proof(conflicting)], {inbound: conflicting});
    let reconstructed;
    await checkCase('lost relayer index reconstructs event and persisted replay refusal', async () => {
      const relayerIndex = new Map([[key(first), first]]);
      relayerIndex.clear(); need(relayerIndex.size === 0, 'local relayer index actually discarded');
      await reload(destination);
      const logs = await invoke('events', 0, origin.endpoint), rebuilt = parseEvent(origin, logs, 'Locked');
      need(digest(rebuilt) === digest(first), 'fresh event-only reconstruction of original ID');
      relayerIndex.set(key(rebuilt), rebuilt); reconstructed = relayerIndex.get(key(rebuilt));
    });
    await refused('reconstructed index does not reset consumed lock', destination, 'mint', [reconstructed, proof(reconstructed)], {from: 2, inbound: reconstructed});
    const b = await burn(40n, accounts[0], 'burn creates pending release without moving reserve');
    await approveMessage(b); await release(b, 'release consumes burn and pays canonical asset');
    await refused('burn release cannot replay for another caller', origin, 'release', [b, proof(b, true)], {from: 1, inbound: b});
    await checkCase('unsolicited canonical donation does not create reserve or outbox credit', async () => {
      const reserveBefore = await endpointRead(origin, 'outstanding'), nonceBefore = await endpointRead(origin, 'nextNonce');
      const outboxBefore = await endpointRead(origin, 'outbox', [nonceBefore]);
      const balanceBefore = BigInt(await tokenRead(origin, 'balanceOf', [origin.endpoint]));
      await ok(0, origin.token, 'BridgeToken', 'transfer', [origin.endpoint, 1n]);
      need(await endpointRead(origin, 'outstanding') === reserveBefore &&
        await endpointRead(origin, 'nextNonce') === nonceBefore &&
        await endpointRead(origin, 'outbox', [nonceBefore]) === outboxBefore,
      'donation changes no bridge liability or source message');
      need(BigInt(await tokenRead(origin, 'balanceOf', [origin.endpoint])) === balanceBefore + 1n,
        'one real donated canonical base unit'); await conservation();
    });
    const capacity = await lock(840n, accounts[1], 'escrow accepts exact outstanding cap');
    await approveMessage(capacity); await mint(capacity, 'gateway accepts exact outstanding cap', true);
    await refused('escrow rejects one base unit over cap', origin, 'lock', [1n, accounts[1]]);
    const extra = makeMessage(0, 999n, 1n); await approveMessage(extra);
    await refused('gateway rejects approved message one unit over cap', destination, 'mint', [extra, proof(extra)], {inbound: extra});
    const capacityBurn = await burn(840n, accounts[0], 'burn frees capped outstanding');
    await approveMessage(capacityBurn); await release(capacityBurn, 'release frees canonical cap');
    await refused('outbound lock rejects remote endpoint recipient', origin, 'lock', [1n, destination.endpoint]);
    await refused('outbound burn rejects remote endpoint recipient', destination, 'burnForRelease', [1n, origin.endpoint], {from: 1});
    await refused('outbound lock refuses zero amount', origin, 'lock', [0n, accounts[1]]);
    await refused('outbound lock refuses uint256 maximum over cap', origin, 'lock', [ethers.MaxUint256, accounts[1]]);
    for (const mode of [1, 2, 3, 4]) {
      await modes(origin, 0, mode);
      await refused('lock transferFrom mode ' + mode + ' rollback', origin, 'lock', [10n, accounts[1]]);
      await modes(origin); const m = await lock(10n, accounts[1], 'lock retry after transferFrom mode ' + mode);
      await approveMessage(m); await mint(m, 'mint corresponding retried lock ' + mode);
    }
    await modes(origin, 0, 5); const noReturnLock = await lock(10n, accounts[1], 'empty-return canonical transferFrom succeeds exactly');
    await modes(origin); await approveMessage(noReturnLock); await mint(noReturnLock, 'mint empty-return lock');
    for (const mode of [1, 2, 3]) {
      const m = await lock(10n, accounts[1], 'lock for failing mint mode ' + mode); await approveMessage(m);
      await modes(destination, 0, 0, mode);
      await refused('mint mode ' + mode + ' rollback and unconsumed source', destination, 'mint', [m, proof(m)], {inbound: m});
      await modes(destination); await mint(m, 'same source retry after mint mode ' + mode);
    }
    for (const mode of [1, 2, 3, 4]) {
      await modes(destination, 0, mode);
      await refused('burn pull transferFrom mode ' + mode + ' rollback', destination, 'burnForRelease', [10n, accounts[0]], {from: 1});
      await modes(destination); const m = await burn(10n, accounts[0], 'burn retry after transferFrom mode ' + mode);
      await approveMessage(m); await release(m, 'release retried burn pull ' + mode);
    }
    await modes(destination, 0, 5); const noReturnBurn = await burn(10n, accounts[0], 'empty-return representation pull succeeds exactly');
    await modes(destination); await approveMessage(noReturnBurn); await release(noReturnBurn, 'release empty-return burn');
    for (const mode of [1, 2, 3]) {
      await modes(destination, 0, 0, 0, mode);
      await refused('burn mode ' + mode + ' rollback', destination, 'burnForRelease', [10n, accounts[0]], {from: 1});
      await modes(destination); const m = await burn(10n, accounts[0], 'burn retry after burn mode ' + mode);
      await approveMessage(m); await release(m, 'release retried burn mode ' + mode);
    }
    for (const mode of [1, 2, 3, 4]) {
      const m = await burn(10n, accounts[0], 'burn for release transfer mode ' + mode); await approveMessage(m);
      await modes(origin, mode);
      await refused('release transfer mode ' + mode + ' rollback', origin, 'release', [m, proof(m)], {inbound: m});
      await modes(origin); await release(m, 'same burn retry after release transfer mode ' + mode);
    }
    const noReturnRelease = await burn(10n, accounts[0], 'burn for empty-return release'); await approveMessage(noReturnRelease);
    await modes(origin, 5); await release(noReturnRelease, 'empty-return canonical release succeeds exactly'); await modes(origin);
    const guardSelector = ethers.id('ReentrancyGuardReentrantCall()').slice(0, 10);
    for (const role of [origin, destination]) {
      const receiver = await invoke('deploy', role.c, 'BridgeReentrantRecipient', [role.token]);
      const nested = interfaces[role.name].encodeFunctionData(role.c === 0 ? 'lock' : 'burnForRelease', [1n, accounts[1 - role.c]]);
      await ok(role.c, receiver, 'BridgeReentrantRecipient', 'configureReentry', [role.endpoint, nested, false]);
      await ok(role.c, role.token, 'BridgeToken', 'configureRecipientCallback', [receiver, true]);
      const make = async label => role.c === 0 ? await burn(5n, receiver, label) : await lock(5n, receiver, label);
      const finish = async (m, label) => role.c === 0 ? await release(m, label) : await mint(m, label);
      const m = await make('outbound for recipient cross-function callback chain ' + role.c); await approveMessage(m);
      await finish(m, 'successful outer call blocks cross-function callback chain ' + role.c);
      need(await scalar(role.c, receiver, 'BridgeReentrantRecipient', 'callbackAttempted') === true &&
        await scalar(role.c, receiver, 'BridgeReentrantRecipient', 'callbackBlocked') === true &&
        lower(await scalar(role.c, receiver, 'BridgeReentrantRecipient', 'reentryResult')) === lower(guardSelector),
      'exact committed reentrancy error');
      await ok(role.c, receiver, 'BridgeReentrantRecipient', 'configureReentry', [role.endpoint, nested, true]);
      const failing = await make('outbound for recipient refusal chain ' + role.c); await approveMessage(failing);
      await refused('recipient revert rolls back callback and bridge chain ' + role.c, role, role.c === 0 ? 'release' : 'mint',
        [failing, proof(failing)], {inbound: failing, receiver});
      await ok(role.c, receiver, 'BridgeReentrantRecipient', 'configureReentry', [role.endpoint, nested, false]);
      await finish(failing, 'same source retry after recipient refusal chain ' + role.c);
      await ok(role.c, role.token, 'BridgeToken', 'configureRecipientCallback', [zero, false]);
    }
    for (const role of [destination, origin]) {
      const m = role.c === 1 ? await lock(3n, accounts[1], 'lock for verifier callback') : await burn(3n, accounts[0], 'burn for verifier callback');
      await approveMessage(m);
      const nested = interfaces[role.name].encodeFunctionData(role.c === 0 ? 'lock' : 'burnForRelease', [1n, accounts[1 - role.c]]);
      await ok(role.c, role.verifier, 'MockProofVerifier', 'configureViewCallback', [role.endpoint, nested, guardSelector]);
      if (role.c === 1) await mint(m, 'successful mint proves verifier static reentry was guard-refused');
      else await release(m, 'successful release proves verifier static reentry was guard-refused');
      await ok(role.c, role.verifier, 'MockProofVerifier', 'configureViewCallback', [zero, '0x', '0x00000000']);
    }
    const queuedLock1 = await lock(2n, accounts[1], 'first independent lock remains pending');
    const queuedLock2 = await lock(3n, accounts[1], 'second independent lock remains pending');
    await checkCase('pending obligations persist without a timeout refund entry point', async () => {
      need(pendingLocks.size === 2 && sum(pendingLocks) === 5n && pendingBurns.size === 0,
        'two actual independent locks pending simultaneously');
      for (const role of [origin, destination]) {
        const writers = interfaces[role.name].fragments.filter(f => f.type === 'function' &&
          f.stateMutability !== 'view' && f.stateMutability !== 'pure').map(f => f.name).sort();
        const expected = (role.c === 0 ? ['lock', 'release', 'setEnabled'] :
          ['mint', 'burnForRelease', 'setEnabled']).sort();
        need(json(writers) === json(expected), 'exhaustive protocol writers exclude cancellation/refund/rescue');
      }
      await reload(origin); await reload(destination);
      for (const m of [queuedLock1, queuedLock2]) {
        need(await endpointRead(origin, 'outbox', [m.nonce]) === digest(m) &&
          await endpointRead(destination, 'consumed', [key(m)]) === false,
        'pending lock retains original outbox and remains unconsumed');
      }
      await conservation();
    });
    await approveMessage(queuedLock1); await approveMessage(queuedLock2);
    await mint(queuedLock2, 'deliver second independent lock before first', true);
    await mint(queuedLock1, 'deliver first independent lock after second');
    const queuedBurn1 = await burn(2n, accounts[0], 'first independent burn remains pending');
    const queuedBurn2 = await burn(3n, accounts[0], 'second independent burn remains pending');
    need(pendingBurns.size === 2 && sum(pendingBurns) === 5n, 'two actual independent burns pending simultaneously');
    await approveMessage(queuedBurn1); await approveMessage(queuedBurn2);
    await release(queuedBurn2, 'deliver second independent burn before first');
    await release(queuedBurn1, 'deliver first independent burn after second');
    await checkCase('reload readback retains completed reserve and replay keys', async () => {
      await reload(origin); await reload(destination); await conservation();
      need(await endpointRead(destination, 'consumed', [key(first)]) === true &&
        await endpointRead(origin, 'consumed', [key(b)]) === true, 'old consumed keys survive backend check');
    });
    const last = await lock(3n, accounts[1], 'last real pending lock before deliberate issuer corruption');
    await approveMessage(last); const conservedBeforeCorruption = await conservation();
    await ok(1, destination.token, 'BridgeToken', 'corruptSupply', [accounts[2], 1n]);
    await ok(1, destination.token, 'BridgeToken', 'approve', [destination.endpoint, 1n], 2);
    await refused('external supply corruption freezes mint without consuming lock', destination, 'mint', [last, proof(last)], {inbound: last});
    await refused('external supply corruption freezes counterfeit burn before pull', destination, 'burnForRelease', [1n, accounts[0]], {from: 2});
    need(BigInt(await tokenRead(destination, 'totalSupply')) === BigInt(await endpointRead(destination, 'outstanding')) + 1n,
      'deliberate issuer corruption remains visible, not repaired');
    return {format: 'nano-bridge-common-synthetic-scenario-v1', passed: true, cases, expectedRefusals,
      backendCalls: calls, gasUsed: gasUsed.toString(), chainIds: chainIds.map(String),
      conservedBeforeCorruption, conservationClaimAfterDeliberateCorruption: false,
      supplyCorruptionPersists: true, mintAndBurnFrozenAfterCorruption: true,
      reverseOrderLockAndBurnDeliveryChecked: true, timeoutRefundEntryPointPresent: false,
      elapsedTimeoutQualified: false,
      reloadChecks, reloadScope: 'backend-declared State reload or explicitly labelled sentinel plus application readback; no WAL/process recovery claim',
      proofModel: 'owner-approved synthetic digest; no source inclusion or finality proof',
      sourceFinalityAuthenticated: false, cryptographicFinalityVerified: false,
      realUsdcUsed: false, wrappedNanoModified: false, economicConversionQualified: false,
      consensusQualified: false, productionQualified: false};
  } catch (error) {
    // Preserve the first real failure; never turn a tool/import error into an
    // expected transaction refusal or return a green partial scenario.
    if (error && typeof error === 'object') error.scenarioProgress = {cases, backendCalls: calls, expectedRefusals};
    throw error;
  }
}
