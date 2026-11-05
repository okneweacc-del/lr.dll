#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace TK;

namespace
{
struct PV { const char* id; float v; };
struct Preset { const char* name; std::vector<PV> v; };

// Giá trị theo đơn vị thật (dB, Hz, ms...). Mọi tham số không liệt kê sẽ về mặc định (module tắt, EQ phẳng).
const std::vector<Preset>& presets()
{
    static const std::vector<Preset> p = {
        { "Clear Vocal", { {"hpfFreq",90}, {"eq2_freq",320}, {"eq2_gain",-2.5f}, {"eq4_gain",2.5f}, {"eq5_gain",1.5f},
            {"clarity_on",1}, {"clarity_amt",0.5f}, {"deess_on",1}, {"deess_thr",-30}, {"comp_on",1}, {"comp_thr",-20}, {"comp_ratio",2.2f}, {"comp_makeup",2} } },
        { "Bright Vocal", { {"hpfFreq",90}, {"eq2_gain",-1.5f}, {"eq5_freq",6500}, {"eq5_gain",3}, {"eq6_freq",12000}, {"eq6_gain",3.5f},
            {"presence_on",1}, {"presence_amt",0.4f}, {"air_on",1}, {"air_amt",0.5f}, {"exc_on",1}, {"exc_amt",0.3f}, {"deess_on",1}, {"deess_thr",-32}, {"comp_on",1}, {"comp_ratio",2}, {"comp_makeup",1.5f} } },
        { "Warm Vocal", { {"hpfFreq",70}, {"eq1_freq",140}, {"eq1_gain",2.5f}, {"eq2_gain",1}, {"eq5_gain",-1.5f}, {"eq6_gain",-1},
            {"warmth_on",1}, {"warmth_amt",0.6f}, {"comp_on",1}, {"comp_ratio",2}, {"comp_makeup",1.5f}, {"deess_on",1} } },
        { "Deep Vocal", { {"hpfFreq",50}, {"eq0_freq",45}, {"eq0_gain",2}, {"eq1_freq",110}, {"eq1_gain",3}, {"eq4_gain",-1},
            {"warmth_on",1}, {"warmth_amt",0.5f}, {"comp_on",1}, {"comp_ratio",2.5f}, {"comp_makeup",2}, {"lpfFreq",16000}, {"lpfOn",1} } },
        { "Soft Vocal", { {"hpfFreq",90}, {"eq4_gain",-1.5f}, {"eq5_freq",5500}, {"eq5_gain",-2}, {"eq6_gain",1}, {"deess_on",1}, {"deess_thr",-34},
            {"comp_on",1}, {"comp_thr",-22}, {"comp_ratio",2}, {"comp_makeup",3}, {"nr_on",1}, {"nr_range",6}, {"warmth_on",1}, {"warmth_amt",0.3f} } },
        { "Studio Vocal", { {"hpfFreq",80}, {"eq2_gain",-2}, {"eq4_gain",1.5f}, {"clarity_on",1}, {"clarity_amt",0.4f}, {"presence_on",1}, {"presence_amt",0.3f},
            {"air_on",1}, {"air_amt",0.3f}, {"comp_on",1}, {"comp_ratio",2.5f}, {"comp_makeup",2}, {"deess_on",1}, {"dyn_on",1}, {"mb_on",1}, {"mb_ratio",1.8f} } },
        { "Gaming Microphone", { {"hpfFreq",120}, {"eq1_gain",-3}, {"eq1_freq",150}, {"eq4_gain",3}, {"nr_on",1}, {"nr_thr",-50}, {"nr_range",15},
            {"comp_on",1}, {"comp_thr",-22}, {"comp_ratio",4}, {"comp_makeup",4}, {"deess_on",1}, {"clarity_on",1}, {"clarity_amt",0.5f}, {"lim_ceiling",-1} } },
        { "Discord Voice", { {"hpfFreq",110}, {"lpfOn",1}, {"lpfFreq",12000}, {"eq4_gain",2.5f}, {"eq2_gain",-1.5f}, {"nr_on",1}, {"nr_thr",-48}, {"nr_range",14},
            {"comp_on",1}, {"comp_thr",-22}, {"comp_ratio",3.5f}, {"comp_makeup",3}, {"deess_on",1}, {"lim_ceiling",-1} } },
        { "Streaming Voice", { {"hpfFreq",100}, {"eq1_gain",-1.5f}, {"eq4_gain",2}, {"nr_on",1}, {"nr_range",10}, {"comp_on",1}, {"comp_ratio",3}, {"comp_makeup",3},
            {"mb_on",1}, {"mb_ratio",2.2f}, {"clarity_on",1}, {"clarity_amt",0.5f}, {"deess_on",1}, {"tr_on",1}, {"tr_sustain",-2}, {"lim_ceiling",-1} } },
        { "Podcast Voice", { {"hpfFreq",80}, {"eq3_gain",1.5f}, {"eq2_gain",-2}, {"warmth_on",1}, {"warmth_amt",0.4f}, {"nr_on",1}, {"nr_range",12},
            {"comp_on",1}, {"comp_thr",-20}, {"comp_ratio",3}, {"comp_makeup",3}, {"deess_on",1}, {"lim_ceiling",-1} } },
        { "Female Vocal", { {"hpfFreq",120}, {"eq1_freq",200}, {"eq1_gain",-2}, {"eq2_freq",400}, {"eq2_gain",-1.5f}, {"eq4_freq",3500}, {"eq4_gain",2}, {"eq5_gain",1.5f},
            {"air_on",1}, {"air_amt",0.4f}, {"deess_on",1}, {"deess_freq",7500}, {"comp_on",1}, {"comp_ratio",2.2f}, {"comp_makeup",1.5f} } },
        { "Male Vocal", { {"hpfFreq",70}, {"eq1_freq",130}, {"eq1_gain",1.5f}, {"eq2_freq",300}, {"eq2_gain",-2}, {"eq4_freq",2800}, {"eq4_gain",2.5f},
            {"deess_on",1}, {"deess_freq",5500}, {"comp_on",1}, {"comp_ratio",2.5f}, {"comp_makeup",2}, {"warmth_on",1}, {"warmth_amt",0.25f} } },
        { "Airy Vocal", { {"hpfFreq",100}, {"eq5_gain",2}, {"eq6_freq",13000}, {"eq6_gain",5}, {"air_on",1}, {"air_amt",0.8f}, {"exc_on",1}, {"exc_amt",0.35f},
            {"presence_on",1}, {"presence_amt",0.3f}, {"deess_on",1}, {"deess_thr",-34}, {"comp_on",1}, {"comp_ratio",2}, {"comp_makeup",1.5f} } },
        { "Natural Vocal", { {"hpfFreq",70}, {"eq4_gain",1}, {"comp_on",1}, {"comp_thr",-16}, {"comp_ratio",1.8f}, {"comp_knee",12}, {"comp_makeup",1} } },
        { "Professional Vocal", { {"hpfFreq",80}, {"eq2_gain",-2}, {"eq4_gain",2}, {"eq5_gain",1}, {"eq6_gain",2}, {"nr_on",1}, {"nr_range",8}, {"deess_on",1},
            {"dyn_on",1}, {"comp_on",1}, {"comp_ratio",2.5f}, {"comp_makeup",2}, {"mb_on",1}, {"mb_ratio",1.8f}, {"exc_on",1}, {"exc_amt",0.2f},
            {"clarity_on",1}, {"clarity_amt",0.35f}, {"air_on",1}, {"air_amt",0.3f}, {"lim_ceiling",-1} } },
    };
    return p;
}

const std::vector<Preset>& eqPresets()
{
    static const std::vector<Preset> p = {
        { "Flat", {} },
        { "Vocal Boost", { {"hpfFreq",80}, {"eq2_gain",-2}, {"eq4_gain",2.5f}, {"eq5_gain",1.5f}, {"eq6_gain",1.5f} } },
        { "Presence Lift", { {"hpfFreq",90}, {"eq4_gain",3}, {"eq5_gain",2} } },
        { "Warm Low End", { {"hpfFreq",60}, {"eq1_gain",3}, {"eq2_gain",1.5f}, {"eq6_gain",-1.5f} } },
        { "De-Mud", { {"hpfFreq",100}, {"eq2_freq",320}, {"eq2_gain",-4}, {"eq2_q",1.4f} } },
        { "Radio Voice", { {"hpfFreq",150}, {"lpfOn",1}, {"lpfFreq",9000}, {"eq1_gain",2}, {"eq4_gain",3} } },
        { "Air Shine", { {"hpfFreq",90}, {"eq6_freq",12000}, {"eq6_gain",5}, {"eq5_gain",1.5f} } },
    };
    return p;
}

bool isPresetParam (const juce::String& id)
{
    for (auto* pre : { "eq", "hpf", "lpf", "clarity", "presence", "warmth", "air", "deess", "nr_", "comp_", "mb_", "dyn_", "exc_", "tr_", "lim_", "pp_" })
        if (id.startsWith (pre)) return true;
    return false;
}
bool isEqParam (const juce::String& id) { return id.startsWith ("eq") || id.startsWith ("hpf") || id.startsWith ("lpf"); }

void atomicMax (std::atomic<float>& a, float v)
{
    float cur = a.load (std::memory_order_relaxed);
    while (v > cur && ! a.compare_exchange_weak (cur, v)) {}
}
}

