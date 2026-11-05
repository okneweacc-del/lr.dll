#pragma once
#include "UI.h"

class TKVocalStudioEditor : public juce::AudioProcessorEditor
{
public:
    explicit TKVocalStudioEditor (TKVocalStudioProcessor&);
    ~TKVocalStudioEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void showPage (int i);
    void buildPages();

    TKVocalStudioProcessor& proc;
    UI::LnF lnf;
    UI::SpectrumView spectrum;
    UI::MeterPanel meters;

    juce::ComboBox presetBox, eqPresetBox;
    juce::TextButton prevBtn { "<" }, nextBtn { ">" }, abBtn { "A" }, copyBtn { "A>B" }, undoBtn { "Undo" }, redoBtn { "Redo" }, resetEqBtn { "Reset EQ" };
    std::unique_ptr<UI::ToggleCell> bypassBtn, levelBtn;
    std::unique_ptr<UI::KnobCell> inKnob, outKnob;
    juce::TextButton tabs[4];
    juce::Component pageHolder;
    juce::OwnedArray<UI::Page> pages;
    juce::Component eqTop;                       // hàng filter + preset của trang EQ
    std::unique_ptr<UI::Section> filterSec;
    int currentPage = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TKVocalStudioEditor)
};
