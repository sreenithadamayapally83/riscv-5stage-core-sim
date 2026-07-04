`include "riscv_defs.v"

module reg_file (
    input  wire                 clk,
    input  wire                 rst_n,
    input  wire [`REG_ADDR_WIDTH-1:0] raddr1,
    input  wire [`REG_ADDR_WIDTH-1:0] raddr2,
    input  wire                 we,
    input  wire [`REG_ADDR_WIDTH-1:0] waddr,
    input  wire [`XLEN-1:0]     wdata,
    output wire [`XLEN-1:0]     rdata1,
    output wire [`XLEN-1:0]     rdata2
);

    reg [`XLEN-1:0] rf [31:0];
    integer i;

    // Synchronous Writes (x0 is hardwired to 0)
    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            for (i = 0; i < 32; i = i + 1) begin
                rf[i] <= `XLEN'b0;
            end
        end else if (we && (waddr != 5'b0)) begin
            rf[waddr] <= wdata;
        end
    end

    // Asynchronous Reads with Internal Write-Forwarding Bypass
    assign rdata1 = (raddr1 == 5'b0) ? `XLEN'b0 :
                    ((we && (waddr == raddr1)) ? wdata : rf[raddr1]);
                    
    assign rdata2 = (raddr2 == 5'b0) ? `XLEN'b0 :
                    ((we && (waddr == raddr2)) ? wdata : rf[raddr2]);

endmodule