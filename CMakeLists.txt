cmake_minimum_required(VERSION 3.22)

project(Bassurda VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# JUCE: use a local checkout if JUCE_DIR is given, otherwise download it.
# JUCE 7 is licensed under GPLv3, which matches this project's license.
# ---------------------------------------------------------------------------
set(JUCE_DIR "" CACHE PATH "Path to a local JUCE checkout (optional)")

if(JUCE_DIR)
    add_subdirectory(${JUCE_DIR} ${CMAKE_BINARY_DIR}/JUCE EXCLUDE_FROM_ALL)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        7.0.12
        GIT_SHALLOW    ON)
    FetchContent_MakeAvailable(JUCE)
endif()

option(BASSURDA_BUILD_TESTS "Build the offline DSP test runner" ON)

# Target: Windows VST3.
set(BASSURDA_FORMATS VST3)

juce_add_plugin(Bassurda
    COMPANY_NAME                "VoyagerSquare"
    COMPANY_WEBSITE             "https://github.com/voyagersquare/Bassurda"
    BUNDLE_ID                   "com.voyagersquare.bassurda"
    PLUGIN_MANUFACTURER_CODE    Vysq
    PLUGIN_CODE                 Bsrd
    IS_SYNTH                    TRUE
    NEEDS_MIDI_INPUT            TRUE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     FALSE
    VST3_CATEGORIES             Instrument Synth
    PRODUCT_NAME                "Bassurda"
    FORMATS                     ${BASSURDA_FORMATS})

set(BASSURDA_CORE_SOURCES
    Source/Parameters.cpp
    Source/Presets.cpp
    Source/DSP/BassEngine.cpp
    Source/PluginProcessor.cpp)

set(BASSURDA_GUI_SOURCES
    Source/GUI/BassurdaLookAndFeel.cpp
    Source/GUI/Displays.cpp
    Source/GUI/Modules.cpp
    Source/PluginEditor.cpp)

target_sources(Bassurda PRIVATE ${BASSURDA_CORE_SOURCES} ${BASSURDA_GUI_SOURCES})
target_include_directories(Bassurda PRIVATE Source)

target_compile_definitions(Bassurda PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(Bassurda
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# ---------------------------------------------------------------------------
# Offline test runner: renders notes through the engine and checks the
# behaviour that matters for a bass synth (phase retrigger, glide, no NaNs,
# clean sub, latency compensation). Also renders a GUI snapshot.
# ---------------------------------------------------------------------------
if(BASSURDA_BUILD_TESTS)
    juce_add_console_app(BassurdaTests PRODUCT_NAME "BassurdaTests")
    target_sources(BassurdaTests PRIVATE
        Tests/TestMain.cpp
        ${BASSURDA_CORE_SOURCES}
        ${BASSURDA_GUI_SOURCES})
    target_include_directories(BassurdaTests PRIVATE Source)
    target_compile_definitions(BassurdaTests PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_STANDALONE_APPLICATION=1
        JUCE_MODAL_LOOPS_PERMITTED=1
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JucePlugin_Name="Bassurda"
        JucePlugin_IsSynth=1
        JucePlugin_WantsMidiInput=1
        JucePlugin_ProducesMidiOutput=0
        JucePlugin_IsMidiEffect=0)
    target_link_libraries(BassurdaTests PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)
endif()
