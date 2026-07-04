`include "riscv_defs.v"

module riscv_core (
    input  wire         clk,
    input  wire         rst_n,
    
    // Instruction Memory Interface
    output wire [31:0]  imem_addr,
    input  wire [31:0]  imem_rdata,
    
    // Data Memory Interface
    output wire [31:0]  dmem_addr,
    output wire         dmem_we,
    output wire [31:0]  dmem_wdata,
    input  wire [31:0]  dmem_rdata
);

    // PIPELINE REGISTER SIGNAL DEFINITIONS
    
    // FETCH (IF) STAGE
    reg  [31:0] pc;
    wire [31:0] next_pc;

    // DECODE (ID) STAGE REGS 
    reg  [31:0] if_id_pc;
    reg  [31:0] if_id_instr;

    //  EXECUTE (EX) STAGE REGS
    reg  [31:0] id_ex_pc;
    reg  [31:0] id_ex_rdata1;
    reg  [31:0] id_ex_rdata2;
    reg  [31:0] id_ex_imm;
    reg  [4:0]  id_ex_rs1;
    reg  [4:0]  id_ex_rs2;
    reg  [4:0]  id_ex_rd;
    reg  [6:0]  id_ex_opcode;
    reg  [2:0]  id_ex_funct3;
    reg         id_ex_funct7_alt;
    reg         id_ex_reg_write;
    reg         id_ex_mem_to_reg;
    reg         id_ex_mem_write;
    reg         id_ex_alu_src;

    //  MEMORY (MEM) STAGE REGS
    reg  [31:0] ex_mem_pc;
    reg  [31:0] ex_mem_alu_result;
    reg  [31:0] ex_mem_wdata;
    reg  [4:0]  ex_mem_rd;
    reg         ex_mem_reg_write;
    reg         ex_mem_mem_to_reg;
    reg         ex_mem_mem_write;

    // WRITEBACK (WB) STAGE REGS
    reg  [31:0] mem_wb_alu_result;
    reg  [31:0] mem_wb_rdata;
    reg  [4:0]  mem_wb_rd;
    reg         mem_wb_reg_write;
    reg         mem_wb_to_reg;


    // 1. FETCH (IF) STAGE INTERCONNECTS
    assign imem_addr = pc;
    assign next_pc   = pc + 4; // Sequential baseline execution

    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            pc <= 32'h00000000;
        end else begin
            pc <= next_pc;
        end
    end

    // IF/ID Pipeline Register Latch Step
    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            if_id_pc    <= 32'b0;
            if_id_instr <= 32'b0;
        end else begin
            if_id_pc    <= pc;
            if_id_instr <= imem_rdata;
        end
    end


    // 2. DECODE (ID) STAGE INTERCONNECTS
    wire [4:0]  id_rs1    = if_id_instr[19:15];
    wire [4:0]  id_rs2    = if_id_instr[24:20];
    wire [4:0]  id_rd     = if_id_instr[11:7];
    wire [6:0]  id_opcode = if_id_instr[6:0];
    wire [2:0]  id_funct3 = if_id_instr[14:12];
    
    // Immediate Generation Logic
    reg [31:0]  id_imm;
    always @(*) begin
        case (id_opcode)
            `OP_I_TYPE_IMM, `OP_I_TYPE_LOAD: id_imm = {{20{if_id_instr[31]}}, if_id_instr[31:20]};
            `OP_S_TYPE_STORE:               id_imm = {{20{if_id_instr[31]}}, if_id_instr[31:25], if_id_instr[11:7]};
            default:                        id_imm = 32'b0;
        endcase
    end

    // Control Unit Decoder Signal Mapping
    reg id_reg_write, id_mem_to_reg, id_mem_write, id_alu_src;
    always @(*) begin
        id_reg_write   = (id_opcode == `OP_R_TYPE || id_opcode == `OP_I_TYPE_IMM || id_opcode == `OP_I_TYPE_LOAD);
        id_mem_to_reg  = (id_opcode == `OP_I_TYPE_LOAD);
        id_mem_write   = (id_opcode == `OP_S_TYPE_STORE);
        id_alu_src     = (id_opcode != `OP_R_TYPE);
    end

    wire [31:0] rf_rdata1, rf_rdata2;
    wire [31:0] wb_data; // Fed back from WB stage output multiplexer

    reg_file u_reg_file (
        .clk    (clk),
        .rst_n  (rst_n),
        .raddr1 (id_rs1),
        .raddr2 (id_rs2),
        .we     (mem_wb_reg_write),
        .waddr  (mem_wb_rd),
        .wdata  (wb_data),
        .rdata1 (rf_rdata1),
        .rdata2 (rf_rdata2)
    );

    // ID/EX Pipeline Register Latch Step
    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            id_ex_pc         <= 32'b0;
            id_ex_rdata1     <= 32'b0;
            id_ex_rdata2     <= 32'b0;
            id_ex_imm        <= 32'b0;
            id_ex_rs1        <= 5'b0;
            id_ex_rs2        <= 5'b0;
            id_ex_rd         <= 5'b0;
            id_ex_opcode     <= 7'b0;
            id_ex_funct3     <= 3'b0;
            id_ex_funct7_alt <= 1'b0;
            id_ex_reg_write  <= 1'b0;
            id_ex_mem_to_reg <= 1'b0;
            id_ex_mem_write  <= 1'b0;
            id_ex_alu_src    <= 1'b0;
        end else begin
            id_ex_pc         <= if_id_pc;
            id_ex_rdata1     <= rf_rdata1;
            id_ex_rdata2     <= rf_rdata2;
            id_ex_imm        <= id_imm;
            id_ex_rs1        <= id_rs1;
            id_ex_rs2        <= id_rs2;
            id_ex_rd         <= id_rd;
            id_ex_opcode     <= id_opcode;
            id_ex_funct3     <= id_funct3;
            id_ex_funct7_alt <= (if_id_instr[31:25] == `F7_ALT);
            id_ex_reg_write  <= id_reg_write;
            id_ex_mem_to_reg <= id_mem_to_reg;
            id_ex_mem_write  <= id_mem_write;
            id_ex_alu_src    <= id_alu_src;
        end
    end


    // 3. EXECUTE (EX) STAGE INTERCONNECTS
    wire [1:0] forward_a, forward_b;
    reg  [31:0] alu_op1, forwarded_rs2_val;

    forwarding_unit u_forwarding_unit (
        .id_ex_rs1        (id_ex_rs1),
        .id_ex_rs2        (id_ex_rs2),
        .ex_mem_rd        (ex_mem_rd),
        .mem_wb_rd        (mem_wb_rd),
        .ex_mem_reg_write (ex_mem_reg_write),
        .mem_wb_reg_write (mem_wb_reg_write),
        .forward_a        (forward_a),
        .forward_b        (forward_b)
    );

    // MUX select lines for operand A bypass pathing
    always @(*) begin
        case (forward_a)
            2'b10:   alu_op1 = ex_mem_alu_result;
            2'b01:   alu_op1 = wb_data;
            default: alu_op1 = id_ex_rdata1;
        endcase
    end

    // MUX select lines for operand B bypass pathing
    always @(*) begin
        case (forward_b)
            2'b10:   forwarded_rs2_val = ex_mem_alu_result;
            2'b01:   forwarded_rs2_val = wb_data;
            default: forwarded_rs2_val = id_ex_rdata2;
        endcase
    end

    // ALU Source selection operand multiplexer
    wire [31:0] alu_op2 = (id_ex_alu_src) ? id_ex_imm : forwarded_rs2_val;

    // Local ALU controller decoder
    reg [3:0] alu_ctrl;
    always @(*) begin
        if (id_ex_opcode == `OP_R_TYPE) begin
            case (id_ex_funct3)
                `F3_ADD_SUB: alu_ctrl = (id_ex_funct7_alt) ? 4'b1000 : 4'b0000; // SUB vs ADD
                `F3_AND:     alu_ctrl = 4'b0111;
                `F3_OR:      alu_ctrl = 4'b0110;
                `F3_XOR:     alu_ctrl = 4'b0100;
                default:     alu_ctrl = 4'b0000;
            endcase
        end else begin
            alu_ctrl = 4'b0000; // Load/Store address offsets or ADDI default to additions
        end
    end

    wire [31:0] ex_alu_result;
    alu u_alu (
        .alu_op1     (alu_op1),
        .alu_op2     (alu_op2),
        .alu_control (alu_ctrl),
        .alu_result  (ex_alu_result),
        .alu_zero    ()
    );

    // EX/MEM Pipeline Register Latch Step
    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            ex_mem_pc         <= 32'b0;
            ex_mem_alu_result <= 32'b0;
            ex_mem_wdata      <= 32'b0;
            ex_mem_rd         <= 5'b0;
            ex_mem_reg_write  <= 1'b0;
            ex_mem_mem_to_reg <= 1'b0;
            ex_mem_mem_write  <= 1'b0;
        end else begin
            ex_mem_pc         <= id_ex_pc;
            ex_mem_alu_result <= ex_alu_result;
            ex_mem_wdata      <= forwarded_rs2_val;
            ex_mem_rd         <= id_ex_rd;
            ex_mem_reg_write  <= id_ex_reg_write;
            ex_mem_mem_to_reg <= id_ex_mem_to_reg;
            ex_mem_mem_write  <= id_ex_mem_write;
        end
    end


    // 4. MEMORY (MEM) STAGE INTERCONNECTS
    assign dmem_addr  = ex_mem_alu_result;
    assign dmem_we    = ex_mem_mem_write;
    assign dmem_wdata = ex_mem_wdata;

    // EX/MEM Pipeline Register Latch Step
    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            mem_wb_alu_result <= 32'b0;
            mem_wb_rdata      <= 32'b0;
            mem_wb_rd         <= 5'b0;
            mem_wb_reg_write  <= 1'b0;
            mem_wb_to_reg     <= 1'b0;
        end else begin
            mem_wb_alu_result <= ex_mem_alu_result;
            mem_wb_rdata      <= dmem_rdata;
            mem_wb_rd         <= ex_mem_rd;
            mem_wb_reg_write  <= ex_mem_reg_write;
            mem_wb_to_reg     <= ex_mem_mem_to_reg;
        end
    end


    // 5. WRITEBACK (WB) STAGE INTERCONNECTS
    assign wb_data = (mem_wb_to_reg) ? mem_wb_rdata : mem_wb_alu_result;

endmodule