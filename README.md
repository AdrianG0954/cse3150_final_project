# CSE3150 Final Project

### How to Build and Run

```bash
# Create build directory (if doesnt exist)
mkdir -p build && cd build

# using the release build type to avoid extra time
# MAKES CODE SOOO MUCH FASTER
cmake -DCMAKE_BUILD_TYPE=Release .. && make

# Run tests
./run_tests

# Or run tests via CMake
make test

# Run main code
./main
```

### Clean Build:

```bash
# Remove build directory and start fresh
rm -rf build
```

## Code Design Choices

### `AsGraph.h/cpp`

This code defines the main graph structure. I chose to split the graph into two components:

#### Class Variables

- **asMap**
  - Key: The ASN as an integer
  - Value: A unique pointer to the actual AS node instance

I chose this design to easily access any node without iterating over the `adjacencyList`. My choice of `unique_ptr` ensures the node instance persists outside of its original scope while providing automatic memory management.

- **adjacencyList**
  - Key: The integer ASN for a specific node
  - Value: A vector of pairs representing the node's edges and their connection types

This is the actual representation of the graph. When beginning the project, I considered two implementation approaches:

1. Create a separate graph for each type of relationship
2. Have one graph containing all relationships

I chose the second option as it greatly reduced code complexity and improved my understanding of the project as a whole.

#### Functions

- **flattenGraph**
  - Responsible for determining the ranks of all nodes and setting the `flattenedGraph` for future propagation
- **processAnnouncementsRange**
  - Takes a reference to the rank's ASNs, the range, and the asMap
  - Responsible for calling `processAnnouncements` for each node
  - This function is important as it enables each propagation method to optimize the workload
- **All Propagation Methods**
  - To further optimize each propagation method, I used multithreading
  - Per the assignment instructions, there is a strict 2-CPU core limit, so I only use two threads
  - I optimize by splitting the workload (each rank's ASNs) in half and assigning each half to a thread
  - This doubles efficiency while avoiding race conditions since threads don't share data

### `Relationships.h`

This file contains enums representing each relationship and relationship type. I chose enums because, after research, I found they are much faster for comparisons than strings. This resulted in dramatically improved performance for relationship comparisons and made the code more readable.
