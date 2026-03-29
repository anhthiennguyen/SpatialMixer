cmake_minimum_required(VERSION 3.22)
project(SpatialMixer VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Point to local JUCE
add_subdirectory($ENV{HOME}/JUCE JUCE_build)

juce_add_plugin(SpatialMixer
    COMPANY_NAME "SpatialAudio"
    PLUGIN_MANUFACTURER_CODE Spat
    PLUGIN_CODE Spmx
    FORMATS VST3 AU
    PRODUCT_NAME "SpatialMixer"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VST3_CATEGORIES "Fx" "EQ" "Dynamics"
    AU_MAIN_TYPE kAudioUnitType_Effect
    COPY_PLUGIN_AFTER_BUILD TRUE
)

target_sources(SpatialMixer PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/TrackState.cpp
    Source/CanvasComponent.cpp
    Source/DSPEngine.cpp
)

target_compile_definitions(SpatialMixer PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(SpatialMixer PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_plugin_client
    juce::juce_dsp
    juce::juce_gui_basics
    juce::juce_graphics
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)
