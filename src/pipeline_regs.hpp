#pragma once

#include "isa.hpp"

namespace Core {

// IF/ID Pipeline Register
struct Reg_IF_ID {
    uint32_t pc        = 0;
    uint32_t raw_instr = 0;
    bool valid         = false; // Latches false on stalls/bubbles
};

// ID/EX Pipeline Register
struct Reg_ID_EX {
    uint32_t pc     = 0;
    Instruction ins;         // The parsed decoded instruction structure
    uint32_t val_rs1 = 0;    // Value read from Register File for rs1
    uint32_t val_rs2 = 0;    // Value read from Register File for rs2
    bool valid      = false;
};

// EX/MEM Pipeline Register
struct Reg_EX_MEM {
    uint32_t pc         = 0;
    Instruction ins;
    uint32_t alu_result = 0; // Evaluated math/address output
    uint32_t val_rs2    = 0; // Kept for store instructions (data to write to memory)
    bool valid          = false;
};

// MEM/WB Pipeline Register
struct Reg_MEM_WB {
    uint32_t pc         = 0;
    Instruction ins;
    uint32_t alu_result = 0; // Captured arithmetic result
    uint32_t read_data  = 0; // Captured memory load data
    bool valid          = false;
};

} // namespace Core