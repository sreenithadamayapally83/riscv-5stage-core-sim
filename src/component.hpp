#pragma once

#include "cache.hpp"
#include <cstdint>
#include <vector>
#include <stdexcept>
#include <iostream>

namespace Core {

// REGISTER FILE
class RegisterFile {
private:
    uint32_t regs[32] = {0};

public:
    uint32_t read(uint8_t index) const {
        if (index >= 32) throw std::out_of_range("Register index out of bounds");
        return regs[index];
    }

    void write(uint8_t index, uint32_t value) {
        if (index >= 32) throw std::out_of_range("Register index out of bounds");
        if (index != 0) regs[index] = value;
    }

    uint32_t read_forwarded(uint8_t read_idx, uint8_t write_idx, uint32_t write_data) const {
        if (read_idx >= 32) throw std::out_of_range("Register index out of bounds");
        if (read_idx == 0) return 0;
        if (read_idx == write_idx) return write_data;
        return regs[read_idx];
    }

    void dump() const {
        std::cout << "=== REGISTER FILE STATE ===\n";
        for (int i = 0; i < 32; i += 4) {
            for (int j = 0; j < 4; ++j) {
                int reg_idx = i + j;
                std::cout << "x" << reg_idx << ": 0x" << std::hex << regs[reg_idx] << "\t";
            }
            std::cout << "\n";
        }
        std::cout << std::dec;
    }
};

// SYSTEM MEMORY INTERCONNECT WITH COUPLING CACHE
class Memory {
private:
    std::vector<uint8_t> ram;
    mutable Cache core_cache; // Instantiated cache hardware block

public:
    explicit Memory(size_t size_bytes) {
        ram.resize(size_bytes, 0);
    }

    // High-level wrapper that queries cache before fallback access
    uint32_t read_word(uint32_t address) const {
        if (address + 3 >= ram.size()) throw std::out_of_range("Memory access out of bounds");

        uint32_t cache_data = 0;
        // 1. Check if the address hits in our 2-Way Cache
        if (core_cache.access(address, cache_data)) {
            return cache_data; // Cache Hit! Fast return
        }

        // 2. Cache Miss: Fetch the 32-bit word directly from RAM
        uint32_t ram_data = ram[address]         | 
                           (ram[address + 1] << 8)  | 
                           (ram[address + 2] << 16) | 
                           (ram[address + 3] << 24);

        // 3. Line Allocation: Align address to an 8-byte block boundary and fetch its data
        uint32_t block_base_addr = address & ~0x7; 
        uint8_t block_buffer[8];
        for (int i = 0; i < 8; ++i) {
            block_buffer[i] = ram[block_base_addr + i];
        }

        // Commit this block to cache storage using our replacement unit
        core_cache.allocate(address, block_buffer);

        return ram_data;
    }

    void write_word(uint32_t address, uint32_t value) {
        if (address + 3 >= ram.size()) throw std::out_of_range("Memory access out of bounds");

        // Write-Through to Main RAM
        ram[address]     = value & 0xFF;
        ram[address + 1] = (value >> 8) & 0xFF;
        ram[address + 2] = (value >> 16) & 0xFF;
        ram[address + 3] = (value >> 24) & 0xFF;

        // Update/allocate back into the cache block if we want to retain it
        uint32_t block_base_addr = address & ~0x7;
        uint8_t block_buffer[8];
        for (int i = 0; i < 8; ++i) {
            block_buffer[i] = ram[block_base_addr + i];
        }
        core_cache.allocate(address, block_buffer);
    }

    void load_binary(uint32_t start_address, const std::vector<uint8_t>& binary_data) {
        if (start_address + binary_data.size() > ram.size()) throw std::runtime_error("Binary oversized");
        for (size_t i = 0; i < binary_data.size(); ++i) {
            ram[start_address + i] = binary_data[i];
        }
    }

    // EXPLICIT FUNCTION TO PRINT METRICS
    void print_cache_metrics() const {
        core_cache.print_stats();
    }
};

} // namespace Core