Cavey Plugin
================================

Cross‑platform audio plugin that allows you to generate your own audio effects via prompting!

<img width="624" height="443" alt="Screenshot 2026-05-16 at 3 44 05 PM" src="https://github.com/user-attachments/assets/f9f512f4-c583-41bd-b933-6e8fe660c51e" />

<img width="399" height="282" alt="Screenshot 2026-05-16 at 3 44 13 PM" src="https://github.com/user-attachments/assets/bd7fbdbe-7b1b-4d8e-84c7-346bff1c7ecd" />

<img width="650" height="454" alt="Screenshot 2026-05-16 at 3 44 42 PM" src="https://github.com/user-attachments/assets/e5d2a927-4c4e-4971-8805-e9741d4f9b04" />


I took [this youtube video](https://www.youtube.com/watch?v=X1b7K0RUHos) in the prototype phase.


Prerequisites
-------------
- CMake 3.21+
- Ninja
- A supported compiler toolchain (Xcode/Clang on macOS, MSVC on Windows, GCC/Clang on Linux)
- vcpkg with `VCPKG_ROOT` pointing to your vcpkg checkout
- JUCE is provided either via FetchContent (default) or a local path
- Ollama

The vcpkg manifest pins dependency resolution to a baseline where the Boost ports resolve to 1.89.0.

Configure & Build
-----------------
```
# From the repo root
# Install vcpkg once:
git clone https://github.com/microsoft/vcpkg "$HOME/vcpkg"
export VCPKG_ROOT="$HOME/vcpkg"

# macOS with Homebrew:
brew install ninja

# Use the vcpkg-backed CMake preset. vcpkg installs Boost from vcpkg.json.
cmake --preset release
cmake --build --preset release

# Option B: Use a local JUCE checkout (offline friendly)
# export JUCE_SOURCE_DIR=/path/to/JUCE
cmake --preset release -DJUCE_SOURCE_DIR="$JUCE_SOURCE_DIR"
cmake --build --preset release
```

This builds VST3, AU (on macOS), and a Standalone app target (build/release/CaveyPlugin_artefacts/Release).

Troubleshooting
---------------
If CMake reports that it cannot find `/scripts/buildsystems/vcpkg.cmake`, `VCPKG_ROOT` is not set in your shell. If it reports another missing `vcpkg.cmake` path, `VCPKG_ROOT` is pointing somewhere other than a vcpkg checkout.

```
echo "$VCPKG_ROOT"
test -f "$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
```

Where to find the plugin
------------------------
- macOS
  - VST3: build/CaveyPlugin_artefacts/Release/VST3/Cavey.vst3
  - AU:   build/CaveyPlugin_artefacts/Release/AU/Cavey.component
  - Standalone: build/CaveyPlugin_artefacts/Release/Standalone/Cavey.app
- Windows
  - VST3: build/CaveyPlugin_artefacts/Release/VST3/Cavey.vst3
  - Standalone: build/CaveyPlugin_artefacts/Release/Standalone/Cavey.exe
- Linux
  - VST3: build/CaveyPlugin_artefacts/Release/VST3/Cavey.vst3
  - Standalone: build/CaveyPlugin_artefacts/Release/Standalone/Cavey

Notes
-----
- Contributing guide: [link](CONTRIBUTING.md)
