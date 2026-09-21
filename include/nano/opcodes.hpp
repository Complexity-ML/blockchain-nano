#pragma once
#include <cstdint>
namespace nano {
// Names and byte values imported from evm.codes and its EthereumJS opcode table.
// evm.codes commit: 224365a22b948c0465e44f08e8496cdf41945d48
// EthereumJS EVM: 10.1.0. See third_party/opcodes/ for sources and licenses.
// This enum also declares EOF/proposed instructions; declarations do not enable them.
enum class NanoOpCodes : uint8_t {
    STOP = 0x00,
    ADD = 0x01,
    MUL = 0x02,
    SUB = 0x03,
    DIV = 0x04,
    SDIV = 0x05,
    MOD = 0x06,
    SMOD = 0x07,
    ADDMOD = 0x08,
    MULMOD = 0x09,
    EXP = 0x0A,
    SIGNEXTEND = 0x0B,
    LT = 0x10,
    GT = 0x11,
    SLT = 0x12,
    SGT = 0x13,
    EQ = 0x14,
    ISZERO = 0x15,
    AND = 0x16,
    OR = 0x17,
    XOR = 0x18,
    NOT = 0x19,
    BYTE = 0x1A,
    SHL = 0x1B,
    SHR = 0x1C,
    SAR = 0x1D,
    CLZ = 0x1E,
    KECCAK256 = 0x20,
    ADDRESS = 0x30,
    BALANCE = 0x31,
    ORIGIN = 0x32,
    CALLER = 0x33,
    CALLVALUE = 0x34,
    CALLDATALOAD = 0x35,
    CALLDATASIZE = 0x36,
    CALLDATACOPY = 0x37,
    CODESIZE = 0x38,
    CODECOPY = 0x39,
    GASPRICE = 0x3A,
    EXTCODESIZE = 0x3B,
    EXTCODECOPY = 0x3C,
    RETURNDATASIZE = 0x3D,
    RETURNDATACOPY = 0x3E,
    EXTCODEHASH = 0x3F,
    BLOCKHASH = 0x40,
    COINBASE = 0x41,
    TIMESTAMP = 0x42,
    NUMBER = 0x43,
    PREVRANDAO = 0x44,
    GASLIMIT = 0x45,
    CHAINID = 0x46,
    SELFBALANCE = 0x47,
    BASEFEE = 0x48,
    BLOBHASH = 0x49,
    BLOBBASEFEE = 0x4A,
    POP = 0x50,
    MLOAD = 0x51,
    MSTORE = 0x52,
    MSTORE8 = 0x53,
    SLOAD = 0x54,
    SSTORE = 0x55,
    JUMP = 0x56,
    JUMPI = 0x57,
    PC = 0x58,
    MSIZE = 0x59,
    GAS = 0x5A,
    JUMPDEST = 0x5B,
    TLOAD = 0x5C,
    TSTORE = 0x5D,
    MCOPY = 0x5E,
    PUSH0 = 0x5F,
    PUSH1 = 0x60,
    PUSH2 = 0x61,
    PUSH3 = 0x62,
    PUSH4 = 0x63,
    PUSH5 = 0x64,
    PUSH6 = 0x65,
    PUSH7 = 0x66,
    PUSH8 = 0x67,
    PUSH9 = 0x68,
    PUSH10 = 0x69,
    PUSH11 = 0x6A,
    PUSH12 = 0x6B,
    PUSH13 = 0x6C,
    PUSH14 = 0x6D,
    PUSH15 = 0x6E,
    PUSH16 = 0x6F,
    PUSH17 = 0x70,
    PUSH18 = 0x71,
    PUSH19 = 0x72,
    PUSH20 = 0x73,
    PUSH21 = 0x74,
    PUSH22 = 0x75,
    PUSH23 = 0x76,
    PUSH24 = 0x77,
    PUSH25 = 0x78,
    PUSH26 = 0x79,
    PUSH27 = 0x7A,
    PUSH28 = 0x7B,
    PUSH29 = 0x7C,
    PUSH30 = 0x7D,
    PUSH31 = 0x7E,
    PUSH32 = 0x7F,
    DUP1 = 0x80,
    DUP2 = 0x81,
    DUP3 = 0x82,
    DUP4 = 0x83,
    DUP5 = 0x84,
    DUP6 = 0x85,
    DUP7 = 0x86,
    DUP8 = 0x87,
    DUP9 = 0x88,
    DUP10 = 0x89,
    DUP11 = 0x8A,
    DUP12 = 0x8B,
    DUP13 = 0x8C,
    DUP14 = 0x8D,
    DUP15 = 0x8E,
    DUP16 = 0x8F,
    SWAP1 = 0x90,
    SWAP2 = 0x91,
    SWAP3 = 0x92,
    SWAP4 = 0x93,
    SWAP5 = 0x94,
    SWAP6 = 0x95,
    SWAP7 = 0x96,
    SWAP8 = 0x97,
    SWAP9 = 0x98,
    SWAP10 = 0x99,
    SWAP11 = 0x9A,
    SWAP12 = 0x9B,
    SWAP13 = 0x9C,
    SWAP14 = 0x9D,
    SWAP15 = 0x9E,
    SWAP16 = 0x9F,
    LOG0 = 0xA0,
    LOG1 = 0xA1,
    LOG2 = 0xA2,
    LOG3 = 0xA3,
    LOG4 = 0xA4,
    DATALOAD = 0xD0, // EOF; requires a compatible EOF revision.
    DATALOADN = 0xD1, // EOF; requires a compatible EOF revision.
    DATASIZE = 0xD2, // EOF; requires a compatible EOF revision.
    DATACOPY = 0xD3, // EOF; requires a compatible EOF revision.
    RJUMP = 0xE0, // EOF; requires a compatible EOF revision.
    RJUMPI = 0xE1, // EOF; requires a compatible EOF revision.
    RJUMPV = 0xE2, // EOF; requires a compatible EOF revision.
    CALLF = 0xE3, // EOF; requires a compatible EOF revision.
    RETF = 0xE4, // EOF; requires a compatible EOF revision.
    JUMPF = 0xE5, // EOF; requires a compatible EOF revision.
    DUPN = 0xE6, // EOF; requires a compatible EOF revision.
    SWAPN = 0xE7, // EOF; requires a compatible EOF revision.
    EXCHANGE = 0xE8, // EOF; requires a compatible EOF revision.
    EOFCREATE = 0xEC, // EOF; requires a compatible EOF revision.
    RETURNCONTRACT = 0xEE, // EOF; requires a compatible EOF revision.
    CREATE = 0xF0,
    CALL = 0xF1,
    CALLCODE = 0xF2,
    RETURN = 0xF3,
    DELEGATECALL = 0xF4,
    CREATE2 = 0xF5,
    RETURNDATALOAD = 0xF7, // EOF; requires a compatible EOF revision.
    EXTCALL = 0xF8, // EOF; requires a compatible EOF revision.
    EXTDELEGATECALL = 0xF9, // EOF; requires a compatible EOF revision.
    STATICCALL = 0xFA,
    EXTSTATICCALL = 0xFB, // EOF; requires a compatible EOF revision.
    REVERT = 0xFD,
    INVALID = 0xFE,
    SELFDESTRUCT = 0xFF,
    // Historical spellings for the same byte values.
    SHA3 = KECCAK256,
    DIFFICULTY = PREVRANDAO,
};
inline constexpr unsigned opcode_count = 169;
// NanoPy historical gas schedule retained during migration, independently of enum coverage.
inline unsigned opcode_gas(uint8_t op) {
    switch(op) {
    case 0x00: return 0;
    case 0xf3: return 0;
    case 0xfd: return 0;
    case 0x30: return 2;
    case 0x32: return 2;
    case 0x33: return 2;
    case 0x34: return 2;
    case 0x36: return 2;
    case 0x38: return 2;
    case 0x3a: return 2;
    case 0x41: return 2;
    case 0x42: return 2;
    case 0x43: return 2;
    case 0x44: return 2;
    case 0x45: return 2;
    case 0x46: return 2;
    case 0x48: return 2;
    case 0x50: return 2;
    case 0x58: return 2;
    case 0x59: return 2;
    case 0x3d: return 2;
    case 0x01: return 3;
    case 0x03: return 3;
    case 0x02: return 5;
    case 0x04: return 5;
    case 0x05: return 5;
    case 0x06: return 5;
    case 0x07: return 5;
    case 0x08: return 8;
    case 0x09: return 8;
    case 0x0b: return 5;
    case 0x10: return 3;
    case 0x11: return 3;
    case 0x12: return 3;
    case 0x13: return 3;
    case 0x14: return 3;
    case 0x15: return 3;
    case 0x16: return 3;
    case 0x17: return 3;
    case 0x18: return 3;
    case 0x19: return 3;
    case 0x1a: return 3;
    case 0x1b: return 3;
    case 0x1c: return 3;
    case 0x1d: return 3;
    case 0x35: return 3;
    case 0x51: return 3;
    case 0x52: return 3;
    case 0x53: return 3;
    case 0x5f: return 2;
    case 0x0a: return 10;
    case 0x56: return 8;
    case 0x57: return 10;
    case 0x5b: return 1;
    case 0x5a: return 2;
    case 0x47: return 5;
    case 0x20: return 30;
    case 0x54: return 100; // Cold surcharge is charged by the EVM.
    case 0x55: return 0; // Entire SSTORE cost is state-dependent.
    case 0x31: return 100; // Cold surcharge is charged by the EVM.
    case 0x3b: return 100; // Cold surcharge is charged by the EVM.
    case 0x3c: return 100; // Cold surcharge is charged by the EVM.
    case 0x3f: return 100; // Cold surcharge is charged by the EVM.
    case 0x40: return 20;
    case 0x37: return 3;
    case 0x39: return 3;
    case 0x3e: return 3;
    case 0xf1: return 100;
    case 0xf2: return 100;
    case 0xf4: return 100;
    case 0xfa: return 100;
    case 0xf0: return 32000;
    case 0xf5: return 32000;
    case 0xa0: return 375;
    case 0xa1: return 750;
    case 0xa2: return 1125;
    case 0xa3: return 1500;
    case 0xa4: return 1875;
    case 0xff: return 5000;
    case 0xfe: return 0;
    case 0x60: return 3;
    case 0x61: return 3;
    case 0x62: return 3;
    case 0x63: return 3;
    case 0x64: return 3;
    case 0x65: return 3;
    case 0x66: return 3;
    case 0x67: return 3;
    case 0x68: return 3;
    case 0x69: return 3;
    case 0x6a: return 3;
    case 0x6b: return 3;
    case 0x6c: return 3;
    case 0x6d: return 3;
    case 0x6e: return 3;
    case 0x6f: return 3;
    case 0x70: return 3;
    case 0x71: return 3;
    case 0x72: return 3;
    case 0x73: return 3;
    case 0x74: return 3;
    case 0x75: return 3;
    case 0x76: return 3;
    case 0x77: return 3;
    case 0x78: return 3;
    case 0x79: return 3;
    case 0x7a: return 3;
    case 0x7b: return 3;
    case 0x7c: return 3;
    case 0x7d: return 3;
    case 0x7e: return 3;
    case 0x7f: return 3;
    case 0x80: return 3;
    case 0x81: return 3;
    case 0x82: return 3;
    case 0x83: return 3;
    case 0x84: return 3;
    case 0x85: return 3;
    case 0x86: return 3;
    case 0x87: return 3;
    case 0x88: return 3;
    case 0x89: return 3;
    case 0x8a: return 3;
    case 0x8b: return 3;
    case 0x8c: return 3;
    case 0x8d: return 3;
    case 0x8e: return 3;
    case 0x8f: return 3;
    case 0x90: return 3;
    case 0x91: return 3;
    case 0x92: return 3;
    case 0x93: return 3;
    case 0x94: return 3;
    case 0x95: return 3;
    case 0x96: return 3;
    case 0x97: return 3;
    case 0x98: return 3;
    case 0x99: return 3;
    case 0x9a: return 3;
    case 0x9b: return 3;
    case 0x9c: return 3;
    case 0x9d: return 3;
    case 0x9e: return 3;
    case 0x9f: return 3;
    default: return 0;
    }
}
}

