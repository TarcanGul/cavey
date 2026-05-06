cmake_minimum_required(VERSION 3.21)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(CaveyPlugin VERSION 0.1.0)
add_compile_options(-Wall -Wextra -Wpedantic)

option(CAVEY_ENABLE_COVERAGE "Enable LLVM coverage reporting for CaveyTests" OFF)

# Dependencies

include(FetchContent)

if(DEFINED JUCE_SOURCE_DIR AND EXISTS "${JUCE_SOURCE_DIR}/CMakeLists.txt")
    message(STATUS "Using JUCE from local path: ${JUCE_SOURCE_DIR}")
    add_subdirectory("${JUCE_SOURCE_DIR}" juce)
else()
    set(FETCHCONTENT_QUIET OFF)
    FetchContent_Declare(
      juce
      GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
      GIT_TAG 8.0.12
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
    src/components/LoadingComponent.cpp
    src/components/AiSetupComponent.cpp
    src/controllers/LLMController.cpp
    src/controllers/HttpTransport.cpp
    src/controllers/CredentialStore.cpp
    src/controllers/ProviderResponseParser.cpp
    src/controllers/OpenAIController.cpp
    src/controllers/AnthropicController.cpp
    src/controllers/OllamaController.cpp
    src/effects/CaveyEffects.cpp

    src/controllers/SystemPrompt.md
)

juce_add_binary_data(CaveyAssets SOURCES src/controllers/SystemPrompt.md)
target_link_libraries(CaveyPlugin PRIVATE CaveyAssets)

set_source_files_properties(src/controllers/SystemPrompt.md PROPERTIES MACOSX_PACKAGE_LOCATION Resources)

target_compile_features(CaveyPlugin PRIVATE cxx_std_23)

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

if(APPLE)
    target_link_libraries(CaveyPlugin PRIVATE "-framework Security")
endif()

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.10.0
)
FetchContent_MakeAvailable(Catch2)

enable_testing()

add_executable(CaveyTests
    test/effects/CaveyEffectsTests.cpp
    test/types/BackendParameterTests.cpp
    test/controllers/ProviderControllerTests.cpp
    test/PluginProcessorTests.cpp
    src/PluginProcessor.cpp
    src/PluginEditor.cpp
    src/types/BackendParameter.cpp
    src/components/Parameter.cpp
    src/components/LoadingComponent.cpp
    src/components/AiSetupComponent.cpp
    src/controllers/LLMController.cpp
    src/controllers/HttpTransport.cpp
    src/controllers/CredentialStore.cpp
    src/controllers/ProviderResponseParser.cpp
    src/controllers/OpenAIController.cpp
    src/controllers/AnthropicController.cpp
    src/controllers/OllamaController.cpp
    src/effects/CaveyEffects.cpp
)

target_include_directories(CaveyTests
    PRIVATE
        src
        test
        "${CMAKE_CURRENT_BINARY_DIR}/CaveyPlugin_artefacts/JuceLibraryCode"
)
target_compile_features(CaveyTests PRIVATE cxx_std_23)

target_compile_definitions(CaveyTests
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(CaveyTests
    PRIVATE
        Catch2::Catch2WithMain
        CaveyAssets
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

if(APPLE)
    target_link_libraries(CaveyTests PRIVATE "-framework Security")
endif()

if(CAVEY_ENABLE_COVERAGE)
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "^(AppleClang|Clang)$")
        message(FATAL_ERROR "CAVEY_ENABLE_COVERAGE requires Clang or AppleClang.")
    endif()

    find_program(LLVM_PROFDATA_EXECUTABLE llvm-profdata)
    find_program(LLVM_COV_EXECUTABLE llvm-cov)

    if(NOT LLVM_PROFDATA_EXECUTABLE OR NOT LLVM_COV_EXECUTABLE)
        find_program(XCRUN_EXECUTABLE xcrun REQUIRED)

        if(NOT LLVM_PROFDATA_EXECUTABLE)
            execute_process(
                COMMAND ${XCRUN_EXECUTABLE} --find llvm-profdata
                OUTPUT_VARIABLE LLVM_PROFDATA_EXECUTABLE
                OUTPUT_STRIP_TRAILING_WHITESPACE
                COMMAND_ERROR_IS_FATAL ANY
            )
        endif()

        if(NOT LLVM_COV_EXECUTABLE)
            execute_process(
                COMMAND ${XCRUN_EXECUTABLE} --find llvm-cov
                OUTPUT_VARIABLE LLVM_COV_EXECUTABLE
                OUTPUT_STRIP_TRAILING_WHITESPACE
                COMMAND_ERROR_IS_FATAL ANY
            )
        endif()
    endif()

    set(CAVEY_COVERAGE_FLAGS -fprofile-instr-generate -fcoverage-mapping -O0 -g)
    set(CAVEY_COVERAGE_DIR "${CMAKE_BINARY_DIR}/coverage")
    set(CAVEY_COVERAGE_RAW_FILE "${CAVEY_COVERAGE_DIR}/cavey.profraw")
    set(CAVEY_COVERAGE_PROFILE "${CAVEY_COVERAGE_DIR}/cavey.profdata")
    set(CAVEY_COVERAGE_HTML_DIR "${CAVEY_COVERAGE_DIR}/html")
    set(CAVEY_COVERAGE_IGNORE_REGEX
        "(_deps|/test/|JuceLibraryCode|BinaryData|CMakeFiles|/build[^/]*)")

    target_compile_options(CaveyTests PRIVATE ${CAVEY_COVERAGE_FLAGS})
    target_link_options(CaveyTests PRIVATE -fprofile-instr-generate -fcoverage-mapping)

    add_custom_target(coverage
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CAVEY_COVERAGE_HTML_DIR}"
        COMMAND ${CMAKE_COMMAND} -E rm -f "${CAVEY_COVERAGE_RAW_FILE}" "${CAVEY_COVERAGE_PROFILE}"
        COMMAND ${CMAKE_COMMAND} -E env
            "LLVM_PROFILE_FILE=${CAVEY_COVERAGE_RAW_FILE}"
            "$<TARGET_FILE:CaveyTests>"
        COMMAND ${LLVM_PROFDATA_EXECUTABLE} merge -sparse
            "${CAVEY_COVERAGE_RAW_FILE}"
            -o "${CAVEY_COVERAGE_PROFILE}"
        COMMAND ${LLVM_COV_EXECUTABLE} report
            "$<TARGET_FILE:CaveyTests>"
            "-instr-profile=${CAVEY_COVERAGE_PROFILE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/src"
            "-ignore-filename-regex=${CAVEY_COVERAGE_IGNORE_REGEX}"
        COMMAND ${LLVM_COV_EXECUTABLE} show
            "$<TARGET_FILE:CaveyTests>"
            "-instr-profile=${CAVEY_COVERAGE_PROFILE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/src"
            "-ignore-filename-regex=${CAVEY_COVERAGE_IGNORE_REGEX}"
            -format=html
            "-output-dir=${CAVEY_COVERAGE_HTML_DIR}"
        DEPENDS CaveyTests
        COMMENT "Running CaveyTests and generating LLVM coverage report"
        VERBATIM
    )
endif()

include(Catch)
catch_discover_tests(CaveyTests)

# On macOS, you can build universal binaries by uncommenting below
# set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
