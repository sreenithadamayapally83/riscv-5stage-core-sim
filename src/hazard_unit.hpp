#pragma once

#include "isa.hpp"
#include "pipeline_regs.hpp"

namespace Core {

class HazardUnit {
public:
    static void compute_forwarding(const Reg_ID_EX& id_ex, 
                                   const Reg_EX_MEM& ex_mem, 
                                   const Reg_MEM_WB& mem_wb,
                                   uint32_t& forwarded_val1, 
                                   uint32_t& forwarded_val2) {
        
        // Start with default values from register file read stage
        forwarded_val1 = id_ex.val_rs1;
        forwarded_val2 = id_ex.val_rs2;

        if (!id_ex.valid) return;

        // FORWARDING TO OPERAND 1 (rs1)
        // Check MEM/WB Stage first (older instruction dependency)
        if (mem_wb.valid && mem_wb.ins.rd != 0 && mem_wb.ins.rd == id_ex.ins.rs1) {
            if (mem_wb.ins.opcode == Opcode::R_TYPE || mem_wb.ins.opcode == Opcode::I_TYPE_IMM) {
                forwarded_val1 = mem_wb.alu_result;
            } else if (mem_wb.ins.opcode == Opcode::I_TYPE_LOAD) {
                forwarded_val1 = mem_wb.read_data;
            }
        }
        // Check EX/MEM Stage second (newer instruction dependency overrides older)
        if (ex_mem.valid && ex_mem.ins.rd != 0 && ex_mem.ins.rd == id_ex.ins.rs1) {
            if (ex_mem.ins.opcode == Opcode::R_TYPE || ex_mem.ins.opcode == Opcode::I_TYPE_IMM) {
                forwarded_val1 = ex_mem.alu_result;
            }
        }

        // FORWARDING TO OPERAND 2 (rs2)
        // Check MEM/WB Stage first
        if (mem_wb.valid && mem_wb.ins.rd != 0 && mem_wb.ins.rd == id_ex.ins.rs2) {
            if (mem_wb.ins.opcode == Opcode::R_TYPE || mem_wb.ins.opcode == Opcode::I_TYPE_IMM) {
                forwarded_val2 = mem_wb.alu_result;
            } else if (mem_wb.ins.opcode == Opcode::I_TYPE_LOAD) {
                forwarded_val2 = mem_wb.read_data;
            }
        }
        // Check EX/MEM Stage second
        if (ex_mem.valid && ex_mem.ins.rd != 0 && ex_mem.ins.rd == id_ex.ins.rs2) {
            if (ex_mem.ins.opcode == Opcode::R_TYPE || ex_mem.ins.opcode == Opcode::I_TYPE_IMM) {
                forwarded_val2 = ex_mem.alu_result;
            }
        }
    }

    static bool check_load_stall(const Reg_ID_EX& id_ex, uint32_t next_stage_rs1, uint32_t next_stage_rs2) {
        if (id_ex.valid && id_ex.ins.opcode == Opcode::I_TYPE_LOAD) {
            if (id_ex.ins.rd == next_stage_rs1 || id_ex.ins.rd == next_stage_rs2) {
                return true;
            }
        }
        return false;
    }
};

} // namespace Core