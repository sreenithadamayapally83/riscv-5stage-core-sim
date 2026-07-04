`ifndef RISCV_DEFS_V
`define RISCV_DEFS_V

// Architectural Widths
`define XLEN             32
`define REG_ADDR_WIDTH   5

// Opcodes (Matching our C++ ISA definitions)
`define OP_R_TYPE        7'b0110011
`define OP_I_TYPE_IMM    7'b0010011
`define OP_I_TYPE_LOAD   7'b0000011
`define OP_S_TYPE_STORE  7'b0100011
`define OP_B_TYPE_BRANCH 7'b1100011

// Funct3 Codes
`define F3_ADD_SUB       3'b000
`define F3_AND           3'b111
`define F3_OR            3'b110
`define F3_XOR           3'b100
`define F3_LW            3'b010
`define F3_SW            3'b010
`define F3_BEQ           3'b000

// Funct7 Codes
`define F7_STANDARD      7'b0000000
`define F7_ALT           7'b0100000

`endif // RISCV_DEFS_V