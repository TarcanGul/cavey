cmake_minimum_required(VERSION 3.21)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(CaveyPlugin VERSION 0.1.0)
add_compile_options(-Wall -Wextra -Wpedantic)

# Dependencies

if(DEFINED JUCE_SOURCE_DIR AND EXISTS "${JUCE_SOURCE_DIR}/CMakeLists.txt")
    message(STATUS "Using JUCE from local path: ${JUCE_SOURCE_DIR}")
    add_subdirectory("${JUCE_SOURCE_DIR}" juce)
else()
    include(FetchContent)
    set(FETCHCONTENT_QUIET OFF)
    FetchContent_Declare(
      juce
      GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
      GIT_TAG 7.0.12
    )
    FetchContent_MakeAvailable(juce)
endif()

find_package(Boost 1.89 REQUIRED COMPONENTS json)

# Create the plugin target
juce_add_plugin(CaveyPlugin
    COMPANY_NAME "TarcanGul"
    BUNDLE_ID com.tarcangul.Cavey
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE Cave  # 4-char code
    PLUGIN_CODE Cave               # 4-char code
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "Cavey"
)

juce_generate_juce_header(CaveyPlugin)

target_sources(CaveyPlugin PRIVATE
    src/PluginProcessor.cpp
    src/PluginEditor.cpp
    src/types/BackendParameter.cpp
    src/components/Parameter.cpp
    src/controllers/OllamaController.cpp
)

target_compile_features(CaveyPlugin PRIVATE cxx_std_17)

# Recommended flags to reduce bloat and disable unused webviews, etc.
target_compile_definitions(CaveyPlugin
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(CaveyPlugin
    PRIVATE
        Boost::headers
        Boost::json
        juce::juce_audio_utils
        juce::juce_audio_plugin_client
        juce::juce_gui_extra
        juce::juce_dsp
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_events
        juce::juce_data_structures
        juce::juce_core
)

# On macOS, you can build universal binaries by uncommenting below
# set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
