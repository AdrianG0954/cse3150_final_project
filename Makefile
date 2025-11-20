# Simple Makefile wrapper for CMake project
# This keeps the source directory clean while using CMake

.PHONY: all build test clean run help cmake-test

# Default target
all: build

# Build the project
build:
	@./build.sh build

# Run tests with full GTest output
test:
	@./build.sh test

# Run tests via CMake test runner
cmake-test:
	@./build.sh cmake-test

# Run the main program
run:
	@./build.sh run

# Clean build directory
clean:
	@./build.sh clean

# Show help
help:
	@echo "Available targets:"
	@echo "  build      - Build the project (default)"
	@echo "  test       - Build and run tests with full output"
	@echo "  cmake-test - Build and run tests via CMake"
	@echo "  run        - Build and run main program"
	@echo "  clean      - Remove build directory"
	@echo "  help       - Show this help"
