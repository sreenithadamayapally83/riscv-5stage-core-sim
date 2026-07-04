#pragma once

#include <cstdint>
#include <vector>

namespace Core {

class BranchPredictor {
private:
    // 2-bit saturating counter table (indexed by lower PC bits)
    std::vector<uint8_t> bht;
    static constexpr size_t BHT_SIZE = 64; // Simple 64-entry history table

public:
    BranchPredictor() {
        // Initialize all branch history entries to 1 (Weakly Not Taken)
        bht.resize(BHT_SIZE, 1);
    }

    // Predicts if a branch at a given PC should be taken (True) or not (False)
    bool predict(uint32_t branch_pc) {
        size_t index = (branch_pc >> 2) % BHT_SIZE; // Drop byte alignment bits
        uint8_t counter = bht[index];
        return (counter >= 2); // 2 or 3 means Taken, 0 or 1 means Not Taken
    }

    // Updates the 2-bit saturating state machine based on the actual outcome
    void update(uint32_t branch_pc, bool actually_taken) {
        size_t index = (branch_pc >> 2) % BHT_SIZE;
        uint8_t counter = bht[index];

        if (actually_taken) {
            if (counter < 3) bht[index]++; // Saturate upward to Strongly Taken
        } else {
            if (counter > 0) bht[index]--; // Saturate downward to Strongly Not Taken
        }
    }
};

} // namespace Core