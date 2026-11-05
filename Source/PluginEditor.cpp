#include "PluginEditor.h"
using namespace UI;

TKVocalStudioEditor::TKVocalStudioEditor (TKVocalStudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p), spectrum (p), meters (p)
{
    setLookAndFeel (&lnf);
    setSize (1180, 770);
    auto& a = proc.apvts;

    addAndMakeVisible (spectrum); addAndMakeVisible (meters);

    presetBox.addItemList (TKVocalStudioProcessor::getPresetNames(), 1);
    presetBox.setSelectedId (proc.getCurrentProgram() + 1, juce::dontSendNotification);
    presetBox.onChange = [this] { proc.applyPreset (presetBox.getSelectedId() - 1); };
    addAndMakeVisible (presetBox);
    prevBtn.onClick = [this] { const int n = presetBox.getNumItems(); presetBox.setSelectedId ((presetBox.getSelectedId() - 2 + n) % n + 1, juce::sendNotificationSync); };
    nextBtn.onClick = [this] { const int n = presetBox.getNumItems(); presetBox.setSelectedId (presetBox.getSelectedId() % n + 1, juce::sendNotificationSync); };
    abBtn.onClick = [this] { proc.toggleAB(); abBtn.setButtonText (proc.isB() ? "B" : "A"); copyBtn.setButtonText (proc.isB() ? "B>A" : "A>B"); };
    copyBtn.onClick = [this] { proc.copyAtoB(); };
    undoBtn.onClick = [this] { proc.undoManager.undo(); };
    redoBtn.onClick = [this] { proc.undoManager.redo(); };
    for (auto* b : { &prevBtn, &nextBtn, &abBtn, &copyBtn, &undoBtn, &redoBtn }) addAndMakeVisible (b);
    bypassBtn = std::make_unique<ToggleCell> (a, "bypass", "BYPASS");
    levelBtn = std::make_unique<ToggleCell> (a, "levelMatch", "LEVEL MATCH");
    addAndMakeVisible (*bypassBtn); addAndMakeVisible (*levelBtn);
    inKnob = std::make_unique<KnobCell> (a, "inGain", "INPUT dB");
    outKnob = std::make_unique<KnobCell> (a, "outGain", "OUTPUT dB");
    addAndMakeVisible (*inKnob); addAndMakeVisible (*outKnob);

    const char* names[4] = { "EQ", "STEREO", "VOCAL", "DYNAMICS" };
    for (int i = 0; i < 4; ++i)
    {
        tabs[i].setButtonText (names[i]); tabs[i].setClickingTogglesState (true); tabs[i].setRadioGroupId (77);
        tabs[i].onClick = [this, i] { showPage (i); };
        addAndMakeVisible (tabs[i]);
    }
    addAndMakeVisible (pageHolder);
    buildPages();
    tabs[0].setToggleState (true, juce::dontSendNotification);
    showPage (0);
}

TKVocalStudioEditor::~TKVocalStudioEditor() { setLookAndFeel (nullptr); }

