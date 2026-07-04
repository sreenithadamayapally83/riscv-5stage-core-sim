#pragma once

#include "isa.hpp"
#include <iostream>

namespace Core {

class Decoder {
public:
    static Instruction decode(uint32_t raw_instr) {
        Instruction instr;
        instr.raw = raw_instr;
        
        // Extract Opcode (Bits [6:0])
        uint32_t op_bits = extract_bits(raw_instr, 0, 6);
        instr.opcode = static_cast<Opcode>(op_bits);

        // Extract base structural register fields common across multiple formats
        instr.rd     = extract_bits(raw_instr, 7, 11);
        instr.funct3 = extract_bits(raw_instr, 12, 14);
        instr.rs1    = extract_bits(raw_instr, 15, 19);
        instr.rs2    = extract_bits(raw_instr, 20, 24);
        instr.funct7 = extract_bits(raw_instr, 25, 31);

        // Decode Immediate values based on the specific instruction type
        switch (instr.opcode) {
            case Opcode::R_TYPE:
                // R-type instructions have no immediate values
                instr.imm = 0;
                break;

            case Opcode::I_TYPE_IMM:
            case Opcode::I_TYPE_LOAD: {
                // I-type: 12-bit immediate [31:20]
                uint32_t imm_11_0 = extract_bits(raw_instr, 20, 31);
                // Perform arithmetic manual sign-extension to 32 bits
                instr.imm = static_cast<int32_t>(imm_11_0 << 20) >> 20;
                break;
            }

            case Opcode::S_TYPE_STORE: {
                // S-type: Split 12-bit immediate [31:25] as imm[11:5] and [11:7] as imm[4:0]
                uint32_t imm_4_0  = extract_bits(raw_instr, 7, 11);
                uint32_t imm_11_5 = extract_bits(raw_instr, 25, 31);
                uint32_t total_imm = imm_4_0 | (imm_11_5 << 5);
                // Sign-extend from 12 bits to 32 bits
                instr.imm = static_cast<int32_t>(total_imm << 20) >> 20;
                break;
            }

            case Opcode::B_TYPE_BRANCH: {
                // B-type: Split 12-bit immediate encoding branch offsets (multiples of 2 bytes)
                uint32_t imm_11   = extract_bits(raw_instr, 7, 7);
                uint32_t imm_4_1  = extract_bits(raw_instr, 8, 11);
                uint32_t imm_10_5 = extract_bits(raw_instr, 25, 30);
                uint32_t imm_12   = extract_bits(raw_instr, 31, 31);
                
                uint32_t total_imm = (imm_4_1 << 1)   | 
                                     (imm_10_5 << 5)  | 
                                     (imm_11 << 11)   | 
                                     (imm_12 << 12);
                // Sign-extend from 13 bits to 32 bits
                instr.imm = static_cast<int32_t>(total_imm << 19) >> 19;
                break;
            }

            case Opcode::J_TYPE_JAL: {
                // J-type: Scrambled 20-bit immediate encoding target jump offsets
                uint32_t imm_19_12 = extract_bits(raw_instr, 12, 19);
                uint32_t imm_11    = extract_bits(raw_instr, 20, 20);
                uint32_t imm_10_1  = extract_bits(raw_instr, 21, 30);
                uint32_t imm_20    = extract_bits(raw_instr, 31, 31);

                uint32_t total_imm = (imm_10_1 << 1)   | 
                                     (imm_11 << 11)    | 
                                     (imm_19_12 << 12) | 
                                     (imm_20 << 20);
                // Sign-extend from 21 bits to 32 bits
                instr.imm = static_cast<int32_t>(total_imm << 11) >> 11;
                break;
            }

            default:
                // If we encounter an unexpected opcode pattern, default to zero
                instr.imm = 0;
                break;
        }

        return instr;
    }
};

} // namespace Core