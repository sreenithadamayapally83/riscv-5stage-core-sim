#include "component.hpp"
#include "pipelined_cpu.hpp"
#include <iostream>
#include <vector>

int main() {
    std::cout << "[SIMULATOR] Launching Maximum Structural Stress Framework\n";
    Core::Memory memory(65536);

    // DEEP STRESS ASSEMBLY WORKLOAD: High-Density Cache & Hazard Compulsion Pattern
    std::vector<uint8_t> stress_program = {
        //Stage 1: Setup Bounds & Indexes 
        0x93, 0x00, 0x40, 0x00, // 0x00: addi x1, x0, 4        (Loop limit constraint = 4 iterations)
        0x13, 0x02, 0x00, 0x00, // 0x04: addi x4, x0, 0        (Loop iterator index i = 0)
        0x93, 0x02, 0x00, 0x08, // 0x08: addi x5, x0, 128      (Cache index stride offset = 128 bytes)

        // Stage 2: Loop Body (RAW Hazards & High Spatial Stride)
        0x33, 0x03, 0x42, 0x00, // 0x0C: add  x6, x4, x5       (RAW Dependency on x4 -> Triggers Forwarding)
        0x23, 0x20, 0x62, 0x00, // 0x10: sw   x6, 0(x4)        (Store calculated index to RAM)
        0x03, 0x27, 0x02, 0x00, // 0x14: lw   x7, 0(x4)        (Load value back instantly -> Triggers Load-Use Stall!)
        
        //Stage 3: Cache Set Thrashing Segment (LRU Eviction Trigger)
        // Repeatedly writes to memory addresses designed to collision-map to the same Cache Sets
        0x23, 0x20, 0x70, 0x00, // 0x18: sw   x7, 0(0)         (Maps to Set 0)
        0x23, 0x20, 0x70, 0x08, // 0x1C: sw   x7, 0(128)       (Maps to Set 0, Way 1)
        0x23, 0x20, 0x70, 0x10, // 0x20: sw   x7, 0(256)       (Maps to Set 0 -> Forces Cache Eviction of address 0!)

        // Stage 4: Control Path Iteration Check
        0x13, 0x02, 0x12, 0x00, // 0x24: addi x4, x4, 1        (Increment index)
        0x63, 0x06, 0x12, 0xfe, // 0x28: beq  x4, x1, -28      (Loop condition check -> Misprediction Flush on exit)
        
        //Finish
        0x00, 0x00, 0x00, 0x00  // 0x2C: halt
    };

    uint32_t entry_point = 0x0000;
    memory.load_binary(entry_point, stress_program);

    Core::PipelinedCPU cpu(entry_point, memory);

    std::cout << "[SIMULATOR] Commencing Cycle Trace Printing:\n";
    std::cout << "----------------------------------------------------------------\n";
    
    uint32_t cycle_count = 0;
    while (!cpu.is_halted() && cycle_count < 200) {
        cpu.clock_cycle();
        cycle_count++;
    }

    cpu.print_performance_metrics();

    return 0;
}