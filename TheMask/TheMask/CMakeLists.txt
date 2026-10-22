cmake_minimum_required(VERSION 3.22)

project(TheMask VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --------------------------------------------------------------------------
# JUCE. Either point THEMASK_JUCE_DIR at a local JUCE checkout, or let CMake
# download a pinned release (needs git and internet access the first time).
# --------------------------------------------------------------------------
set(THEMASK_JUCE_DIR "" CACHE PATH "Optional path to an existing JUCE checkout")

if(THEMASK_JUCE_DIR)
    add_subdirectory(${THEMASK_JUCE_DIR} juce EXCLUDE_FROM_ALL)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.6
        GIT_SHALLOW    ON)
    FetchContent_MakeAvailable(JUCE)
endif()

# --------------------------------------------------------------------------
# The plugin
# --------------------------------------------------------------------------
juce_add_plugin(TheMask
    COMPANY_NAME                "TheMaskAudio"
    PLUGIN_MANUFACTURER_CODE    Tmsk
    PLUGIN_CODE                 Mask
    PRODUCT_NAME                "The Mask"
    FORMATS                     VST3 Standalone
    VST3_CATEGORIES             Fx Filter
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    COPY_PLUGIN_AFTER_BUILD     FALSE
    BUNDLE_ID                   com.themaskaudio.themask)

target_sources(TheMask PRIVATE
    Source/MaskEngine.cpp
    Source/Limiter.cpp
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_include_directories(TheMask PRIVATE Source)

target_compile_definitions(TheMask PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(TheMask
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

# --------------------------------------------------------------------------
# DSP unit tests (no JUCE needed):  cmake --build build --target TheMaskEngineTests
# --------------------------------------------------------------------------
add_executable(TheMaskEngineTests EXCLUDE_FROM_ALL
    Tests/test_engine.cpp
    Source/MaskEngine.cpp)
target_include_directories(TheMaskEngineTests PRIVATE Source)
