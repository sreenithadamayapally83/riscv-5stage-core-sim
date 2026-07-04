# Compiler configuration
CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2

# Directories
SRC_DIR := src
BUILD_DIR := build

# Target Executable Name
TARGET := riscv_sim

# Source files and Object files mapping
SRCS := $(wildcard $(SRC_DIR)/*.cpp)
OBJS := $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(SRCS))

# Default build rule
all: $(TARGET)

# Link object files together to create executable
$(TARGET): $(OBJS)
	@echo "[LINK] Linking executable: $@"
	$(CXX) $(CXXFLAGS) $^ -o $@

# Compile source files to object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	@echo "[COMPILE] Compiling: $<"
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Create build directory if it doesn't exist
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Clean up build artifacts
clean:
	@echo "[CLEAN] Removing build files and executable..."
	rm -rf $(BUILD_DIR) $(TARGET)

.PHONY: all clean