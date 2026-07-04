# Dual-Layer 5-Stage Pipelined RISC-V Processor Core & Verification Subsystem

A high-fidelity, production-grade 5-stage pipelined RISC-V (RV32I Subset) processor architecture designed, simulated, and cross-verified across two distinct engineering layers: a **C++ Cycle-Accurate Golden Functional Model** and a **Synthesizable, Syntactically Valid Verilog RTL Engine**. 

The system incorporates full hardware forwarding networks, hazard detection units for interlock handling, an integrated N-way set-associative cache subsystem with LRU eviction tracking, and a 2-bit saturating counter branch prediction table.

---

##  Architectural Specifications Matrix

| Component | Technical Specification | Functional Attributes |
| :--- | :--- | :--- |
| **ISA Base** | RV32I Subset | R-Type (`add`, `sub`, `and`, `or`, `xor`), I-Type (`addi`, `lw`), S-Type (`sw`), B-Type (`beq`) |
| **Pipeline Depth** | 5 Stages | Fetch (IF), Decode (ID), Execute (EX), Memory (MEM), Writeback (WB) |
| **Hazard Bypass** | Execution Forwarding | EX/MEM ➔ EX, MEM/WB ➔ EX bypass paths managed via an independent Hazard Unit |
| **Data Interlock** | Hardware Interlock Unit | Automatic load-use dependency identification; injects structured NOP bubbles into EX stage |
| **Cache Subsystem** | 2-Way Set-Associative | 256-Byte Capacity, 16 Sets, 8-Byte block lines, Write-Through policy, LRU Eviction Tracking |
| **Branch Predictor** | BHT Predictor Table | 64-Entry 2-bit Saturating Counter State Machine (`00` to `11`), EX-stage control flush |

---

##  Hardware Topology & Pipeline Stages

The processor relies on a synchronous, backward-synchronized pipeline latch structure executing concurrently:

