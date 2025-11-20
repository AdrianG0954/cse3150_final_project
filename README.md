# CSE3150 Final Project

## Running Tests

This project uses Google Test (GTest) for comprehensive unit testing.

### Build and Run Tests:

```bash
# Create build directory and configure
mkdir -p build && cd build

 
# Build the project
# - cmake configures the project and adds dependencies)
# - make builds the code
cmake .. && make

# Run tests with detailed output
./run_tests

# Or run tests via CMake
make test
```

### Clean Build:

```bash
# Remove build directory and start fresh
rm -rf build
```

The test suite includes 20 tests covering:

- AS class functionality (constructors, relationships, getters)
- AsGraph functionality (parsing, graph building, cycle detection)
- Edge cases and error handling
