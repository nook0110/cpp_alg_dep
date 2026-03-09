# Polynomial Algebraic Dependency Checker (C++ Version)

A C++ implementation of the polynomial algebraic dependency checker that analyzes polynomial pairs (f, g) ∈ Z[x,y] to find algebraic dependencies q(x, f, g) = 0 and verify divisibility conditions.

## Features

✅ **Brute Force Mode**: Systematically enumerate and check polynomial pairs  
✅ **Manual Check Mode**: Test specific polynomial pairs  
✅ **State Persistence**: Resume interrupted searches from checkpoints  
✅ **Result Caching**: SQLite database prevents recomputation  
✅ **Configurable**: Adjustable degrees, coefficient ranges, enumeration strategies  
✅ **High Performance**: C++ implementation with GiNaC symbolic computation library

## Dependencies

The C++ version requires the following libraries:

- **GiNaC** (>= 1.8): Symbolic computation library
- **CLN**: Class Library for Numbers (required by GiNaC)
- **SQLite3**: Database for result caching
- **nlohmann/json**: JSON library for state persistence
- **CMake** (>= 3.15): Build system
- **C++20 compiler**: GCC 10+, Clang 12+, or MSVC 2019+

## Installation

### Ubuntu/Debian

```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    libginac-dev \
    libcln-dev \
    libsqlite3-dev \
    nlohmann-json3-dev \
    pkg-config
```

### Fedora/RHEL

```bash
sudo dnf install -y \
    gcc-c++ \
    cmake \
    ginac-devel \
    cln-devel \
    sqlite-devel \
    json-devel \
    pkgconfig
```

### macOS (Homebrew)

```bash
brew install cmake ginac cln sqlite nlohmann-json pkg-config
```

### Arch Linux

```bash
sudo pacman -S cmake ginac cln sqlite nlohmann-json pkgconf
```

## Building

```bash
mkdir build
cd build
cmake ..
make -j$(nproc)
```

The executable will be created as `build/poly_dep_check`.

### Installation (Optional)

```bash
sudo make install
```

This installs the binary to `/usr/local/bin/poly_dep_check`.

## Quick Start

### Check a Specific Polynomial Pair

```bash
./poly_dep_check check "x^2 + y^2" "x*y"
```

### Run Brute Force Search

```bash
./poly_dep_check brute

./poly_dep_check brute --max-degree-f 3 --max-degree-g 3 --max-degree-q 4 --workers 8

./poly_dep_check brute --resume
```

### View Statistics

```bash
./poly_dep_check stats
```

### Query Results

```bash
./poly_dep_check query

./poly_dep_check query --both-divisible
```

## Mathematical Background

For each pair of polynomials (f, g) ∈ Z[x,y], the program:

1. **Finds dependency**: Searches for polynomial q ∈ Q[x,u,v] such that q(x, f(x,y), g(x,y)) = 0
   - Uses resultant method to eliminate y algebraically
   - Computes Res_y(u - f(x,y), v - g(x,y))
   - Produces polynomial in Q[x,u,v] directly

2. **Checks divisibility**: Verifies if:
   - ∂q/∂u is divisible by ∂q/∂x (where u represents f)
   - ∂q/∂v is divisible by ∂q/∂x (where v represents g)

## Command Reference

### `brute` - Brute Force Mode

Run systematic enumeration of polynomial pairs.

```bash
./poly_dep_check brute [OPTIONS]
```

**Options:**
- `--max-degree-f N`: Maximum degree for polynomial f (default: 2)
- `--max-degree-g N`: Maximum degree for polynomial g (default: 2)
- `--max-degree-q N`: Maximum degree for dependency q (default: 3)
- `--coeff-min N`: Minimum coefficient value (default: -2)
- `--coeff-max N`: Maximum coefficient value (default: 2)
- `--strategy {lexicographic,degree_first}`: Enumeration strategy (default: lexicographic)
- `--checkpoint-interval N`: Save state every N pairs (default: 10)
- `--workers N`: Number of parallel workers (default: 4)
- `--resume`: Resume from last checkpoint

