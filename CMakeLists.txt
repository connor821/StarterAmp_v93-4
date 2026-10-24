cmake_minimum_required(VERSION 3.22)
# Windows: link the C++ runtime statically so friends do not need to install anything extra.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
# The one place the version lives. Edit this line (0.1.0, 0.1.1, ...); the project version, the file names and the label in the
# plug-in window all follow. A trailing .0 is not shown (0.1.0 shows as v0.1).
set(STARTERAMP_VERSION 0.1.0)
project(StarterAmp VERSION ${STARTERAMP_VERSION})
string(REGEX REPLACE "\\.0$" "" STARTERAMP_VERSION_SHORT "${STARTERAMP_VERSION}")
set(STARTERAMP_VERSION_LABEL "v${STARTERAMP_VERSION_SHORT}")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Downloads JUCE automatically the first time you configure (needs internet).
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

# AU only exists on macOS.
if(APPLE)
    set(STARTERAMP_FORMATS Standalone VST3 AU)
else()
    set(STARTERAMP_FORMATS Standalone VST3)
endif()

juce_add_plugin(StarterAmp
    COMPANY_NAME "Starter Amp"
    BUNDLE_ID com.starteramp.StarterAmp        # JUCE builds one from the company and product names otherwise, and spaces in it make CMake warn
    PLUGIN_MANUFACTURER_CODE Stam
    PLUGIN_CODE SamM
    FORMATS ${STARTERAMP_FORMATS}
    PRODUCT_NAME "Starter Amp ${STARTERAMP_VERSION_LABEL}"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT "Starter Amp needs audio input to process your guitar.")

target_sources(StarterAmp PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/CabRealism.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/IrLibrary.h
    Source/IrLibraryPanel.h)

# Smaller binary: the plugin only reads WAV / AIFF impulse responses, so the other audio
# codecs JUCE can include are switched off.
target_compile_definitions(StarterAmp PUBLIC
    JUCE_WEB_BROWSER=0
    STARTERAMP_VERSION_LABEL="${STARTERAMP_VERSION_LABEL}"
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_USE_FLAC=0
    JUCE_USE_OGGVORBIS=0
    JUCE_USE_MP3AUDIOFORMAT=0
    JUCE_USE_LAME_AUDIO_FORMAT=0
    JUCE_USE_WINDOWS_MEDIA_FORMAT=0
    JUCE_DISABLE_JUCE_VERSION_PRINTING=1)

# Release builds: optimise for speed, drop unused code and symbols.
if(NOT MSVC)
target_compile_options(StarterAmp PRIVATE
    $<$<CONFIG:Release>:-O3>
    $<$<CONFIG:Release>:-ffunction-sections>
    $<$<CONFIG:Release>:-fdata-sections>
    $<$<CONFIG:Release>:-fvisibility-inlines-hidden>)
endif()
if(APPLE)
    # -x drops local symbol names from the binary (smaller file, nothing audible changes).
    target_link_options(StarterAmp PRIVATE $<$<CONFIG:Release>:-Wl,-dead_strip> $<$<CONFIG:Release>:-Wl,-x>)
endif()
# The interface code is not time critical, so build it for size instead of speed.
# (Xcode cannot do per-config per-file options, so this is skipped for that generator.)
if(NOT CMAKE_GENERATOR STREQUAL "Xcode")
    set_source_files_properties(Source/PluginEditor.cpp PROPERTIES COMPILE_OPTIONS "$<$<CONFIG:Release>:-Os>")
endif()

target_link_libraries(StarterAmp
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# Optional automated tests (stability, block sizes, automation, save/load, lifecycle, regression render).
#   cmake -B build -DSTARTERAMP_BUILD_TESTS=ON && cmake --build build --target StarterAmpTests --config Release
# then run the StarterAmpTests program from the Tests folder:  StarterAmpTests stress   and   StarterAmpTests regress x golden.txt
option(STARTERAMP_BUILD_TESTS "Build the automated test program" OFF)
if(STARTERAMP_BUILD_TESTS)
    juce_add_console_app(StarterAmpTests PRODUCT_NAME "StarterAmpTests")
    target_sources(StarterAmpTests PRIVATE
        Tests/tests.cpp
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp)
    target_include_directories(StarterAmpTests PRIVATE Source)
    target_compile_definitions(StarterAmpTests PUBLIC
        JUCE_WEB_BROWSER=0
        STARTERAMP_VERSION_LABEL="${STARTERAMP_VERSION_LABEL}"
        JUCE_USE_CURL=0
        JUCE_USE_FLAC=0
        JUCE_USE_OGGVORBIS=0
        JUCE_USE_MP3AUDIOFORMAT=0
        JUCE_USE_LAME_AUDIO_FORMAT=0
        JUCE_USE_WINDOWS_MEDIA_FORMAT=0
        JUCE_DISABLE_JUCE_VERSION_PRINTING=1)
    target_link_libraries(StarterAmpTests
        PRIVATE
            juce::juce_audio_utils
            juce::juce_dsp
        PUBLIC
            juce::juce_recommended_config_flags)
endif()
