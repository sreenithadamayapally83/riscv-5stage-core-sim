`include "riscv_defs.v"

module alu (
    input  wire [`XLEN-1:0] alu_op1,
    input  wire [`XLEN-1:0] alu_op2,
    input  wire [3:0]       alu_control,
    output reg  [`XLEN-1:0] alu_result,
    output wire             alu_zero
);

    always @(*) begin
        case (alu_control)
            4'b0000: alu_result = alu_op1 + alu_op2; // ADD
            4'b1000: alu_result = alu_op1 - alu_op2; // SUB
            4'b0111: alu_result = alu_op1 & alu_op2; // AND
            4'b0110: alu_result = alu_op1 | alu_op2; // OR
            4'b0100: alu_result = alu_op1 ^ alu_op2; // XOR
            default: alu_result = `XLEN'b0;
        endcase
    end

    assign alu_zero = (alu_result == `XLEN'b0);

endmodule