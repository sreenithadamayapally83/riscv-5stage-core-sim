#pragma once

#include "isa.hpp"
#include "component.hpp"
#include "decoder.hpp"
#include "pipeline_regs.hpp"
#include "predictor.hpp"
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>

namespace Core {

class PipelinedCPU {
private:
    uint32_t pc;
    RegisterFile reg_file;
    Memory& memory;
    bool halted;

    // Pipeline Registers
    Reg_IF_ID  reg_if_id;
    Reg_ID_EX  reg_id_ex;
    Reg_EX_MEM reg_ex_mem;
    Reg_MEM_WB reg_mem_wb;

    // Control & Predictor Structures
    BranchPredictor predictor;
    bool stall_pipeline = false;

    // Stress Testing Metric Tracking Registers
    uint32_t total_cycles         = 0;
    uint32_t instructions_retired  = 0;
    uint32_t forward_a_events      = 0;
    uint32_t forward_b_events      = 0;
    uint32_t load_use_stalls       = 0;
    uint32_t branch_flushes        = 0;

    // Direct String Captures for Real-Time Pipeline Trace Tables
    std::string stage_if_str  = "BUBBLE";
    std::string stage_id_str  = "BUBBLE";
    std::string stage_ex_str  = "BUBBLE";
    std::string stage_mem_str = "BUBBLE";
    std::string stage_wb_str  = "BUBBLE";

    std::string get_mnemonic(Opcode op, uint32_t raw) {
        if (raw == 0) return "NOP";
        switch(op) {
            case Opcode::R_TYPE: {
                uint32_t f3 = (raw >> 12) & 0x7;
                uint32_t f7 = (raw >> 25) & 0x7F;
                if (f3 == 0x0) return (f7 == 0x20) ? "SUB" : "ADD";
                if (f3 == 0x7) return "AND";
                if (f3 == 0x6) return "OR";
                if (f3 == 0x4) return "XOR";
                return "R-OP";
            }
            case Opcode::I_TYPE_IMM:  return "ADDI";
            case Opcode::I_TYPE_LOAD: return "LW";
            case Opcode::S_TYPE_STORE:return "SW";
            case Opcode::B_TYPE_BRANCH:return "BEQ";
            default:                  return "UNKNOWN";
        }
    }

public:
    PipelinedCPU(uint32_t entry_point, Memory& sys_mem)
        : pc(entry_point), memory(sys_mem), halted(false) {}

    uint32_t get_pc() const { return pc; }
    const RegisterFile& get_regs() const { return reg_file; }
    bool is_halted() const { return halted; }

    void clock_cycle() {
        if (halted) return;

        total_cycles++;

        // Process stages backwards to emulate concurrent register updates
        stage_writeback();
        stage_memory();
        stage_execute();
        stage_decode();
        stage_fetch();

        // Print Pipeline Visualization Row for current Cycle
        std::cout << "Cycle " << std::setw(2) << std::left << total_cycles << " | "
                  << "IF: "  << std::setw(6) << std::left << stage_if_str  << " | "
                  << "ID: "  << std::setw(6) << std::left << stage_id_str  << " | "
                  << "EX: "  << std::setw(6) << std::left << stage_ex_str  << " | "
                  << "MEM: " << std::setw(6) << std::left << stage_mem_str << " | "
                  << "WB: "  << std::setw(6) << std::left << stage_wb_str  << "\n";

        if (!reg_if_id.valid && !reg_id_ex.valid && !reg_ex_mem.valid && !reg_mem_wb.valid) {
            halted = true;
            std::cout << "\n[Pipelined CPU] Pipeline drained. Core Halted.\n";
        }
    }

