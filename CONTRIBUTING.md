## General rules

- Please open an issue before starting to work on a feature for tracking purposes.
- Feel free to make any feature requests under the issues tab.

## Development Flow

- I like to use the AudioPluginHost JUCE plugin that comes bundled with JUCE. This allows fast iteration.

## Logs

- The logs are written in ~/Library/Logs/Cavey/cavey.log (in Mac). For other OS it will be the default location picked by 
the JUCE framework.

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

