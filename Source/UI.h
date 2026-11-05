#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include "PluginProcessor.h"

namespace UI
{
using namespace juce;
inline const Colour kBg (0xff04040a), kBlue (0xff2ad4ff), kPurple (0xffa64dff), kDim (0xff8c94b0), kRed (0xffff4d6d), kGreen (0xff4dffb0);

inline void glass (Graphics& g, Rectangle<float> r, float corner = 14.f)
{
    g.setGradientFill (ColourGradient (Colour (0x22ffffff), r.getX(), r.getY(), Colour (0x0affffff), r.getX(), r.getBottom(), false));
    g.fillRoundedRectangle (r, corner);
    g.setGradientFill (ColourGradient (kBlue.withAlpha (0.55f), r.getX(), r.getY(), kPurple.withAlpha (0.45f), r.getRight(), r.getBottom(), false));
    g.drawRoundedRectangle (r.reduced (0.5f), corner, 1.2f);
    g.setColour (Colour (0x30ffffff)); // viền sáng phía trên giả lập kính
    g.drawLine (r.getX() + corner, r.getY() + 1.5f, r.getRight() - corner, r.getY() + 1.5f, 1.0f);
}

struct LnF : LookAndFeel_V4
{
    LnF()
    {
        setColour (Slider::textBoxTextColourId, Colours::white);
        setColour (Slider::textBoxOutlineColourId, Colours::transparentBlack);
        setColour (Slider::textBoxBackgroundColourId, Colours::transparentBlack);
        setColour (Label::textColourId, kDim);
        setColour (ComboBox::backgroundColourId, Colour (0xff0d0d18)); setColour (ComboBox::textColourId, Colours::white);
        setColour (ComboBox::outlineColourId, kBlue.withAlpha (0.5f)); setColour (ComboBox::arrowColourId, kBlue);
        setColour (PopupMenu::backgroundColourId, Colour (0xff0d0d18)); setColour (PopupMenu::textColourId, Colours::white);
        setColour (PopupMenu::highlightedBackgroundColourId, kPurple.withAlpha (0.5f));
        setColour (TextButton::textColourOffId, Colours::white); setColour (TextButton::textColourOnId, Colours::white);
    }
    void drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, Slider&) override
    {
        auto b = Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (5.f);
        const float r = jmin (b.getWidth(), b.getHeight()) * 0.5f; auto c = b.getCentre();
        Path track; track.addCentredArc (c.x, c.y, r, r, 0, a0, a1, true);
        g.setColour (Colour (0x28ffffff)); g.strokePath (track, PathStrokeType (3.f, PathStrokeType::curved, PathStrokeType::rounded));
        const float ang = a0 + pos * (a1 - a0);
        Path val; val.addCentredArc (c.x, c.y, r, r, 0, a0, ang, true);
        g.setGradientFill (ColourGradient (kBlue, c.x - r, c.y, kPurple, c.x + r, c.y, false));
        g.strokePath (val, PathStrokeType (3.5f, PathStrokeType::curved, PathStrokeType::rounded));
        const float kr = r * 0.72f;
        g.setGradientFill (ColourGradient (Colour (0xff23233a), c.x, c.y - kr, Colour (0xff0a0a14), c.x, c.y + kr, false));
        g.fillEllipse (c.x - kr, c.y - kr, kr * 2, kr * 2);
        Path p; p.addRoundedRectangle (-1.5f, -kr, 3.f, kr * 0.55f, 1.5f);
        g.setColour (Colours::white); g.fillPath (p, AffineTransform::rotation (ang).translated (c.x, c.y));
    }
    void drawToggleButton (Graphics& g, ToggleButton& b, bool hover, bool) override
    {
        auto r = b.getLocalBounds().toFloat().reduced (1.f);
        const bool on = b.getToggleState();
        g.setColour (on ? kBlue.withAlpha (hover ? 0.45f : 0.35f) : Colour (hover ? 0x20ffffff : 0x10ffffff));
        g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
        g.setColour (on ? kBlue : Colour (0x40ffffff)); g.drawRoundedRectangle (r, r.getHeight() * 0.5f, 1.2f);
        g.setColour (on ? Colours::white : kDim); g.setFont (Font (12.f, Font::bold));
        g.drawText (b.getButtonText(), r.toNearestInt(), Justification::centred);
    }
    void drawButtonBackground (Graphics& g, Button& b, const Colour&, bool hover, bool down) override
    {
        auto r = b.getLocalBounds().toFloat().reduced (1.f);
        g.setColour (Colour (down ? 0x30ffffff : hover ? 0x20ffffff : 0x10ffffff)); g.fillRoundedRectangle (r, 8.f);
        g.setColour (b.getToggleState() ? kPurple : kBlue.withAlpha (0.5f)); g.drawRoundedRectangle (r, 8.f, 1.2f);
    }
    Font getTextButtonFont (TextButton&, int) override { return Font (13.f, Font::bold); }
};

