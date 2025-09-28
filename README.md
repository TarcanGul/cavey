Cavey JUCE Plugin (CMake)
================================

Minimal cross‑platform JUCE audio plugin using CMake.

Prerequisites
-------------
- CMake 3.21+
- A supported compiler toolchain (Xcode/Clang on macOS, MSVC on Windows, GCC/Clang on Linux)
- JUCE is provided either via FetchContent (default) or a local path

Configure & Build
-----------------
```
# From the repo root
# Option A: Use FetchContent (requires network on first configure)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release

# Option B: Use a local JUCE checkout (offline friendly)
# export JUCE_SOURCE_DIR=/path/to/JUCE
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DJUCE_SOURCE_DIR="$JUCE_SOURCE_DIR"
cmake --build build --config Release
```

This builds VST3, AU (on macOS), and a Standalone app target.

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
- AAX is not enabled by default as it requires the Avid SDK.
- You can adjust plugin formats in CMakeLists.txt under FORMATS.
- The GUI text and size are set in src/PluginEditor.cpp.
