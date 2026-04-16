# Polynomial Dependency Checker

A C++ tool for finding algebraic dependencies between polynomials. The tool searches for polynomials Q such that Q(f, g) = 0 for given polynomials f and g.

## Features

- Brute force search for polynomial dependencies
- Checkpoint and resume support for long-running computations
- Multi-threaded processing
- SQLite database for caching results
- Manual checking of specific polynomial pairs
- Configurable polynomial degrees and coefficient ranges

## Dependencies

- CMake 4 or higher
- C++23 compiler (GCC 15 recommended)
- GiNaC library
- CLN library
- SQLite3
- nlohmann_json
- GTest (for tests)

## Building

```bash
mkdir build
cd build
cmake ..
make
```

## Usage

### Brute Force Search

Search for dependencies across all polynomial combinations:

```bash
./run.sh brute-force
```

### Manual Check

Check a specific pair of polynomials:

```bash
./run.sh check "x^2 + y^2" "x*y"
```

### Configuration Options

- `--max-degree-f`: Maximum degree for polynomial f (default: 2)
- `--max-degree-g`: Maximum degree for polynomial g (default: 2)
- `--max-degree-q`: Maximum degree for polynomial Q (default: 3)
- `--coeff-min`: Minimum coefficient value (default: -2)
- `--coeff-max`: Maximum coefficient value (default: 2)
- `--num-workers`: Number of worker threads (default: 0, auto-detect)
- `--cache-file`: Path to cache database (default: data/results.db)
- `--state-file`: Path to state file for resume (default: data/state.json)

### Resume from Checkpoint

Resume a previously interrupted brute force search:

```bash
./run_with_resume.sh
```

### Run Until Complete

Run brute force search until all combinations are checked:

```bash
./run_until_complete.sh
```

## Project Structure

- `include/`: Header files
- `src/`: Source files
- `tests/`: Unit tests
- `examples/`: Example scripts
- `data/`: Runtime data directory (created automatically)

## Key Components

- `DependencyFinder`: Main class for finding polynomial dependencies
- `Generator`: Enumerates polynomial combinations
- `Cache`: SQLite-based result caching
- `State`: Checkpoint and resume functionality
- `WorkerPool`: Multi-threaded processing

## Testing

Run unit tests:

```bash
cd build
ctest
```
