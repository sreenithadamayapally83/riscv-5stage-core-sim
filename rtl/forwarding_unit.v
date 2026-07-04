`include "riscv_defs.v"

module forwarding_unit (
    input  wire [`REG_ADDR_WIDTH-1:0] id_ex_rs1,
    input  wire [`REG_ADDR_WIDTH-1:0] id_ex_rs2,
    input  wire [`REG_ADDR_WIDTH-1:0] ex_mem_rd,
    input  wire [`REG_ADDR_WIDTH-1:0] mem_wb_rd,
    input  wire                       ex_mem_reg_write,
    input  wire                       mem_wb_reg_write,
    output reg  [1:0]                 forward_a,
    output reg  [1:0]                 forward_b
);

    always @(*) begin
        // Default: No Forwarding (Use original values from ID/EX Latch)
        forward_a = 2'b00;
        forward_b = 2'b00;

        // --------------------------------------------------------------------
        // FORWARDING TO OPERAND A (rs1)
        // --------------------------------------------------------------------
        // EX Hazard: Most recent instruction writes to rs1
        if (ex_mem_reg_write && (ex_mem_rd != 5'b0) && (ex_mem_rd == id_ex_rs1)) begin
            forward_a = 2'b10; // Forward from EX/MEM stage output
        end
        // MEM Hazard: Older instruction writes to rs1
        else if (mem_wb_reg_write && (mem_wb_rd != 5'b0) && (mem_wb_rd == id_ex_rs1)) begin
            forward_a = 2'b01; // Forward from MEM/WB stage output
        end

        // --------------------------------------------------------------------
        // FORWARDING TO OPERAND B (rs2)
        // --------------------------------------------------------------------
        // EX Hazard: Most recent instruction writes to rs2
        if (ex_mem_reg_write && (ex_mem_rd != 5'b0) && (ex_mem_rd == id_ex_rs2)) begin
            forward_b = 2'b10; // Forward from EX/MEM stage output
        end
        // MEM Hazard: Older instruction writes to rs2
        else if (mem_wb_reg_write && (mem_wb_rd != 5'b0) && (mem_wb_rd == id_ex_rs2)) begin
            forward_b = 2'b01; // Forward from MEM/WB stage output
        end
    end

endmodule