// ---------------------------------------------------------------- control cells
struct KnobCell : Component
{
    Label name; Slider s { Slider::RotaryHorizontalVerticalDrag, Slider::TextBoxBelow };
    std::unique_ptr<TK::APVTS::SliderAttachment> att;
    KnobCell (TK::APVTS& apvts, const String& id, const String& text)
    {
        name.setText (text, dontSendNotification); name.setJustificationType (Justification::centred); name.setFont (Font (11.5f));
        addAndMakeVisible (name); addAndMakeVisible (s);
        s.setTextBoxStyle (Slider::TextBoxBelow, false, 64, 16);
        att = std::make_unique<TK::APVTS::SliderAttachment> (apvts, id, s);
        s.setDoubleClickReturnValue (true, apvts.getParameter (id)->convertFrom0to1 (apvts.getParameter (id)->getDefaultValue()));
        s.textFromValueFunction = [] (double v) { return std::abs (v) >= 1000 ? String (v / 1000.0, 2) + "k" : String (v, std::abs (v) >= 100 ? 0 : 1); };
        s.valueFromTextFunction = [] (const String& t) { double v = t.getDoubleValue(); return t.containsIgnoreCase ("k") ? v * 1000.0 : v; };
        s.updateText();
    }
    void resized() override { auto r = getLocalBounds(); name.setBounds (r.removeFromTop (14)); s.setBounds (r); }
};

struct ToggleCell : ToggleButton
{
    std::unique_ptr<TK::APVTS::ButtonAttachment> att;
    ToggleCell (TK::APVTS& apvts, const String& id, const String& text) { setButtonText (text); att = std::make_unique<TK::APVTS::ButtonAttachment> (apvts, id, *this); }
};

struct ComboCell : ComboBox
{
    std::unique_ptr<TK::APVTS::ComboBoxAttachment> att;
    ComboCell (TK::APVTS& apvts, const String& id)
    {
        if (auto* p = dynamic_cast<AudioParameterChoice*> (apvts.getParameter (id))) addItemList (p->choices, 1);
        att = std::make_unique<TK::APVTS::ComboBoxAttachment> (apvts, id, *this);
    }
};