### `check` - Manual Check Mode

Check a specific polynomial pair.

```bash
./poly_dep_check check F G [OPTIONS]
```

**Arguments:**
- `F`: First polynomial (e.g., "x^2 + y^2")
- `G`: Second polynomial (e.g., "x*y")

**Options:**
- `--max-degree-q N`: Maximum degree for dependency q (default: 3)

### `stats` - Statistics

Show summary statistics from the result cache.

```bash
./poly_dep_check stats
```

### `query` - Query Results

Query and display results from the cache.

```bash
./poly_dep_check query [OPTIONS]
```

**Options:**
- `--both-divisible`: Show only pairs where both conditions hold
- `--limit N`: Maximum number of results to show (default: 20)

## Input Format

Polynomials are entered as strings using standard mathematical notation:

- **Variables**: `x` and `y`
- **Exponentiation**: `x^2` or `x**2`
- **Multiplication**: `x*y` or `2*x`
- **Addition/Subtraction**: `x + y`, `x - y`
- **Examples**:
  - `x^2 + y^2`
  - `x*y - 1`
  - `2*x^2 + 3*x*y - y^2`

## Data Storage

The program stores data in the `data/` directory:

- **`data/results.db`**: SQLite database with all results
- **`data/state.json`**: Checkpoint state for brute force

These files are created automatically on first run.

## Project Structure

```
.
├── CMakeLists.txt          # Build configuration
├── include/                # Header files
│   ├── config.h
│   ├── polynomial.h
│   ├── generator.h
│   ├── dependency_finder.h
│   ├── divisibility.h
│   ├── cache.h
│   ├── state.h
│   ├── brute_force.h
│   ├── manual_check.h
│   └── cli.h
├── src/                    # Implementation files
│   ├── main.cpp
│   ├── config.cpp
│   ├── polynomial.cpp
│   ├── generator.cpp
│   ├── dependency_finder.cpp
│   ├── divisibility.cpp
│   ├── cache.cpp
│   ├── state.cpp
│   ├── brute_force.cpp
│   ├── manual_check.cpp
│   └── cli.cpp
└── README_CPP.md          # This file
```

## Performance Considerations

- **Computational Complexity**: Grows exponentially with degree and coefficient range
- **Caching**: Always checks cache before computation
- **Checkpointing**: Saves state regularly to allow resume
- **Parallelization**: Uses multiple threads for batch processing
- **Interruption**: Press Ctrl+C to safely interrupt and save progress

## Differences from Python Version

1. **Performance**: C++ version is significantly faster due to compiled nature
2. **Dependencies**: Uses GiNaC instead of SymPy for symbolic computation
3. **Parallelization**: Thread-based instead of process-based
4. **Memory**: More efficient memory usage
5. **Build**: Requires compilation step

## Troubleshooting

### Build Errors

**"ginac/ginac.h not found"**
```bash
sudo apt-get install libginac-dev
```

**"nlohmann/json.hpp not found"**
```bash
sudo apt-get install nlohmann-json3-dev
```

### Runtime Errors

**"Failed to open database"**

Ensure the `data/` directory is writable:
```bash
mkdir -p data
chmod 755 data
```

### Performance Issues

Reduce the search space:
```bash
./poly_dep_check brute --max-degree-f 1 --max-degree-g 1 --coeff-min -1 --coeff-max 1
```

### Clear Cache

Delete the data directory:
```bash
rm -rf data/
```

## Migration from Python Version

To migrate existing results from the Python version:

1. The database schema is compatible
2. Copy `data/results.db` from Python version
3. State files are not compatible (different format)

## License

[Add your license here]

## Contributing

[Add contribution guidelines here]

## Acknowledgments

- **GiNaC**: Symbolic computation library
- **CLN**: Arbitrary precision arithmetic
- **SQLite**: Embedded database
- **nlohmann/json**: JSON library