TKVocalStudioProcessor::TKVocalStudioProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      undoManager (30000, 100),
      apvts (*this, &undoManager, "TKVocalStudio", createLayout())
{
    auto& s = settings;
    bindF ("inGain", &s.inGainDb); bindF ("outGain", &s.outGainDb); bindB ("bypass", &s.bypass); bindB ("levelMatch", &s.levelMatch);
    bindF ("leftGain", &s.leftDb); bindF ("rightGain", &s.rightDb); bindF ("balance", &s.balance); bindF ("width", &s.width);
    bindF ("midGain", &s.midDb); bindF ("sideGain", &s.sideDb); bindF ("separation", &s.separation);
    bindB ("monoCheck", &s.monoCheck); bindB ("phaseInvL", &s.invL); bindB ("phaseInvR", &s.invR);
    bindB ("eqOn", &s.eqOn); bindB ("hpfOn", &s.hpfOn); bindF ("hpfFreq", &s.hpfFreq); bindB ("lpfOn", &s.lpfOn); bindF ("lpfFreq", &s.lpfFreq);
    for (int b = 0; b < numBands; ++b)
    {
        bindB (bandId (b, "on"), &s.band[b].on); bindF (bandId (b, "freq"), &s.band[b].freq);
        bindF (bandId (b, "gain"), &s.band[b].gain); bindF (bandId (b, "q"), &s.band[b].q); bindI (bandId (b, "type"), &s.band[b].type);
    }
    bindB ("clarity_on", &s.clarity.on); bindF ("clarity_amt", &s.clarity.amt);
    bindB ("presence_on", &s.presence.on); bindF ("presence_amt", &s.presence.amt);
    bindB ("warmth_on", &s.warmth.on); bindF ("warmth_amt", &s.warmth.amt);
    bindB ("air_on", &s.air.on); bindF ("air_amt", &s.air.amt);
    bindB ("deess_on", &s.deessOn); bindF ("deess_freq", &s.deessFreq); bindF ("deess_thr", &s.deessThr); bindF ("deess_ratio", &s.deessRatio);
    bindB ("nr_on", &s.nrOn); bindF ("nr_thr", &s.nrThr); bindF ("nr_range", &s.nrRange); bindF ("nr_release", &s.nrRelease);
    bindB ("comp_on", &s.compOn); bindF ("comp_thr", &s.compThr); bindF ("comp_ratio", &s.compRatio); bindF ("comp_att", &s.compAtt);
    bindF ("comp_rel", &s.compRel); bindF ("comp_knee", &s.compKnee); bindF ("comp_makeup", &s.compMakeup); bindB ("comp_autoMakeup", &s.compAuto);
    bindB ("mb_on", &s.mbOn); bindF ("mb_x1", &s.mbX1); bindF ("mb_x2", &s.mbX2); bindF ("mb_thr", &s.mbThr); bindF ("mb_ratio", &s.mbRatio);
    bindF ("mb_lowGain", &s.mbLow); bindF ("mb_midGain", &s.mbMid); bindF ("mb_highGain", &s.mbHigh);
    bindB ("dyn_on", &s.dynOn); bindF ("dyn_freq", &s.dynFreq); bindF ("dyn_q", &s.dynQ); bindF ("dyn_thr", &s.dynThr); bindF ("dyn_range", &s.dynRange);
    bindB ("exc_on", &s.excOn); bindF ("exc_freq", &s.excFreq); bindF ("exc_amt", &s.excAmt);
    bindB ("tr_on", &s.trOn); bindF ("tr_attack", &s.trAttack); bindF ("tr_sustain", &s.trSustain);
    bindB ("lim_on", &s.limOn); bindF ("lim_ceiling", &s.limCeiling); bindF ("lim_release", &s.limRelease); bindB ("pp_on", &s.ppOn);
}

