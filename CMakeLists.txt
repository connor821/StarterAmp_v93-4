cmake_minimum_required(VERSION 3.22)
# Windows: link the C++ runtime statically so friends do not need to install anything extra.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
project(StarterAmpV93 VERSION 0.93.0)

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

juce_add_plugin(StarterAmpV82
    COMPANY_NAME "YourName"
    PLUGIN_MANUFACTURER_CODE Yrnm
    PLUGIN_CODE SamM
    FORMATS ${STARTERAMP_FORMATS}
    PRODUCT_NAME "Starter Amp v93"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT "Starter Amp needs audio input to process your guitar.")

target_sources(StarterAmpV82 PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/CabRealism.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/IrLibrary.h
    Source/IrLibraryPanel.h)

# Smaller binary: the plugin only reads WAV / AIFF impulse responses, so the other audio
# codecs JUCE can include are switched off.
target_compile_definitions(StarterAmpV82 PUBLIC
    JUCE_WEB_BROWSER=0
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
target_compile_options(StarterAmpV82 PRIVATE
    $<$<CONFIG:Release>:-O3>
    $<$<CONFIG:Release>:-ffunction-sections>
    $<$<CONFIG:Release>:-fdata-sections>
    $<$<CONFIG:Release>:-fvisibility-inlines-hidden>)
endif()
if(APPLE)
    # -x drops local symbol names from the binary (smaller file, nothing audible changes).
    target_link_options(StarterAmpV82 PRIVATE $<$<CONFIG:Release>:-Wl,-dead_strip> $<$<CONFIG:Release>:-Wl,-x>)
endif()
# The interface code is not time critical, so build it for size instead of speed.
# (Xcode cannot do per-config per-file options, so this is skipped for that generator.)
if(NOT CMAKE_GENERATOR STREQUAL "Xcode")
    set_source_files_properties(Source/PluginEditor.cpp PROPERTIES COMPILE_OPTIONS "$<$<CONFIG:Release>:-Os>")
endif()

target_link_libraries(StarterAmpV82
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