struct Section : Component
{
    String title; TK::APVTS& apvts;
    OwnedArray<KnobCell> knobs; OwnedArray<ToggleCell> toggles; OwnedArray<ComboCell> combos;
    Section (TK::APVTS& a, const String& t) : title (t), apvts (a) {}
    Section& knob (const String& id, const String& text) { auto* k = knobs.add (new KnobCell (apvts, id, text)); addAndMakeVisible (k); return *this; }
    Section& toggle (const String& id, const String& text) { auto* k = toggles.add (new ToggleCell (apvts, id, text)); addAndMakeVisible (k); return *this; }
    Section& combo (const String& id) { auto* k = combos.add (new ComboCell (apvts, id)); addAndMakeVisible (k); return *this; }
    float weight() const { return (float) knobs.size() + 0.6f * (float) (toggles.size() + combos.size()) + 0.5f; }
    void paint (Graphics& g) override
    {
        glass (g, getLocalBounds().toFloat());
        g.setColour (kBlue); g.setFont (Font (12.5f, Font::bold)); g.drawText (title.toUpperCase(), 12, 6, getWidth() - 24, 16, Justification::left);
    }
    void resized() override
    {
        const int pad = 10, W = getWidth() - 2 * pad; int x = pad, y = 28;
        for (auto* t : toggles) { if (x + 70 > pad + W) { x = pad; y += 28; } t->setBounds (x, y, 66, 22); x += 72; }
        for (auto* c : combos)  { if (x + 96 > pad + W) { x = pad; y += 28; } c->setBounds (x, y, 92, 22); x += 98; }
        if (! toggles.isEmpty() || ! combos.isEmpty()) y += 30;
        x = pad; const int cw = 70, ch = 96;
        const int perRow = jmax (1, W / cw);
        const int pitch = jmin (cw + 14, W / perRow);
        int col = 0;
        for (auto* k : knobs) { if (col == perRow) { col = 0; y += ch; } k->setBounds (pad + col * pitch + (pitch - cw) / 2, y, cw, ch); ++col; }
    }
};

struct Page : Component
{
    OwnedArray<Section> secs;
    Section& add (TK::APVTS& a, const String& t) { auto* s = secs.add (new Section (a, t)); addAndMakeVisible (s); return *s; }
    void resized() override
    {
        float tot = 0; for (auto* s : secs) tot += s->weight();
        const int gap = 8; const float avail = (float) (getWidth() - gap * (secs.size() - 1));
        int x = 0;
        for (int i = 0; i < secs.size(); ++i) { const int w = (int) (avail * secs[i]->weight() / tot); secs[i]->setBounds (x, 0, w, getHeight()); x += w + gap; }
    }
};

// ---------------------------------------------------------------- spectrum analyzer (L/R riêng) + đường cong EQ
class SpectrumView : public Component, private Timer
{
public:
    explicit SpectrumView (TKVocalStudioProcessor& p) : proc (p), fft (order), window ((size_t) fftSize, dsp::WindowingFunction<float>::hann)
    { magL.fill (-100.f); magR.fill (-100.f); startTimerHz (30); }

    void paint (Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        glass (g, r); auto a = r.reduced (10, 14);
        auto xOf = [&] (double f) { return a.getX() + a.getWidth() * (float) (std::log (f / 20.0) / std::log (1000.0)); };
        auto yOf = [&] (float db) { return a.getBottom() - a.getHeight() * jlimit (0.f, 1.f, (db + 100.f) / 100.f); };
        g.setFont (Font (10.f));
        for (double f : { 50., 100., 200., 500., 1000., 2000., 5000., 10000. })
        { g.setColour (Colour (0x18ffffff)); g.drawVerticalLine ((int) xOf (f), a.getY(), a.getBottom());
          g.setColour (kDim); g.drawText (f >= 1000 ? String ((int) (f / 1000)) + "k" : String ((int) f), (int) xOf (f) - 14, (int) a.getBottom() - 12, 28, 12, Justification::centred); }
        for (float db : { -80.f, -60.f, -40.f, -20.f }) { g.setColour (Colour (0x14ffffff)); g.drawHorizontalLine ((int) yOf (db), a.getX(), a.getRight()); }

        auto draw = [&] (const std::array<float, fftSize / 2>& m, Colour c)
        {
            Path p; bool first = true;
            for (int i = 1; i < fftSize / 2; ++i)
            {
                const double f = i * proc.currentSampleRate / fftSize; if (f < 20 || f > 20000) continue;
                const float x = xOf (f), y = yOf (m[(size_t) i]);
                if (first) { p.startNewSubPath (x, a.getBottom()); p.lineTo (x, y); first = false; } else p.lineTo (x, y);
            }
            if (first) return;
            Path fill (p); fill.lineTo (a.getRight(), a.getBottom()); fill.closeSubPath();
            g.setGradientFill (ColourGradient (c.withAlpha (0.35f), 0, a.getY(), c.withAlpha (0.02f), 0, a.getBottom(), false)); g.fillPath (fill);
            g.setColour (c); g.strokePath (p, PathStrokeType (1.4f));
        };
        draw (magL, kBlue); draw (magR, kPurple);

        // đường cong EQ (±18 dB quanh giữa)
        Path eq; const float mid = a.getCentreY();
        for (int i = 0; i <= 240; ++i)
        {
            const double f = 20.0 * std::pow (1000.0, i / 240.0); const float x = xOf (f);
            const float y = mid - (float) (eqDb (f) / 18.0) * a.getHeight() * 0.35f;
            i == 0 ? eq.startNewSubPath (x, y) : eq.lineTo (x, y);
        }
        g.setColour (Colours::white.withAlpha (0.9f)); g.strokePath (eq, PathStrokeType (1.6f));
        g.setColour (kBlue); g.drawText ("L", 16, 16, 20, 14, Justification::left);
        g.setColour (kPurple); g.drawText ("R", 32, 16, 20, 14, Justification::left);
        g.setColour (Colours::white); g.drawText ("EQ", 48, 16, 24, 14, Justification::left);
    }

private:
    static constexpr int order = 11, fftSize = 1 << order;
    TKVocalStudioProcessor& proc; dsp::FFT fft; dsp::WindowingFunction<float> window;
    std::array<float, fftSize / 2> magL, magR; std::array<float, fftSize * 2> work;