void TKVocalStudioEditor::buildPages()
{
    auto& a = proc.apvts;

    // ---- Page 0: EQ ----
    auto* eq = pages.add (new Page());
    eq->addAndMakeVisible (eqTop);
    filterSec = std::make_unique<Section> (a, "Filters");
    filterSec->toggle ("eqOn", "EQ ON").toggle ("hpfOn", "HPF").knob ("hpfFreq", "HPF Hz").toggle ("lpfOn", "LPF").knob ("lpfFreq", "LPF Hz");
    eqTop.addAndMakeVisible (*filterSec);
    eqPresetBox.addItemList (TKVocalStudioProcessor::getEqPresetNames(), 1);
    eqPresetBox.setTextWhenNothingSelected ("EQ Presets...");
    eqPresetBox.onChange = [this] { proc.applyEqPreset (eqPresetBox.getSelectedId() - 1); };
    resetEqBtn.onClick = [this] { proc.resetEq(); eqPresetBox.setSelectedId (0, juce::dontSendNotification); };
    eqTop.addAndMakeVisible (eqPresetBox); eqTop.addAndMakeVisible (resetEqBtn);
    for (int b = 0; b < TK::numBands; ++b)
    {
        auto& s = eq->add (a, TK::bandNames[b]);
        s.toggle (TK::bandId (b, "on"), "ON").combo (TK::bandId (b, "type"))
         .knob (TK::bandId (b, "freq"), "Freq Hz").knob (TK::bandId (b, "gain"), "Gain dB").knob (TK::bandId (b, "q"), "Q");
    }

    // ---- Page 1: Stereo ----
    auto* st = pages.add (new Page());
    st->add (a, "Left / Right").knob ("leftGain", "Left dB").knob ("rightGain", "Right dB").knob ("balance", "Balance")
        .toggle ("phaseInvL", "INV L").toggle ("phaseInvR", "INV R");
    st->add (a, "Mid / Side").knob ("midGain", "Mid dB").knob ("sideGain", "Side dB").knob ("width", "Width").knob ("separation", "Separation");
    st->add (a, "Monitoring").toggle ("monoCheck", "MONO CHECK");

    // ---- Page 2: Vocal ----
    auto* vo = pages.add (new Page());
    vo->add (a, "Vocal Tone").toggle ("clarity_on", "CLARITY").knob ("clarity_amt", "Clarity").toggle ("presence_on", "PRESENCE").knob ("presence_amt", "Presence")
        .toggle ("warmth_on", "WARMTH").knob ("warmth_amt", "Warmth").toggle ("air_on", "AIR").knob ("air_amt", "Air");
    vo->add (a, "De-esser").toggle ("deess_on", "ON").knob ("deess_freq", "Freq Hz").knob ("deess_thr", "Thresh dB").knob ("deess_ratio", "Ratio");
    vo->add (a, "Noise Reduction").toggle ("nr_on", "ON").knob ("nr_thr", "Thresh dB").knob ("nr_range", "Reduce dB").knob ("nr_release", "Release ms");
    vo->add (a, "Exciter").toggle ("exc_on", "ON").knob ("exc_freq", "Freq Hz").knob ("exc_amt", "Amount");
    vo->add (a, "Transient").toggle ("tr_on", "ON").knob ("tr_attack", "Attack dB").knob ("tr_sustain", "Sustain dB");

    // ---- Page 3: Dynamics ----
    auto* dy = pages.add (new Page());
    dy->add (a, "Gentle Compressor").toggle ("comp_on", "ON").toggle ("comp_autoMakeup", "AUTO MK").knob ("comp_thr", "Thresh dB").knob ("comp_ratio", "Ratio")
        .knob ("comp_att", "Attack ms").knob ("comp_rel", "Release ms").knob ("comp_knee", "Knee dB").knob ("comp_makeup", "Makeup dB");
    dy->add (a, "Multiband").toggle ("mb_on", "ON").knob ("mb_x1", "X-over 1").knob ("mb_x2", "X-over 2").knob ("mb_thr", "Thresh dB").knob ("mb_ratio", "Ratio")
        .knob ("mb_lowGain", "Low dB").knob ("mb_midGain", "Mid dB").knob ("mb_highGain", "High dB");
    dy->add (a, "Dynamic EQ").toggle ("dyn_on", "ON").knob ("dyn_freq", "Freq Hz").knob ("dyn_q", "Q").knob ("dyn_thr", "Thresh dB").knob ("dyn_range", "Range dB");
    dy->add (a, "Limiter / Protection").toggle ("lim_on", "LIMITER").toggle ("pp_on", "PEAK PROT").knob ("lim_ceiling", "Ceiling dB").knob ("lim_release", "Release ms");

    for (auto* p : pages) pageHolder.addChildComponent (p);
}

