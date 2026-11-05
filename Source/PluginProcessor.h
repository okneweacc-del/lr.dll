#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "Params.h"
#include "DSP.h"

class TKVocalStudioProcessor : public juce::AudioProcessor
{
public:
    TKVocalStudioProcessor();
    ~TKVocalStudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "TK VOCAL STUDIO"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.5; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentPreset; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // --- Preset / EQ preset / A-B ---
    static juce::StringArray getPresetNames();
    static juce::StringArray getEqPresetNames();
    void applyPreset (int index);
    void applyEqPreset (int index);
    void resetEq();
    void toggleAB();
    bool isB() const { return slotIsB; }
    void copyAtoB();

    // --- dữ liệu cho UI ---
    juce::UndoManager undoManager;
    TK::APVTS apvts;
    TK::Stats stats;
    static constexpr int ringSize = 8192;
    std::array<float, ringSize> ringL {}, ringR {};
    std::atomic<int> ringPos { 0 };
    double currentSampleRate = 48000.0;
    TKdsp::Settings getSettingsSnapshot() const { return settings; }

private:
    struct Bind { std::atomic<float>* src; void* dst; int kind; };
    std::vector<Bind> binds;
    void bindF (const juce::String& id, float* d) { binds.push_back ({ apvts.getRawParameterValue (id), d, 0 }); }
    void bindB (const juce::String& id, bool* d) { binds.push_back ({ apvts.getRawParameterValue (id), d, 1 }); }
    void bindI (const juce::String& id, int* d) { binds.push_back ({ apvts.getRawParameterValue (id), d, 2 }); }
    void setParam (const juce::String& id, float realValue);
    void setAllDefaults (bool eqOnly);

    TKdsp::Settings settings;
    TKdsp::VocalChain chain;
    int currentPreset = 0;
    bool slotIsB = false;
    juce::ValueTree slotState[2];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TKVocalStudioProcessor)
};
