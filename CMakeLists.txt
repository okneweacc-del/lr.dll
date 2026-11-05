cmake_minimum_required(VERSION 3.22)
project(TKVocalStudio VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# JUCE: đặt thư mục JUCE cạnh project (extern/JUCE) hoặc truyền -DJUCE_DIR=...
# build.bat sẽ tự tải JUCE nếu chưa có.
set(JUCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/extern/JUCE" CACHE PATH "Đường dẫn JUCE")
add_subdirectory(${JUCE_DIR} JUCE EXCLUDE_FROM_ALL)

juce_add_plugin(TKVocalStudio
    COMPANY_NAME "TK Audio"
    PLUGIN_MANUFACTURER_CODE TKau
    PLUGIN_CODE TkVs
    FORMATS VST3 Standalone
    PRODUCT_NAME "TK VOCAL STUDIO"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    VST3_CATEGORIES Fx
    COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(TKVocalStudio PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(TKVocalStudio PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(TKVocalStudio PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags)
