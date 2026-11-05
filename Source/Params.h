#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

namespace TK
{
using APVTS = juce::AudioProcessorValueTreeState;

// ---------- EQ ----------
constexpr int numBands = 7;
inline const char* bandNames[numBands] = { "Sub Bass", "Bass", "Low Mid", "Mid", "Upper Mid", "Presence", "Air" };
inline const float bandFreqMin[numBands] = { 20.f, 60.f, 250.f, 500.f, 2000.f, 4000.f, 8000.f };
inline const float bandFreqMax[numBands] = { 60.f, 250.f, 500.f, 2000.f, 4000.f, 8000.f, 20000.f };
inline const float bandFreqDef[numBands] = { 40.f, 120.f, 350.f, 1000.f, 3000.f, 6000.f, 12000.f };
// 0 Bell, 1 Low Shelf, 2 High Shelf, 3 Notch
inline const int bandTypeDef[numBands] = { 1, 0, 0, 0, 0, 0, 2 };

inline juce::String bandId (int b, const char* what) { return "eq" + juce::String (b) + "_" + what; }

struct Stats
{
    // meters / analyser, được ghi từ audio thread, đọc từ UI thread
    std::atomic<float> inPeakL { 0 }, inPeakR { 0 }, outPeakL { 0 }, outPeakR { 0 };
    std::atomic<float> loudness { -100 };    // LUFS-ish (K-weighted gần đúng, short-term 3s)
    std::atomic<float> correlation { 1 };
    std::atomic<float> monoLossDb { 0 };     // mức mất năng lượng khi gộp mono
    std::atomic<float> grComp { 0 }, grLimiter { 0 }, grDeEss { 0 };
};

inline APVTS::ParameterLayout createLayout()
{
    using FP = juce::AudioParameterFloat;
    using BP = juce::AudioParameterBool;
    using CP = juce::AudioParameterChoice;
    using NR = juce::NormalisableRange<float>;
    APVTS::ParameterLayout l;

    auto fp = [&] (const juce::String& id, const juce::String& name, float lo, float hi, float def)
    {
        l.add (std::make_unique<FP> (juce::ParameterID { id, 1 }, name, NR (lo, hi), def));
    };
    auto fpLog = [&] (const juce::String& id, const juce::String& name, float lo, float hi, float def)
    {
        NR r (lo, hi, 0.f); r.setSkewForCentre (std::sqrt (lo * hi));
        l.add (std::make_unique<FP> (juce::ParameterID { id, 1 }, name, r, def));
    };
    auto bp = [&] (const juce::String& id, const juce::String& name, bool def)
    { l.add (std::make_unique<BP> (juce::ParameterID { id, 1 }, name, def)); };

    // ----- Global -----
    fp ("inGain", "Input Gain", -24.f, 24.f, 0.f);
    fp ("outGain", "Output Gain", -24.f, 24.f, 0.f);
    bp ("bypass", "Bypass", false);
    bp ("levelMatch", "Level Match", false);

    // ----- Stereo -----
    fp ("leftGain", "Left Gain", -24.f, 12.f, 0.f);
    fp ("rightGain", "Right Gain", -24.f, 12.f, 0.f);
    fp ("balance", "Balance", -1.f, 1.f, 0.f);
    fp ("width", "Width", 0.f, 2.f, 1.f);
    fp ("midGain", "Mid Gain", -12.f, 12.f, 0.f);
    fp ("sideGain", "Side Gain", -12.f, 12.f, 0.f);
    fp ("separation", "Separation", 0.f, 1.f, 0.f);
    bp ("monoCheck", "Mono Check", false);
    bp ("phaseInvL", "Invert L", false);
    bp ("phaseInvR", "Invert R", false);

    // ----- EQ -----
    bp ("eqOn", "EQ On", true);
    bp ("hpfOn", "HPF On", true);
    fpLog ("hpfFreq", "HPF Freq", 20.f, 400.f, 80.f);
    bp ("lpfOn", "LPF On", false);
    fpLog ("lpfFreq", "LPF Freq", 3000.f, 20000.f, 20000.f);
    for (int b = 0; b < numBands; ++b)
    {
        juce::String n (bandNames[b]);
        bp (bandId (b, "on"), n + " On", true);
        fpLog (bandId (b, "freq"), n + " Freq", bandFreqMin[b], bandFreqMax[b], bandFreqDef[b]);
        fp (bandId (b, "gain"), n + " Gain", -15.f, 15.f, 0.f);
        fpLog (bandId (b, "q"), n + " Q", 0.2f, 10.f, 0.9f);
        l.add (std::make_unique<CP> (juce::ParameterID { bandId (b, "type"), 1 }, n + " Type",
                                     juce::StringArray { "Bell", "Low Shelf", "High Shelf", "Notch" }, bandTypeDef[b]));
    }

    // ----- Tone modules -----
    for (auto m : { "clarity", "presence", "warmth", "air" })
    {
        bp (juce::String (m) + "_on", juce::String (m) + " On", false);
        fp (juce::String (m) + "_amt", juce::String (m) + " Amount", 0.f, 1.f, 0.4f);
    }

    // ----- De-esser -----
    bp ("deess_on", "De-esser On", false);
    fpLog ("deess_freq", "De-esser Freq", 3000.f, 10000.f, 6500.f);
    fp ("deess_thr", "De-esser Threshold", -50.f, 0.f, -28.f);
    fp ("deess_ratio", "De-esser Ratio", 1.5f, 10.f, 4.f);

    // ----- Noise reduction (expander/gate downward) -----
    bp ("nr_on", "Noise Reduction On", false);
    fp ("nr_thr", "NR Threshold", -80.f, -20.f, -55.f);
    fp ("nr_range", "NR Reduction", 0.f, 30.f, 10.f);
    fp ("nr_release", "NR Release", 20.f, 600.f, 180.f);

    // ----- Compressor -----
    bp ("comp_on", "Compressor On", false);
    fp ("comp_thr", "Comp Threshold", -40.f, 0.f, -18.f);
    fp ("comp_ratio", "Comp Ratio", 1.f, 10.f, 2.5f);
    fp ("comp_att", "Comp Attack", 0.5f, 100.f, 15.f);
    fp ("comp_rel", "Comp Release", 20.f, 800.f, 160.f);
    fp ("comp_knee", "Comp Knee", 0.f, 18.f, 8.f);
    fp ("comp_makeup", "Comp Makeup", 0.f, 18.f, 0.f);
    bp ("comp_autoMakeup", "Comp Auto Makeup", false);

    // ----- Multiband -----
    bp ("mb_on", "Multiband On", false);
    fpLog ("mb_x1", "MB Crossover 1", 100.f, 800.f, 250.f);
    fpLog ("mb_x2", "MB Crossover 2", 1500.f, 8000.f, 3500.f);
    fp ("mb_thr", "MB Threshold", -40.f, 0.f, -20.f);
    fp ("mb_ratio", "MB Ratio", 1.f, 6.f, 2.f);
    fp ("mb_lowGain", "MB Low Makeup", -6.f, 12.f, 0.f);
    fp ("mb_midGain", "MB Mid Makeup", -6.f, 12.f, 0.f);
    fp ("mb_highGain", "MB High Makeup", -6.f, 12.f, 0.f);

    // ----- Dynamic EQ -----
    bp ("dyn_on", "Dynamic EQ On", false);
    fpLog ("dyn_freq", "DynEQ Freq", 200.f, 8000.f, 3000.f);
    fp ("dyn_q", "DynEQ Q", 0.5f, 8.f, 2.f);
    fp ("dyn_thr", "DynEQ Threshold", -50.f, 0.f, -25.f);
    fp ("dyn_range", "DynEQ Range", -12.f, 12.f, -4.f);

    // ----- Exciter -----
    bp ("exc_on", "Exciter On", false);
    fpLog ("exc_freq", "Exciter Freq", 1500.f, 10000.f, 4000.f);
    fp ("exc_amt", "Exciter Amount", 0.f, 1.f, 0.25f);

    // ----- Transient -----
    bp ("tr_on", "Transient On", false);
    fp ("tr_attack", "Transient Attack", -12.f, 12.f, 0.f);
    fp ("tr_sustain", "Transient Sustain", -12.f, 12.f, 0.f);

    // ----- Limiter + peak protection -----
    bp ("lim_on", "Limiter On", true);
    fp ("lim_ceiling", "Limiter Ceiling", -6.f, 0.f, -0.3f);
    fp ("lim_release", "Limiter Release", 10.f, 500.f, 80.f);
    bp ("pp_on", "Peak Protection On", true);

    return l;
}
} // namespace TK
