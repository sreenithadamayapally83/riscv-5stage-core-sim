#pragma once

#include <cstdint>
#include <vector>
#include <iostream>
#include <iomanip>

namespace Core {

struct CacheLine {
    bool valid     = false;
    bool dirty     = false;
    uint32_t tag   = 0;
    uint32_t lru_age = 0;   
    uint8_t data[8] = {0};  
};

class Cache {
private:
    static constexpr size_t NUM_SETS = 16;
    static constexpr size_t NUM_WAYS = 2;
    std::vector<std::vector<CacheLine>> sets;
    
    uint32_t hit_count = 0;
    uint32_t miss_count = 0;
    uint32_t eviction_count = 0;

public:
    Cache() {
        sets.resize(NUM_SETS, std::vector<CacheLine>(NUM_WAYS));
    }

    void parse_address(uint32_t addr, uint32_t& tag, uint32_t& index, uint32_t& offset) {
        offset = addr & 0x7;            
        index  = (addr >> 3) & 0xF;     
        tag    = addr >> 7;             
    }

    bool access(uint32_t addr, uint32_t& data_out) {
        uint32_t tag, index, offset;
        parse_address(addr, tag, index, offset);

        auto& current_set = sets[index];

        for (size_t way = 0; way < NUM_WAYS; ++way) {
            if (current_set[way].valid && current_set[way].tag == tag) {
                hit_count++;
                current_set[way].lru_age = 1;      
                current_set[way ^ 1].lru_age = 0;  
                
                data_out = current_set[way].data[offset] |
                           (current_set[way].data[offset + 1] << 8) |
                           (current_set[way].data[offset + 2] << 16) |
                           (current_set[way].data[offset + 3] << 24);
                
                std::cout << "[CACHE HIT]  Address: 0x" << std::hex << addr 
                          << " | Set: " << std::dec << index << " | Way: " << way << "\n";
                return true; 
            }
        }

        miss_count++;
        std::cout << "[CACHE MISS] Address: 0x" << std::hex << addr 
                  << " | Set: " << std::dec << index << " -> Allocating from RAM...\n";
        return false;
    }

    void allocate(uint32_t addr, const uint8_t* block_data) {
        uint32_t tag, index, offset;
        parse_address(addr, tag, index, offset);

        auto& current_set = sets[index];
        size_t target_way = 0;

        if (current_set[0].valid && !current_set[1].valid) {
            target_way = 1;
        } else if (current_set[0].valid && current_set[1].valid) {
            // Eviction condition triggered
            target_way = (current_set[0].lru_age < current_set[1].lru_age) ? 0 : 1;
            eviction_count++;
            std::cout << "[EVICTION]   Evicting Set: " << std::dec << index 
                      << " | Way: " << target_way << " [Tag: 0x" << std::hex << current_set[target_way].tag 
                      << "] -> Overwriting with Tag: 0x" << tag << "\n";
        }

        current_set[target_way].valid = true;
        current_set[target_way].tag = tag;
        current_set[target_way].lru_age = 1;
        current_set[target_way ^ 1].lru_age = 0; 
        
        for (int i = 0; i < 8; ++i) {
            current_set[target_way].data[i] = block_data[i];
        }
    }

    void print_stats() const {
        std::cout << "                  CACHE SUBSYSTEM PERFORMANCE REPORT            \n";
        std::cout << std::left << std::setw(35) << "Cache Access Hits:" << hit_count << "\n";
        std::cout << std::left << std::setw(35) << "Cache Access Misses:" << miss_count << "\n";
        std::cout << std::left << std::setw(35) << "Cache Line Evictions (LRU):" << eviction_count << "\n";
        if (hit_count + miss_count > 0) {
            double rate = (100.0 * hit_count) / (hit_count + miss_count);
            std::cout << std::left << std::setw(35) << "Total Cache Hit Rate:" << std::fixed << std::setprecision(2) << rate << "%\n";
        }
    }
};

} // namespace Core