    void print_performance_metrics() const {
        double cpi = (instructions_retired > 0) ? static_cast<double>(total_cycles) / instructions_retired : 0.0;
        std::cout << "               HAZARD & PERFORMANCE ANALYSIS REPORT             \n";
        std::cout << std::left << std::setw(40) << "Total Execution Cycles:" << total_cycles << "\n";
        std::cout << std::left << std::setw(40) << "Instructions Fully Retired:" << instructions_retired << "\n";
        std::cout << std::left << std::setw(40) << "Calculated Pipeline CPI:" << std::fixed << std::setprecision(4) << cpi << "\n";
        std::cout << std::left << std::setw(40) << "Forwarding Unit Activations (rs1):" << forward_a_events << "\n";
        std::cout << std::left << std::setw(40) << "Forwarding Unit Activations (rs2):" << forward_b_events << "\n";
        std::cout << std::left << std::setw(40) << "Load-Use Hazard Stalls Inserted:" << load_use_stalls << "\n";
        std::cout << std::left << std::setw(40) << "Control Hazard Pipeline Flushes:" << branch_flushes << "\n";
        
        memory.print_cache_metrics();
    }

private:
    void stage_writeback() {
        if (!reg_mem_wb.valid) {
            stage_wb_str = "BUBBLE";
            return;
        }
        stage_wb_str = get_mnemonic(reg_mem_wb.ins.opcode, reg_mem_wb.ins.raw);

        Instruction ins = reg_mem_wb.ins;
        bool retired = false;

        if (ins.opcode == Opcode::R_TYPE || ins.opcode == Opcode::I_TYPE_IMM || ins.opcode == Opcode::I_TYPE_LOAD) {
            uint32_t data = (ins.opcode == Opcode::I_TYPE_LOAD) ? reg_mem_wb.read_data : reg_mem_wb.alu_result;
            reg_file.write(ins.rd, data);
            retired = true;
        }

        if (retired) instructions_retired++;
        reg_mem_wb.valid = false;
    }

    void stage_memory() {
        if (!reg_ex_mem.valid) {
            stage_mem_str = "BUBBLE";
            return;
        }
        stage_mem_str = get_mnemonic(reg_ex_mem.ins.opcode, reg_ex_mem.ins.raw);

        reg_mem_wb.pc = reg_ex_mem.pc;
        reg_mem_wb.ins = reg_ex_mem.ins;
        reg_mem_wb.alu_result = reg_ex_mem.alu_result;
        reg_mem_wb.valid = true;

        Instruction ins = reg_ex_mem.ins;
        if (ins.opcode == Opcode::I_TYPE_LOAD) {
            reg_mem_wb.read_data = memory.read_word(reg_ex_mem.alu_result);
        } else if (ins.opcode == Opcode::S_TYPE_STORE) {
            memory.write_word(reg_ex_mem.alu_result, reg_ex_mem.val_rs2);
            reg_mem_wb.valid = false; 
            instructions_retired++; 
        }
        reg_ex_mem.valid = false;
    }

    void stage_execute() {
        if (!reg_id_ex.valid) {
            stage_ex_str = "BUBBLE";
            return;
        }
        stage_ex_str = get_mnemonic(reg_id_ex.ins.opcode, reg_id_ex.ins.raw);

        uint32_t op1 = reg_id_ex.val_rs1;
        uint32_t op2_forwarded = reg_id_ex.val_rs2;
        Instruction ins = reg_id_ex.ins;

        // RAW Hazard Forwarding Bypass Evaluation
        if (reg_mem_wb.valid && reg_mem_wb.ins.rd != 0) {
            uint32_t f_val = (reg_mem_wb.ins.opcode == Opcode::I_TYPE_LOAD) ? reg_mem_wb.read_data : reg_mem_wb.alu_result;
            if (reg_mem_wb.ins.rd == ins.rs1) { op1 = f_val; forward_a_events++; }
            if (reg_mem_wb.ins.rd == ins.rs2) { op2_forwarded = f_val; forward_b_events++; }
        }
        if (reg_ex_mem.valid && reg_ex_mem.ins.rd != 0) {
            uint32_t f_val = reg_ex_mem.alu_result;
            if (reg_ex_mem.ins.rd == ins.rs1) { op1 = f_val; forward_a_events++; }
            if (reg_ex_mem.ins.rd == ins.rs2) { op2_forwarded = f_val; forward_b_events++; }
        }

        uint32_t op2 = (ins.opcode == Opcode::R_TYPE || ins.opcode == Opcode::B_TYPE_BRANCH) ? op2_forwarded : ins.imm;

        // Control Branch Validation
        if (ins.opcode == Opcode::B_TYPE_BRANCH) {
            bool branch_taken = (ins.funct3 == Funct3::BEQ) && (op1 == op2_forwarded);
            bool predicted_taken = predictor.predict(reg_id_ex.pc);

            if (branch_taken != predicted_taken) {
                branch_flushes++;
                reg_if_id.valid = false; 
                reg_id_ex.valid = false;
                pc = branch_taken ? (reg_id_ex.pc + ins.imm) : (reg_id_ex.pc + 4);
                predictor.update(reg_id_ex.pc, branch_taken);
                
                stage_ex_str += "[FLUSH]";
                return;
            }
            predictor.update(reg_id_ex.pc, branch_taken);
            instructions_retired++;
        }

        reg_ex_mem.pc = reg_id_ex.pc;
        reg_ex_mem.ins = ins;
        reg_ex_mem.val_rs2 = op2_forwarded; 
        reg_ex_mem.valid = true;

        if (ins.opcode == Opcode::R_TYPE) {
            if (ins.funct3 == Funct3::ADD_SUB) {
                reg_ex_mem.alu_result = (ins.funct7 == Funct7::ALT) ? (op1 - op2) : (op1 + op2);
            } else if (ins.funct3 == Funct3::AND) reg_ex_mem.alu_result = op1 & op2;
            else if (ins.funct3 == Funct3::OR)  reg_ex_mem.alu_result = op1 | op2;
            else if (ins.funct3 == Funct3::XOR) reg_ex_mem.alu_result = op1 ^ op2;
        } else if (ins.opcode == Opcode::I_TYPE_IMM || ins.opcode == Opcode::I_TYPE_LOAD || ins.opcode == Opcode::S_TYPE_STORE) {
            reg_ex_mem.alu_result = op1 + op2;
        }
        reg_id_ex.valid = false;
    }

