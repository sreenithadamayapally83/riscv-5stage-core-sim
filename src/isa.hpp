#pragma once

#include <cstdint>

namespace Core {

// Core Opcodes (Bits [6:0] of the 32-bit instruction)
enum class Opcode : uint32_t {
    R_TYPE        = 0x33, // Register-Register Arithmetic/Logic
    I_TYPE_IMM    = 0x13, // Register-Immediate Arithmetic
    I_TYPE_LOAD   = 0x03, // Memory Load instructions
    S_TYPE_STORE  = 0x23, // Memory Store instructions
    B_TYPE_BRANCH = 0x63, // Conditional Branches
    J_TYPE_JAL    = 0x6F  // Unconditional Jump and Link
};

// Funct3 codes to differentiate operations within the same Opcode category
namespace Funct3 {
    // R-Type & I-Type Immediate
    constexpr uint32_t ADD_SUB = 0x0; // Differentiated by Funct7
    constexpr uint32_t XOR     = 0x4;
    constexpr uint32_t OR      = 0x6;
    constexpr uint32_t AND     = 0x7;
    
    // Load/Store sizes
    constexpr uint32_t LW      = 0x2; // Load Word (32-bit)
    constexpr uint32_t SW      = 0x2; // Store Word (32-bit)

    // Branch conditions
    constexpr uint32_t BEQ     = 0x0; // Branch if Equal
}

// Funct7 codes for R-Type instructions
namespace Funct7 {
    constexpr uint32_t BASE    = 0x00; // e.g., ADD
    constexpr uint32_t ALT     = 0x20; // e.g., SUB
}

// This structure holds all extracted fields of a single instruction after decoding
struct Instruction {
    uint32_t raw    = 0;  // The original unparsed 32-bit binary
    Opcode opcode;        // Extracted opcode
    uint32_t rd     = 0;  // Destination register index (0 to 31)
    uint32_t rs1    = 0;  // Source register 1 index (0 to 31)
    uint32_t rs2    = 0;  // Source register 2 index (0 to 31)
    uint32_t funct3 = 0;  // 3-bit function field
    uint32_t funct7 = 0;  // 7-bit function field
    int32_t  imm    = 0;  // Sign-extended immediate value
};

// Clean inline helper for bit slicing: extracts bits from 'low' to 'high' (inclusive)
inline uint32_t extract_bits(uint32_t val, uint8_t low, uint8_t high) {
    return (val >> low) & ((1U << (high - low + 1)) - 1);
}

} // namespace Core