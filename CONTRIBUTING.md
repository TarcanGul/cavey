## Running Tests

### To build the tests (once)
```bash
cmake --build build --target CaveyTests
```

### To run the tests
```bash
ctest --test-dir build --output-on-failure
```

### To run the tests with coverage
```bash
cmake -S . -B build-coverage -DCAVEY_ENABLE_COVERAGE=ON
cmake --build build-coverage --target coverage
```
The coverage target runs CaveyTests, prints the LLVM coverage summary in the terminal, and writes the HTML report to:
build-coverage/coverage/html/index.html