bool TKVocalStudioProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    return l.getMainInputChannelSet() == juce::AudioChannelSet::stereo() && l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void TKVocalStudioProcessor::prepareToPlay (double sr, int)
{
    currentSampleRate = sr;
    chain.prepare (sr);
    setLatencySamples (chain.getLatency());
}

void TKVocalStudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    if (buffer.getNumChannels() < 2) return;

    for (auto& b : binds)
    {
        const float v = b.src->load (std::memory_order_relaxed);
        if (b.kind == 0) *static_cast<float*> (b.dst) = v;
        else if (b.kind == 1) *static_cast<bool*> (b.dst) = v > 0.5f;
        else *static_cast<int*> (b.dst) = (int) std::lround (v);
    }

    float* L = buffer.getWritePointer (0);
    float* R = buffer.getWritePointer (1);
    chain.process (L, R, n, settings);

    const auto& m = chain.meters;
    atomicMax (stats.inPeakL, m.inPeakL); atomicMax (stats.inPeakR, m.inPeakR);
    atomicMax (stats.outPeakL, m.outPeakL); atomicMax (stats.outPeakR, m.outPeakR);
    stats.loudness.store (m.loudness); stats.correlation.store (m.correlation); stats.monoLossDb.store (m.monoLossDb);
    stats.grComp.store (m.grComp); stats.grLimiter.store (m.grLimiter); stats.grDeEss.store (m.grDeEss);

    int w = ringPos.load (std::memory_order_relaxed);
    for (int i = 0; i < n; ++i) { ringL[(size_t) w] = L[i]; ringR[(size_t) w] = R[i]; w = (w + 1) & (ringSize - 1); }
    ringPos.store (w, std::memory_order_release);
}

