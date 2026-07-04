#pragma once

#include "isa.hpp"
#include "component.hpp"
#include "decoder.hpp"
#include <iostream>

namespace Core {

class CPU {
private:
    uint32_t pc;               // Program Counter
    RegisterFile reg_file;     // Core Architecture Registers
    Memory& memory;            // Reference to system interconnect memory
    bool halted;               // Flag to gracefully stop the execution loop

public:
    // Initialize CPU pointing to its starting execution vector
    CPU(uint32_t entry_point, Memory& sys_mem) 
        : pc(entry_point), memory(sys_mem), halted(false) {}

    // Getters for verification tracking
    uint32_t get_pc() const { return pc; }
    const RegisterFile& get_regs() const { return reg_file; }
    bool is_halted() const { return halted; }

    // Execute a single clock cycle sequentially
    void step() {
        if (halted) return;

        // 1. FETCH: Fetch 32-bit instruction from memory at current PC
        uint32_t raw_instr = memory.read_word(pc);

        // Treat 0x00000000 or a specific loop catch as a Halted state for tests
        if (raw_instr == 0x00000000) {
            halted = true;
            std::cout << "[CPU] Halted encountered at PC: 0x" << std::hex << pc << std::dec << "\n";
            return;
        }

        // 2. DECODE: Parse raw fields and extract immediates
        Instruction instr = Decoder::decode(raw_instr);
        
        // Track modern PC mutations
        uint32_t next_pc = pc + 4; 

        // 3. EXECUTE / MEM / WRITEBACK (Sequential Evaluation)
        switch (instr.opcode) {
            case Opcode::R_TYPE: {
                uint32_t val1 = reg_file.read(instr.rs1);
                uint32_t val2 = reg_file.read(instr.rs2);
                uint32_t result = 0;

                if (instr.funct3 == Funct3::ADD_SUB) {
                    if (instr.funct7 == Funct7::ALT) {
                        result = val1 - val2; // SUB
                    } else {
                        result = val1 + val2; // ADD
                    }
                } else if (instr.funct3 == Funct3::AND) {
                    result = val1 & val2;     // AND
                } else if (instr.funct3 == Funct3::OR) {
                    result = val1 | val2;     // OR
                } else if (instr.funct3 == Funct3::XOR) {
                    result = val1 ^ val2;     // XOR
                }
                reg_file.write(instr.rd, result);
                break;
            }

            case Opcode::I_TYPE_IMM: {
                uint32_t val1 = reg_file.read(instr.rs1);
                uint32_t result = 0;

                if (instr.funct3 == Funct3::ADD_SUB) {
                    result = val1 + instr.imm; // ADDI
                } else if (instr.funct3 == Funct3::AND) {
                    result = val1 & instr.imm; // ANDI
                } else if (instr.funct3 == Funct3::OR) {
                    result = val1 | instr.imm; // ORI
                } else if (instr.funct3 == Funct3::XOR) {
                    result = val1 ^ instr.imm; // XORI
                }
                reg_file.write(instr.rd, result);
                break;
            }

            case Opcode::I_TYPE_LOAD: {
                if (instr.funct3 == Funct3::LW) {
                    uint32_t target_addr = reg_file.read(instr.rs1) + instr.imm;
                    uint32_t loaded_val = memory.read_word(target_addr);
                    reg_file.write(instr.rd, loaded_val);
                }
                break;
            }

            case Opcode::S_TYPE_STORE: {
                if (instr.funct3 == Funct3::SW) {
                    uint32_t target_addr = reg_file.read(instr.rs1) + instr.imm;
                    uint32_t value_to_store = reg_file.read(instr.rs2);
                    memory.write_word(target_addr, value_to_store);
                }
                break;
            }

            case Opcode::B_TYPE_BRANCH: {
                if (instr.funct3 == Funct3::BEQ) {
                    if (reg_file.read(instr.rs1) == reg_file.read(instr.rs2)) {
                        next_pc = pc + instr.imm; // Branch taken
                    }
                }
                break;
            }

            case Opcode::J_TYPE_JAL: {
                reg_file.write(instr.rd, pc + 4); // Store link register address
                next_pc = pc + instr.imm;         // Jump target offset
                break;
            }

            default:
                std::cerr << "[CPU ERROR] Unknown Opcode executed: 0x" 
                          << std::hex << static_cast<uint32_t>(instr.opcode) << std::dec << "\n";
                halted = true;
                return;
        }

        // Commit PC update to complete sequential cycle step
        pc = next_pc;
    }
};

} // namespace Core