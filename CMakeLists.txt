cmake_minimum_required(VERSION 3.22)
project(YanoSwing VERSION 0.1.0)

add_subdirectory(libs/JUCE)

if(APPLE)
    set(YANOSWING_FORMATS VST3 AU Standalone)
else()
    set(YANOSWING_FORMATS VST3 Standalone)
endif()

juce_add_plugin(YanoSwing
    COMPANY_NAME "Bumpin Audio"
    BUNDLE_ID "com.bumpinaudio.yanoswing"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT TRUE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE Nabs
    PLUGIN_CODE YnSw
    FORMATS ${YANOSWING_FORMATS}
    PRODUCT_NAME "Yano Swing")

target_sources(YanoSwing PRIVATE
    Source/YanoSwingProcessor.cpp
    Source/PluginEditor.cpp
    Source/PluginEntry.cpp)

target_compile_definitions(YanoSwing PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(YanoSwing
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# Dev-only headless MIDI grid test (not shipped) -- see Source/AudioTestTool.cpp
juce_add_console_app(AudioTestTool PRODUCT_NAME "AudioTestTool")

target_sources(AudioTestTool PRIVATE
    Source/YanoSwingProcessor.cpp
    Source/PluginEditor.cpp
    Source/AudioTestTool.cpp)

target_compile_definitions(AudioTestTool PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_STANDALONE_APPLICATION=1)

target_link_libraries(AudioTestTool
    PRIVATE
        juce::juce_audio_formats
        juce::juce_audio_utils
        juce::juce_gui_basics
        juce::juce_audio_processors
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags)

# Dev-only headless UI snapshot tool (not shipped) -- see Source/SnapshotTool.cpp
juce_add_console_app(SnapshotTool PRODUCT_NAME "SnapshotTool")

target_sources(SnapshotTool PRIVATE
    Source/YanoSwingProcessor.cpp
    Source/PluginEditor.cpp
    Source/SnapshotTool.cpp)

target_compile_definitions(SnapshotTool PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_STANDALONE_APPLICATION=1)

target_link_libraries(SnapshotTool
    PRIVATE
        juce::juce_audio_utils
        juce::juce_gui_basics
        juce::juce_audio_processors
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags)

# Dev-only listening aid, not part of the shipped repo -- renders a click
# pattern (straight vs swung) so the groove can be auditioned as audio.
juce_add_console_app(RenderTool PRODUCT_NAME "RenderTool")

target_sources(RenderTool PRIVATE
    Source/YanoSwingProcessor.cpp
    Source/PluginEditor.cpp
    Source/RenderTool.cpp)

target_compile_definitions(RenderTool PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_STANDALONE_APPLICATION=1)

target_link_libraries(RenderTool
    PRIVATE
        juce::juce_audio_formats
        juce::juce_audio_utils
        juce::juce_gui_basics
        juce::juce_audio_processors
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags)