// ------------------------------------------------------------------ presets
juce::StringArray TKVocalStudioProcessor::getPresetNames() { juce::StringArray a; for (auto& p : presets()) a.add (p.name); return a; }
juce::StringArray TKVocalStudioProcessor::getEqPresetNames() { juce::StringArray a; for (auto& p : eqPresets()) a.add (p.name); return a; }
int TKVocalStudioProcessor::getNumPrograms() { return (int) presets().size(); }
const juce::String TKVocalStudioProcessor::getProgramName (int i) { return juce::isPositiveAndBelow (i, getNumPrograms()) ? presets()[(size_t) i].name : juce::String(); }
void TKVocalStudioProcessor::setCurrentProgram (int i) { applyPreset (i); }

void TKVocalStudioProcessor::setParam (const juce::String& id, float v)
{
    if (auto* p = apvts.getParameter (id)) { p->beginChangeGesture(); p->setValueNotifyingHost (p->convertTo0to1 (v)); p->endChangeGesture(); }
}

void TKVocalStudioProcessor::setAllDefaults (bool eqOnly)
{
    for (auto* p : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
        {
            const auto id = rp->getParameterID();
            if (eqOnly ? isEqParam (id) : isPresetParam (id))
            { rp->beginChangeGesture(); rp->setValueNotifyingHost (rp->getDefaultValue()); rp->endChangeGesture(); }
        }
}

void TKVocalStudioProcessor::applyPreset (int index)
{
    if (! juce::isPositiveAndBelow (index, (int) presets().size())) return;
    undoManager.beginNewTransaction ("Preset " + juce::String (presets()[(size_t) index].name));
    setAllDefaults (false);
    for (auto& pv : presets()[(size_t) index].v) setParam (pv.id, pv.v);
    currentPreset = index;
}

void TKVocalStudioProcessor::applyEqPreset (int index)
{
    if (! juce::isPositiveAndBelow (index, (int) eqPresets().size())) return;
    undoManager.beginNewTransaction ("EQ Preset");
    setAllDefaults (true);
    for (auto& pv : eqPresets()[(size_t) index].v) setParam (pv.id, pv.v);
}

void TKVocalStudioProcessor::resetEq() { undoManager.beginNewTransaction ("Reset EQ"); setAllDefaults (true); }

void TKVocalStudioProcessor::toggleAB()
{
    const int cur = slotIsB ? 1 : 0, other = 1 - cur;
    slotState[cur] = apvts.copyState();
    if (! slotState[other].isValid()) slotState[other] = slotState[cur].createCopy();
    apvts.replaceState (slotState[other].createCopy());
    slotIsB = ! slotIsB;
}

void TKVocalStudioProcessor::copyAtoB()
{
    // sao chép trạng thái hiện tại sang slot còn lại
    slotState[slotIsB ? 0 : 1] = apvts.copyState();
}

// ------------------------------------------------------------------ state
void TKVocalStudioProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    state.setProperty ("preset", currentPreset, nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, dest);
}

void TKVocalStudioProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto vt = juce::ValueTree::fromXml (*xml);
            currentPreset = (int) vt.getProperty ("preset", 0);
            apvts.replaceState (vt);
        }
}

juce::AudioProcessorEditor* TKVocalStudioProcessor::createEditor() { return new TKVocalStudioEditor (*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new TKVocalStudioProcessor(); }