void TKVocalStudioEditor::showPage (int i)
{
    currentPage = i;
    for (int k = 0; k < pages.size(); ++k) pages[k]->setVisible (k == i);
    resized();
}

void TKVocalStudioEditor::paint (juce::Graphics& g)
{
    g.fillAll (kBg);
    g.setGradientFill (juce::ColourGradient (kBlue.withAlpha (0.12f), 0, 0, juce::Colours::transparentBlack, 400, 300, true));
    g.fillRect (getLocalBounds());
    g.setGradientFill (juce::ColourGradient (kPurple.withAlpha (0.14f), (float) getWidth(), (float) getHeight(), juce::Colours::transparentBlack, (float) getWidth() - 450, (float) getHeight() - 300, true));
    g.fillRect (getLocalBounds());
    g.setFont (juce::Font (24.f, juce::Font::bold));
    g.setGradientFill (juce::ColourGradient (kBlue, 16, 0, kPurple, 260, 0, false));
    g.drawText ("TK VOCAL STUDIO", 16, 10, 280, 34, juce::Justification::left);
}

void TKVocalStudioEditor::resized()
{
    auto r = getLocalBounds().reduced (12);
    auto top = r.removeFromTop (38);
    top.removeFromLeft (290);
    presetBox.setBounds (top.removeFromLeft (200).reduced (0, 4)); top.removeFromLeft (4);
    prevBtn.setBounds (top.removeFromLeft (30).reduced (0, 4)); top.removeFromLeft (4);
    nextBtn.setBounds (top.removeFromLeft (30).reduced (0, 4)); top.removeFromLeft (14);
    abBtn.setBounds (top.removeFromLeft (38).reduced (0, 4)); top.removeFromLeft (4);
    copyBtn.setBounds (top.removeFromLeft (50).reduced (0, 4)); top.removeFromLeft (14);
    undoBtn.setBounds (top.removeFromLeft (56).reduced (0, 4)); top.removeFromLeft (4);
    redoBtn.setBounds (top.removeFromLeft (56).reduced (0, 4));
    bypassBtn->setBounds (top.removeFromRight (86).reduced (0, 6)); top.removeFromRight (6);
    levelBtn->setBounds (top.removeFromRight (110).reduced (0, 6));
    r.removeFromTop (8);

    auto upper = r.removeFromTop (270);
    auto right = upper.removeFromRight (276); upper.removeFromRight (8);
    spectrum.setBounds (upper);
    meters.setBounds (right.removeFromTop (190)); right.removeFromTop (6);
    inKnob->setBounds (right.removeFromLeft (right.getWidth() / 2));
    outKnob->setBounds (right);

    r.removeFromTop (8);
    auto tabRow = r.removeFromTop (28);
    for (int i = 0; i < 4; ++i) tabs[i].setBounds (tabRow.removeFromLeft (110).reduced (2, 0));
    r.removeFromTop (8);
    pageHolder.setBounds (r);
    for (auto* p : pages) p->setBounds (pageHolder.getLocalBounds());

    // trang EQ: hàng trên (filters + preset) và 7 band bên dưới
    if (auto* eq = pages[0])
    {
        auto b = eq->getLocalBounds();
        eqTop.setBounds (b.removeFromTop (118)); b.removeFromTop (8);
        auto t = eqTop.getLocalBounds();
        auto btns = t.removeFromRight (220); t.removeFromRight (8);
        filterSec->setBounds (t);
        eqPresetBox.setBounds (btns.removeFromTop (30).reduced (0, 2)); btns.removeFromTop (8);
        resetEqBtn.setBounds (btns.removeFromTop (30));
        const int gap = 8, n = TK::numBands, w = (b.getWidth() - gap * (n - 1)) / n;
        for (int i = 0; i < n; ++i) eq->secs[i]->setBounds (b.getX() + i * (w + gap), b.getY(), w, b.getHeight());
    }
}
