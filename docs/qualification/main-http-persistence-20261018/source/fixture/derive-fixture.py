"""Pure public-data derivation. No SDK, Keccak, signatures, node or network."""
from pathlib import Path
import hashlib
import json
import sys

SOURCE_PIN = {'sha256': 'a6524e0dffedf85740802755700d1984aa4bca40d880f5075751001514ba8f2e', 'bytes': 11527}
CASE = 'tests/berlin/eip2929_gas_cost_increases/test_call.py::test_call_insufficient_balance[fork_Shanghai-blockchain_test_from_state_test]'

def pin(raw):
    return {'sha256': hashlib.sha256(raw).hexdigest(), 'bytes': len(raw)}

def encoded_json(value):
    return (json.dumps(value, indent=2) + '\n').encode()

def integer(value):
    n = int(value, 16)
    if n < 0:
        raise ValueError('unsigned integer required')
    return n.to_bytes((n.bit_length() + 7) // 8, 'big')

def prefix(length, base):
    if length < 56:
        return bytes([base + length])
    size = integer(hex(length))
    return bytes([base + 55 + len(size)]) + size

def rlp(value):
    if isinstance(value, list):
        body = b''.join(rlp(item) for item in value)
        return prefix(len(body), 0xc0) + body
    if type(value) is not bytes:
        raise ValueError('bytes or list required')
    if len(value) == 1 and value[0] < 0x80:
        return value
    return prefix(len(value), 0x80) + value

def bounds(raw, at):
    if not 0 <= at < len(raw):
        raise ValueError('RLP offset')
    lead = raw[at]
    if lead < 0x80:
        return at, at + 1, False, at + 1
    is_list = lead >= 0xc0
    base = 0xc0 if is_list else 0x80
    if lead < base + 56:
        start = at + 1
        length = lead - base
    else:
        length_size = lead - base - 55
        start = at + 1 + length_size
        if start > len(raw) or raw[at + 1] == 0:
            raise ValueError('RLP length encoding')
        length = int.from_bytes(raw[at + 1:start], 'big')
        if length < 56:
            raise ValueError('noncanonical long RLP')
    end = start + length
    if end > len(raw) or (not is_list and length == 1 and raw[start] < 0x80):
        raise ValueError('RLP payload')
    return start, end, is_list, end

def children(raw, at):
    start, end, is_list, _ = bounds(raw, at)
    if not is_list:
        raise ValueError('RLP list required')
    rows = []
    while start < end:
        next_at = bounds(raw, start)[3]
        if next_at > end:
            raise ValueError('RLP child outside parent')
        rows.append((start, next_at))
        start = next_at
    return rows

def derive(source):
    if pin(source) != SOURCE_PIN:
        raise ValueError('known public source bytes')
    fixture = json.loads(source)
    if set(fixture) != {CASE}:
        raise ValueError('one exact public fixture')
    case = fixture[CASE]
    if case['network'] != 'Shanghai' or len(case['blocks']) != 1:
        raise ValueError('Shanghai single-block fixture')
    block = case['blocks'][0]
    if len(block['transactions']) != 1 or len(block['receipts']) != 1:
        raise ValueError('one existing signed transaction and receipt')
    tx, receipt = block['transactions'][0], block['receipts'][0]
    if int(tx['type'], 16) != 0 or (int(tx['v'], 16) - 35) // 2 != 1 or int(tx['chainId'], 16) != 1:
        raise ValueError('existing EIP-155 legacy chain-one signature')
    address = bytes.fromhex(tx['to'][2:])
    if len(address) != 20:
        raise ValueError('twenty-byte recipient')
    fields = [integer(tx[k]) for k in ('nonce', 'gasPrice', 'gasLimit')]
    fields += [address, integer(tx['value']), bytes.fromhex(tx['data'][2:])]
    fields += [integer(tx[k]) for k in ('v', 'r', 's')]
    raw = rlp(fields)
    source_block = bytes.fromhex(block['rlp'][2:])
    if bounds(source_block, 0)[3] != len(source_block):
        raise ValueError('source block trailing bytes')
    outer = children(source_block, 0)
    tx_spans = children(source_block, outer[1][0])
    if len(tx_spans) != 1 or source_block[slice(*tx_spans[0])] != raw:
        raise ValueError('pure encoding must equal original signed RLP span')
    header = case['genesisBlockHeader']
    genesis = {'config': {'chainId': 1, 'consensus': 'pow', 'evmFork': 'Shanghai', 'genesisHeaderVersion': 2},
               'difficulty': '0x1', 'gasLimit': header['gasLimit'], 'baseFeePerGas': header['baseFeePerGas'],
               'timestamp': header['timestamp'], 'nonce': header['nonce'], 'extraData': header['extraData'], 'alloc': case['pre']}
    genesis_bytes = encoded_json(genesis)
    transactions_bytes = encoded_json(['0x' + raw.hex()])
    expectations = {'format': 'nano-main-http-persistence-expectations-v1', 'ready': False, 'nano': None,
                    'inputs': {'genesis': pin(genesis_bytes), 'transactions': pin(transactions_bytes)},
                    'chainId': '0x1', 'evmFork': 'Shanghai', 'consensus': 'pow', 'timestamps': [12, 24],
                    'coinbase': block['blockHeader']['coinbase'], 'sender': tx['sender'], 'target': tx['to'],
                    'txHash': receipt['transactionHash'], 'receipt': {'status': '0x1', 'gasUsed': receipt['cumulativeGasUsed'], 'type': '0x0'},
                    'gasLimit': hex(int(header['gasLimit'], 16)), 'baseFeePerGas': hex(int(header['baseFeePerGas'], 16)),
                    'storageKeys': ['0x0', '0x1'], 'genesisValues': ['0x0', '0x0'], 'postValues': ['0x0', '0x67'],
                    'senderNonceBefore': '0x0', 'senderNonceAfter': '0x1',
                    'senderBalanceBefore': hex(int(case['pre'][tx['sender']]['balance'], 16)),
                    'senderBalanceAfter': hex(int(case['postState'][tx['sender']]['balance'], 16)),
                    'targetNonce': hex(int(case['pre'][tx['to']]['nonce'], 16)),
                    'targetBalance': hex(int(case['pre'][tx['to']]['balance'], 16)), 'targetCode': case['pre'][tx['to']]['code']}
    provenance = {'format': 'nano-main-http-public-fixture-derivation-v1', 'sourcePin': SOURCE_PIN, 'sourceCase': CASE,
                  'signedFieldsCopiedWithoutResigning': True, 'rawRlpEqualsExactSourceBlockTransactionSpan': True,
                  'rawTransactionBytes': pin(raw), 'sourceBlockTransactionSpan': list(tx_spans[0]),
                  'expectedEthereumTransactionHashFromSourceReceipt': receipt['transactionHash'],
                  'keccakComputedDuringPreparation': False, 'ecdsaSignatureOrSenderRecoveryVerifiedDuringPreparation': False,
                  'expectedEthereumTransactionHashVerifiedDuringPreparation': False,
                  'genesisAllocGasAndBaseFeeCopiedFromSource': True, 'sourceConsensus': case['sealEngine'], 'derivedConsensus': 'pow',
                  'intentionalChanges': ['positive difficulty one for Nano PoW', 'Nano Shanghai domain/header codec; no Ethereum block hash claim',
                                         'timestamps 12 and 24 instead of official block timestamp 1000', 'Nano PoW rewards; no complete EEST post-state root assertion'],
                  'sourceGenesisStateRootReferenceOnly': header['stateRoot'], 'sourcePostStateRootReferenceOnly': block['blockHeader']['stateRoot'],
                  'completeEestPostStateRootAsserted': False, 'consensusOrFinalityQualified': False, 'productionQualified': False}
    return {'genesis.json': genesis_bytes, 'transaction-fields.json': encoded_json(tx), 'signed-raw-transactions.json': transactions_bytes,
            'expectations.json': encoded_json(expectations), 'derivation-public.json': encoded_json(provenance), 'serve-config.json': encoded_json({})}

def main():
    if len(sys.argv) != 3:
        raise ValueError('derive-fixture.py PUBLIC_SOURCE_JSON NEW_OUTPUT_DIRECTORY')
    source = Path(sys.argv[1]).read_bytes()
    payloads = derive(source)
    output = Path(sys.argv[2])
    output.mkdir(mode=0o755, parents=False, exist_ok=True)
    if output.resolve(strict=True) != output:
        raise ValueError('physical absolute output directory')
    for name, body in payloads.items():
        with (output / name).open('xb') as handle:
            handle.write(body)
    print(json.dumps({'publicDataOnly': True, 'payloads': {name: pin(body) for name, body in payloads.items()}, 'keccakOrSdkOrNodeInvoked': False}))

if __name__ == '__main__':
    main()