    double eqDb (double f) const
    {
        const double sr = proc.currentSampleRate; double m = 1.0;
        auto val = [&] (const String& id) { return (double) *proc.apvts.getRawParameterValue (id); };
        if (val ("hpfOn") > 0.5) { TKdsp::Filter4 h; h.setHP (sr, val ("hpfFreq")); m *= h.magnitudeAt (f, sr); }
        if (val ("lpfOn") > 0.5) { TKdsp::Filter4 h; h.setLP (sr, val ("lpfFreq")); m *= h.magnitudeAt (f, sr); }
        if (val ("eqOn") > 0.5)
            for (int b = 0; b < TK::numBands; ++b)
            {
                if (val (TK::bandId (b, "on")) < 0.5) continue;
                TKdsp::BandSettings bs; bs.freq = (float) val (TK::bandId (b, "freq")); bs.gain = (float) val (TK::bandId (b, "gain"));
                bs.q = (float) val (TK::bandId (b, "q")); bs.type = (int) std::lround (val (TK::bandId (b, "type")));
                TKdsp::Biquad bq; TKdsp::VocalChain::configureBand (bq, sr, bs); m *= bq.magnitudeAt (f, sr);
            }
        return 20.0 * std::log10 (std::max (m, 1e-6));
    }

    void analyse (const std::array<float, TKVocalStudioProcessor::ringSize>& ring, std::array<float, fftSize / 2>& mag)
    {
        const int w = proc.ringPos.load (std::memory_order_acquire);
        for (int i = 0; i < fftSize; ++i) work[(size_t) i] = ring[(size_t) ((w - fftSize + i) & (TKVocalStudioProcessor::ringSize - 1))];
        std::fill (work.begin() + fftSize, work.end(), 0.f);
        window.multiplyWithWindowingTable (work.data(), (size_t) fftSize);
        fft.performFrequencyOnlyForwardTransform (work.data());
        for (int i = 0; i < fftSize / 2; ++i)
        {
            const float db = Decibels::gainToDecibels (work[(size_t) i] / (fftSize * 0.25f), -120.f);
            const float old = mag[(size_t) i];
            mag[(size_t) i] = db > old ? db : old + (db - old) * 0.2f; // attack tức thì, decay mượt
        }
    }
    void timerCallback() override { analyse (proc.ringL, magL); analyse (proc.ringR, magR); repaint(); }
};