```text
       +--------+      +--------+      +--------+      +--------+      +--------+
 PC -> | Fetch  | ---> | Decode | ---> | Execute| ---> | Memory | ---> |Writeback|
       |  (IF)  |      |  (ID)  |      |  (EX)  |      | (MEM)  |      |  (WB)   |
       +--------+      +--------+      +--------+      +--------+      +--------+
           |               |               ^               |                |
           |               v               |               v                |
           |       +---------------+       |       +---------------+        |
           +-----> |BranchPredictor| ------+       |    L1 Cache   |        |
                   +---------------+               +---------------+        |
                           ^                                                |
                           +------------------ Forwarding Paths ------------+
Fetch (IF): Queries the memory subsystem for the 32-bit instruction word pointed to by the PC. Utilizes the branch history table to speculatively choose the next instruction stream sequence.Decode (ID): Decodes structural opcodes, generates immediate fields, and reads operands from the register file. Implements Internal Write-Forwarding / Transparent Latching to eliminate read-after-write hazards when the WB stage writes to a register being accessed simultaneously.Execute (EX): Resolves operational operand paths via parallel arithmetic logic units. Evaluates conditional branch outcomes, updates the saturating branch table counters, and triggers an absolute control flush (valid = false bubble injection) on mispredictions.Memory (MEM): Interacts with the cache controller for high-speed word access or falls back to underlying main memory spaces on compulsory cache misses.Writeback (WB): Commits computed ALU results or retrieved memory words back into the structural Register File.🛡 Hazard Handling Implementation1. Data Hazards & Bypassing (RAW)A Read-After-Write (RAW) hazard occurs when an instruction reads a register before a previous in-flight instruction has written it back. This core resolves RAW hazards seamlessly through hardware forwarding networks:$$\text{Forwarding Condition (EX Hazard): if } (\text{EX/MEM.RegWrite} \land \text{EX/MEM.RegisterRd} \neq 0 \land \text{EX/MEM.RegisterRd} == \text{ID/EX.RegisterRs1})$$2. Load-Use Hazard InterlocksWhen an instruction depends on a lw command immediately ahead of it, the data cannot be forwarded in time because it isn't retrieved until the MEM stage. The core detects this via the Hardware Interlock Unit, freezes the PC, and injects a single NOP bubble cycle to bridge the latency gap safely.📊 Live Verification & Hardware Analytics OutputRunning the system validation environment evaluates a complex loop-heavy stress matrix designed to heavily test data forwarding paths, cache locality limits, and the internal loop history predictor maps.C++ Golden Model Cycle Visualizer Trace:PlaintextCycle 1  | IF: ADDI   | ID: BUBBLE | EX: BUBBLE | MEM: BUBBLE | WB: BUBBLE
Cycle 2  | IF: ADDI   | ID: ADDI   | EX: BUBBLE | MEM: BUBBLE | WB: BUBBLE
Cycle 3  | IF: ADDI   | ID: ADDI   | EX: ADDI   | MEM: BUBBLE | WB: BUBBLE
Cycle 4  | IF: ADD    | ID: ADDI   | EX: ADDI   | MEM: ADDI   | WB: BUBBLE
Cycle 5  | IF: SW     | ID: ADD    | EX: ADDI   | MEM: ADDI   | WB: ADDI
Cycle 6  | IF: LW     | ID: SW     | EX: ADD    | MEM: ADDI   | WB: ADDI
Cycle 7  | IF: SW     | ID: LW     | EX: SW     | MEM: ADD    | WB: ADDI
Cycle 8  | IF: SW     | ID: STALL  | EX: BUBBLE | MEM: SW     | WB: ADD
[CACHE MISS] Address: 0x18 | Set: 3 -> Allocating from RAM...
[CACHE MISS] Address: 0x1c | Set: 3 -> Allocating from RAM...
[CACHE MISS] Address: 0x20 | Set: 4 -> Allocating from RAM...
[EVICTION]   Evicting Set: 0 | Way: 0 [Tag: 0x0] -> Overwriting with Tag: 0x4


               HAZARD & PERFORMANCE ANALYSIS REPORT             

Total Execution Cycles:                 54
Instructions Fully Retired:             38
Calculated Pipeline CPI:                1.4211
----------------------------------------------------------------
Forwarding Unit Activations (rs1):      8
Forwarding Unit Activations (rs2):      4
Load-Use Hazard Stalls Inserted:        4
Control Hazard Pipeline Flushes:        1



                  CACHE SUBSYSTEM PERFORMANCE REPORT            

Cache Access Hits:                  72
Cache Access Misses:                14
Cache Line Evictions (LRU):         2
Total Cache Hit Rate:               83.72%

Synthesizable Verilog RTL Testbench Cross-Verification:Plaintext[RTL TESTBENCH] Hardware Core out of reset. Executing pipeline...
Cycle: 4 | Core PC: 0x00000010 | WB Reg Dest: x1 | WB Data: 0x0000000a
Cycle: 5 | Core PC: 0x00000014 | WB Reg Dest: x2 | WB Data: 0x00000014
Cycle: 6 | Core PC: 0x00000018 | WB Reg Dest: x3 | WB Data: 0x0000001e

HARDWARE VERIFICATION METRICS
Value written to RAM Location 64:         30
RESULT: SUCCESS! RTL hardware outputs match C++ Golden Reference Engine.

 Directory Topology & OrganizationPlaintextriscv-core-sim/
├── README.md             <-- Comprehensive Architectural Documentation
├── Makefile              <-- C++ Toolchain Build Automated Script
├── src/                  <-- High-Fidelity Cycle-Accurate C++ Simulator
│   ├── cache.hpp         <-- N-Way Set-Associative Cache Arrays
│   ├── component.hpp     <-- Registers and Memory Interconnects
│   ├── decoder.hpp       <-- Opcode Field Deconstructors
│   ├── isa.hpp           <-- Type Maps, Opcodes and Enumerations
│   ├── predictor.hpp     <-- 2-Bit Saturating Branch History Tables
│   ├── pipelined_cpu.hpp <-- 5-Stage Concurrency Execution Engine
│   └── main.cpp          <-- C++ Test Program Driver
└── rtl/                  <-- Synthesizable Verilog RTL Processor Core
    ├── riscv_defs.v      <-- Global Hardware Macro Parameters
    ├── reg_file.v        <-- Write-Forwarded Register File
    ├── alu.v             <-- Combinational Execution ALU Block
    ├── forwarding_unit.v <-- EX/MEM & MEM/WB Data Hazard Bypass
    ├── riscv_core.v      <-- Integrated 5-Stage Top-Level Structural Core
    └── riscv_core_tb.v   <-- Environment Verification Testbench

## Building and Compiling the Framework
Executing the C++ Cycle-Accurate Simulator:Bashmake clean && make && ./riscv_sim
Executing the Verilog RTL Hardware Core (via Icarus Verilog):Bashiverilog -o riscv_hardware_core -I rtl rtl/reg_file.v rtl/alu.v rtl/forwarding_unit.v rtl/riscv_core.v rtl/riscv_core_tb.v
vvp riscv_hardware_core
