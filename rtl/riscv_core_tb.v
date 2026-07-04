`timescale 1ns/1ps
`include "riscv_defs.v"

module riscv_core_tb;

    // Testbench Driver Signals
    reg         clk;
    reg         rst_n;
    
    // Interconnect Memory Wires
    wire [31:0] imem_addr;
    wire [31:0] imem_rdata;
    wire [31:0] dmem_addr;
    wire         dmem_we;
    wire [31:0] dmem_wdata;
    reg  [31:0] dmem_rdata;

    // Instantiate the 5-Stage Core under test
    riscv_core u_dut (
        .clk        (clk),
        .rst_n      (rst_n),
        .imem_addr  (imem_addr),
        .imem_rdata (imem_rdata),
        .dmem_addr  (dmem_addr),
        .dmem_we    (dmem_we),
        .dmem_wdata (dmem_wdata),
        .dmem_rdata (dmem_rdata)
    );

    // Unified 64KB Virtual Memory Array Layout (Shared Instruction & Data)
    reg [7:0] memory_array [65535:0];

    // Read ports emulation (Little-Endian assembly mapping matching C++ logic)
    assign imem_rdata = {memory_array[imem_addr+3], memory_array[imem_addr+2], 
                         memory_array[imem_addr+1], memory_array[imem_addr]};

    always @(*) begin
        dmem_rdata = {memory_array[dmem_addr+3], memory_array[dmem_addr+2], 
                      memory_array[dmem_addr+1], memory_array[dmem_addr]};
    end

    // Synchronous Data Memory writes
    always @(posedge clk) begin
        if (dmem_we) begin
            memory_array[dmem_addr]   <= dmem_wdata[7:0];
            memory_array[dmem_addr+1] <= dmem_wdata[15:8];
            memory_array[dmem_addr+2] <= dmem_wdata[23:16];
            memory_array[dmem_addr+3] <= dmem_wdata[31:24];
        end
    end

    // Master Clock Generator (50MHz -> 20ns clock cycle duration)
    always #10 clk = ~clk;

    // Verification Driver Process
    integer cycle;
    initial begin
        // Initialize lines
        clk   = 1'b0;
        rst_n = 1'b0;
        cycle = 0;

        // Clear whole memory footprint
        for (integer i = 0; i < 65536; i = i + 1) begin
            memory_array[i] = 8'h00;
        end

        // Flash our exact 24-byte verified test program binary into memory (offset 0x0)
        // addi x1, x0, 10
        memory_array[0] = 8'h93; memory_array[1] = 8'h00; memory_array[2] = 8'ha0; memory_array[3] = 8'h00;
        // addi x2, x0, 20
        memory_array[4] = 8'h13; memory_array[5] = 8'h01; memory_array[6] = 8'h40; memory_array[7] = 8'h01;
        // add  x3, x1, x2
        memory_array[8] = 8'hb3; memory_array[9] = 8'h81; memory_array[10] = 8'h20; memory_array[11] = 8'h00;
        // sw   x3, 64(x0)
        memory_array[12] = 8'h23; memory_array[13] = 8'h20; memory_array[14] = 8'h30; memory_array[15] = 8'h04;
        // lw   x4, 64(x0)
        memory_array[16] = 8'h03; memory_array[17] = 8'h22; memory_array[18] = 8'h00; memory_array[19] = 8'h04;
        // halt (0x00000000)
        memory_array[20] = 8'h00; memory_array[21] = 8'h00; memory_array[22] = 8'h00; memory_array[23] = 8'h00;

        // Release system reset after 2 cycles
        #25;
        rst_n = 1'b1;
        $display("[RTL TESTBENCH] Hardware Core out of reset. Executing pipeline...");

        // Monitor execution for 12 clock cycles to let pipeline drain out
        for (cycle = 0; cycle < 12; cycle = cycle + 1) begin
            @(posedge clk);
            $display("Cycle: %0d | Core PC: 0x%h | WB Reg Dest: x%0d | WB Data: 0x%h", 
                     cycle, u_dut.pc, u_dut.mem_wb_rd, u_dut.wb_data);
        end

        // Final Co-Verification checks against C++ Golden reference output states
        $display("\n                  HARDWARE VERIFICATION METRICS                   ");
        $display("Value written to RAM Location 64: %d", 
                 {memory_array[67], memory_array[66], memory_array[65], memory_array[64]});
                 
        if ({memory_array[67], memory_array[66], memory_array[65], memory_array[64]} == 32'd30) begin
            $display("RESULT: SUCCESS! RTL hardware outputs match C++ Golden Reference Engine.");
        end else begin
            $display("RESULT: MISMATCH ERROR. Inspect forwarding paths.");
        end
        

        $finish;
    end

endmodule