// ---------------------------------------------------------------- meters
class MeterPanel : public Component, private Timer
{
public:
    explicit MeterPanel (TKVocalStudioProcessor& p) : proc (p) { startTimerHz (30); }
    void paint (Graphics& g) override
    {
        auto r = getLocalBounds().toFloat(); glass (g, r);
        g.setFont (Font (12.5f, Font::bold)); g.setColour (kBlue); g.drawText ("METERS", 12, 6, 100, 16, Justification::left);
        const char* names[4] = { "IN L", "IN R", "OUT L", "OUT R" };
        const int top = 30, bot = getHeight() - 78, bw = 28, gap = 22;
        auto yOf = [&] (float lin) { const float db = jlimit (-60.f, 3.f, Decibels::gainToDecibels (lin, -80.f)); return (float) bot - (float) (bot - top) * (db + 60.f) / 63.f; };
        for (int i = 0; i < 4; ++i)
        {
            const int x = 18 + i * (bw + gap);
            g.setColour (Colour (0x20ffffff)); g.fillRoundedRectangle ((float) x, (float) top, (float) bw, (float) (bot - top), 4.f);
            const float y = yOf (level[i]);
            g.setGradientFill (ColourGradient (level[i] > 1.0f ? kRed : kBlue, 0, (float) top, kGreen.interpolatedWith (kBlue, 0.4f), 0, (float) bot, false));
            g.fillRoundedRectangle ((float) x, y, (float) bw, (float) bot - y, 4.f);
            g.setColour (Colours::white); g.fillRect ((float) x, yOf (hold[i]), (float) bw, 2.f);
            g.setColour (kDim); g.setFont (Font (10.5f)); g.drawText (names[i], x - 8, bot + 3, bw + 16, 12, Justification::centred);
            g.drawText (Decibels::gainToDecibels (hold[i], -80.f) > -79.f ? String (Decibels::gainToDecibels (hold[i], -80.f), 1) : "-inf", x - 8, bot + 15, bw + 16, 12, Justification::centred);
        }
        // correlation
        const int cy = getHeight() - 38, cx = 18, cw = getWidth() - 36;
        g.setColour (Colour (0x20ffffff)); g.fillRoundedRectangle ((float) cx, (float) cy, (float) cw, 8.f, 4.f);
        const float mid = cx + cw * 0.5f, pos = cx + cw * (corr * 0.5f + 0.5f);
        g.setColour (corr < 0 ? kRed : kGreen); g.fillRect (jmin (mid, pos), (float) cy, std::abs (pos - mid), 8.f);
        g.setColour (kDim); g.setFont (Font (10.5f)); g.drawText ("-1", cx, cy + 9, 20, 12, Justification::left); g.drawText ("+1", cx + cw - 20, cy + 9, 20, 12, Justification::right);
        g.drawText ("PHASE CORRELATION " + String (corr, 2), cx, cy - 14, cw, 12, Justification::centred);
        const bool warn = corr < -0.2f || monoLoss < -6.f;
        g.setColour (warn ? kRed : kGreen); g.setFont (Font (11.f, Font::bold));
        g.drawText (String (warn ? "MONO: CANH BAO" : "MONO: OK") + "  " + String (monoLoss, 1) + " dB   " + String (lufs, 1) + " LUFS",
                    cx, getHeight() - 16, cw, 14, Justification::centred);
    }
private:
    TKVocalStudioProcessor& proc; float level[4] {}, hold[4] {}, corr = 1, monoLoss = 0, lufs = -100;
    void timerCallback() override
    {
        float v[4] = { proc.stats.inPeakL.exchange (0.f), proc.stats.inPeakR.exchange (0.f), proc.stats.outPeakL.exchange (0.f), proc.stats.outPeakR.exchange (0.f) };
        for (int i = 0; i < 4; ++i) { level[i] = jmax (v[i], level[i] * 0.85f); hold[i] = jmax (v[i], hold[i] * 0.985f); }
        corr = proc.stats.correlation.load(); monoLoss = proc.stats.monoLossDb.load(); lufs = proc.stats.loudness.load();
        repaint();
    }
};
} // namespace UI