    void stage_decode() {
        if (stall_pipeline) {
            stage_id_str = "STALL";
            stall_pipeline = false; 
            return;
        }

        if (!reg_if_id.valid) {
            stage_id_str = "BUBBLE";
            return;
        }
        
        uint32_t next_raw = reg_if_id.raw_instr;
        uint32_t next_rs1 = (next_raw >> 15) & 0x1F;
        uint32_t next_rs2 = (next_raw >> 20) & 0x1F;

        // Load-Use Hazard Interlock Check
        if (reg_id_ex.valid && reg_id_ex.ins.opcode == Opcode::I_TYPE_LOAD) {
            if (reg_id_ex.ins.rd == next_rs1 || reg_id_ex.ins.rd == next_rs2) {
                stall_pipeline = true;
                load_use_stalls++;
                reg_id_ex.valid = false; 
                stage_id_str = "STALL";
                return;
            }
        }

        Instruction decoded_ins = Decoder::decode(next_raw);
        stage_id_str = get_mnemonic(decoded_ins.opcode, next_raw);

        uint32_t wb_rd = 0, wb_data = 0;
        if (reg_mem_wb.valid && reg_mem_wb.ins.rd != 0) {
            wb_rd = reg_mem_wb.ins.rd;
            wb_data = (reg_mem_wb.ins.opcode == Opcode::I_TYPE_LOAD) ? reg_mem_wb.read_data : reg_mem_wb.alu_result;
        }

        reg_id_ex.pc = reg_if_id.pc;
        reg_id_ex.ins = decoded_ins;
        reg_id_ex.val_rs1 = reg_file.read_forwarded(decoded_ins.rs1, wb_rd, wb_data);
        reg_id_ex.val_rs2 = reg_file.read_forwarded(decoded_ins.rs2, wb_rd, wb_data);
        reg_id_ex.valid = true;
        reg_if_id.valid = false;
    }

    void stage_fetch() {
        if (stall_pipeline) {
            stage_if_str = "STALL";
            return;
        }

        uint32_t raw_instr = memory.read_word(pc);
        if (raw_instr == 0x00000000) {
            stage_if_str = "NOP";
            return;
        }

        Instruction quick_peek = Decoder::decode(raw_instr);
        stage_if_str = get_mnemonic(quick_peek.opcode, raw_instr);

        reg_if_id.pc = pc;
        reg_if_id.raw_instr = raw_instr;
        reg_if_id.valid = true;

        if (predictor.predict(pc) && quick_peek.opcode == Opcode::B_TYPE_BRANCH) {
            pc += quick_peek.imm;
        } else {
            pc += 4;
        }
    }
};

} // namespace Core