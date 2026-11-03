#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
// Voice
//==============================================================================
struct SalomohSound : public juce::SynthesiserSound
{
    bool appliesToNote (int) override { return true; }
    bool appliesToChannel (int) override { return true; }
};

class SalomohVoice : public juce::SynthesiserVoice
{
public:
    explicit SalomohVoice (juce::AudioProcessorValueTreeState& s) : apvts (s) {}

    bool canPlaySound (juce::SynthesiserSound* s) override { return dynamic_cast<SalomohSound*> (s) != nullptr; }

    void prepare (double sampleRate, int blockSize)
    {
        sr = sampleRate;
        adsr.setSampleRate (sampleRate);
        juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) blockSize, 1 };
        filter.prepare (spec);
        filter.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
        temp.setSize (1, blockSize);
    }

    void startNote (int midiNote, float velocity, juce::SynthesiserSound*, int pitchWheel) override
    {
        note = midiNote;
        vel = velocity;
        pitchWheelMoved (pitchWheel);
        phase1 = phase2 = phaseSub = 0.0;
        juce::ADSR::Parameters p { get ("attack"), get ("decay"), get ("sustain"), get ("release") };
        adsr.setParameters (p);
        adsr.noteOn();
    }

    void stopNote (float, bool allowTailOff) override
    {
        if (allowTailOff) adsr.noteOff();
        else { adsr.reset(); clearCurrentNote(); }
    }

    void pitchWheelMoved (int v) override { bend = ((v - 8192) / 8192.0) * 2.0; }
    void controllerMoved (int cc, int v) override { if (cc == 1) modWheel = v / 127.0f; }

    void renderNextBlock (juce::AudioBuffer<float>& out, int start, int num) override
    {
        if (! isVoiceActive()) return;
        if (temp.getNumSamples() < num) temp.setSize (1, num, false, false, true);

        const float wave   = get ("wave");       // 0 sine -> 1 saw -> 2 square
        const float detune = get ("detune");     // cents
        const float subLvl = get ("sub");
        const float noise  = get ("noise");
        const float drive  = 1.0f + get ("grain") * 8.0f;
        const float lfoRate = get ("lfoRate");
        const float lfoAmt  = juce::jlimit (0.0f, 1.0f, get ("lfoAmt") + get ("mouvement") * 0.6f + modWheel * 0.5f);
        const float cutoffBase = get ("cutoff") * std::pow (2.0f, get ("brillance") * 3.0f - 1.0f);
        filter.setResonance (juce::jmap (get ("reso"), 0.0f, 1.0f, 0.7f, 6.0f));

        const double freq = juce::MidiMessage::getMidiNoteInHertz (note) * std::pow (2.0, bend / 12.0);
        const double inc1 = freq / sr;
        const double inc2 = freq * std::pow (2.0, detune / 1200.0) / sr;
        auto* d = temp.getWritePointer (0);

        for (int i = 0; i < num; ++i)
        {
            lfoPhase += lfoRate / sr; if (lfoPhase >= 1.0) lfoPhase -= 1.0;
            const float lfo = (float) std::sin (lfoPhase * juce::MathConstants<double>::twoPi);

            float s = 0.5f * (osc (phase1, wave) + osc (phase2, wave));
            s += subLvl * (float) std::sin (phaseSub * juce::MathConstants<double>::twoPi);
            s += noise * (rng.nextFloat() * 2.0f - 1.0f);
            s = std::tanh (s * drive) / std::tanh (drive);

            if ((i & 15) == 0)
                filter.setCutoffFrequency (juce::jlimit (30.0f, 18000.0f, cutoffBase * std::pow (2.0f, lfo * lfoAmt * 2.0f)));
            s = filter.processSample (0, s);

            d[i] = s * adsr.getNextSample() * (0.35f + 0.65f * vel) * 0.3f;

            phase1 += inc1; if (phase1 >= 1.0) phase1 -= 1.0;
            phase2 += inc2; if (phase2 >= 1.0) phase2 -= 1.0;
            phaseSub += inc1 * 0.5; if (phaseSub >= 1.0) phaseSub -= 1.0;
        }

        for (int ch = 0; ch < out.getNumChannels(); ++ch)
            out.addFrom (ch, start, temp, 0, 0, num);

        if (! adsr.isActive()) clearCurrentNote();
    }

private:
    float get (const char* id) const { return apvts.getRawParameterValue (id)->load(); }

    static float osc (double ph, float wave)
    {
        const float sine = (float) std::sin (ph * juce::MathConstants<double>::twoPi);
        const float saw  = (float) (2.0 * ph - 1.0);
        const float sq   = ph < 0.5 ? 1.0f : -1.0f;
        if (wave < 1.0f) return sine + (saw - sine) * wave;
        return saw + (sq - saw) * (wave - 1.0f);
    }

    juce::AudioProcessorValueTreeState& apvts;
    juce::ADSR adsr;
    juce::dsp::StateVariableTPTFilter<float> filter;
    juce::AudioBuffer<float> temp;
    juce::Random rng;
    double sr = 44100.0, phase1 = 0, phase2 = 0, phaseSub = 0, lfoPhase = 0, bend = 0;
    int note = 60;
    float vel = 1.0f, modWheel = 0.0f;
};

//==============================================================================
// Presets
//==============================================================================
const std::vector<Preset>& SalomohAudioProcessor::presets()
{
    static const std::vector<Preset> p = {
        { "Init",            {} },
        { "Halo Pad",        { {"wave",0.9f},{"detune",14},{"attack",1.2f},{"release",2.5f},{"cutoff",2400},{"espace",0.7f},{"chorusMix",0.5f},{"lfoAmt",0.2f} } },
        { "Orbit Keys",      { {"wave",0.3f},{"attack",0.005f},{"decay",0.6f},{"sustain",0.4f},{"release",0.6f},{"cutoff",4000},{"espace",0.35f} } },
        { "Neon Lead",       { {"wave",1.0f},{"detune",9},{"attack",0.01f},{"sustain",0.8f},{"cutoff",6000},{"reso",0.35f},{"grain",0.3f},{"delayMix",0.3f} } },
        { "Sub Gravity",     { {"wave",0.1f},{"sub",0.9f},{"attack",0.005f},{"release",0.25f},{"cutoff",700},{"grain",0.4f},{"espace",0.05f} } },
        { "Chrome Pluck",    { {"wave",1.3f},{"attack",0.001f},{"decay",0.25f},{"sustain",0.0f},{"release",0.3f},{"cutoff",3200},{"reso",0.5f},{"delayMix",0.35f} } },
        { "Lunar Choir",     { {"wave",0.6f},{"detune",20},{"noise",0.05f},{"attack",0.9f},{"release",3.0f},{"cutoff",1800},{"espace",0.85f},{"chorusMix",0.6f} } },
        { "Pulse Drive",     { {"wave",2.0f},{"attack",0.005f},{"cutoff",1500},{"reso",0.6f},{"lfoRate",4},{"lfoAmt",0.5f},{"grain",0.5f} } },
        { "Glass Bells",     { {"wave",0.0f},{"detune",1200},{"attack",0.001f},{"decay",1.5f},{"sustain",0.0f},{"release",1.8f},{"espace",0.6f},{"delayMix",0.25f} } },
        { "Dust Texture",    { {"wave",0.5f},{"noise",0.35f},{"attack",0.6f},{"release",2.0f},{"cutoff",1200},{"lfoRate",0.2f},{"lfoAmt",0.4f},{"espace",0.7f} } },
        { "Gold Brass",      { {"wave",1.0f},{"detune",6},{"attack",0.06f},{"sustain",0.7f},{"cutoff",2200},{"reso",0.2f},{"grain",0.2f},{"espace",0.3f} } },
        { "Tunnel Wobble",   { {"wave",1.6f},{"sub",0.5f},{"cutoff",900},{"reso",0.7f},{"lfoRate",2},{"lfoAmt",0.8f},{"grain",0.4f} } },
    };
    return p;
}

//==============================================================================
SalomohAudioProcessor::SalomohAudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    for (int i = 0; i < 16; ++i) synth.addVoice (new SalomohVoice (apvts));
    synth.addSound (new SalomohSound());
}

juce::AudioProcessorValueTreeState::ParameterLayout SalomohAudioProcessor::createLayout()
{
    using P = juce::AudioParameterFloat;
    using R = juce::NormalisableRange<float>;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> v;
    auto add = [&] (const char* id, const char* name, R r, float def) { v.push_back (std::make_unique<P> (juce::ParameterID { id, 1 }, name, r, def)); };

    // Macros
    add ("brillance", "Brillance", R (0, 1), 0.5f);
    add ("espace",    "Espace",    R (0, 1), 0.3f);
    add ("mouvement", "Mouvement", R (0, 1), 0.0f);
    add ("grain",     "Grain",     R (0, 1), 0.1f);
    // Oscillateur
    add ("wave",   "Forme",   R (0, 2), 1.0f);
    add ("detune", "Detune",  R (0, 1200, 0, 0.4f), 8.0f);
    add ("sub",    "Sub",     R (0, 1), 0.2f);
    add ("noise",  "Bruit",   R (0, 1), 0.0f);
    // Filtre
    add ("cutoff", "Cutoff",    R (40, 16000, 0, 0.3f), 3000.0f);
    add ("reso",   "Resonance", R (0, 1), 0.2f);
    // Enveloppe
    add ("attack",  "Attack",  R (0.001f, 5, 0, 0.35f), 0.01f);
    add ("decay",   "Decay",   R (0.001f, 5, 0, 0.35f), 0.3f);
    add ("sustain", "Sustain", R (0, 1), 0.8f);
    add ("release", "Release", R (0.005f, 8, 0, 0.35f), 0.4f);
    // LFO
    add ("lfoRate", "LFO Rate", R (0.05f, 20, 0, 0.4f), 1.0f);
    add ("lfoAmt",  "LFO Amt",  R (0, 1), 0.0f);
    // FX
    add ("chorusMix", "Chorus", R (0, 1), 0.2f);
    add ("delayMix",  "Delay",  R (0, 1), 0.0f);
    add ("delayTime", "Delay Time", R (0.05f, 1.5f), 0.375f);
    add ("master",    "Volume", R (-36, 6), -3.0f);
    // Scaler (accords sur une touche)
    v.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "chord", 1 }, "Accord",
        juce::StringArray { "Off", "Majeur", "Mineur", "Maj7", "Min7", "Sus2", "Sus4", "Power" }, 0));
    return { v.begin(), v.end() };
}

bool SalomohAudioProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    return l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        || l.getMainOutputChannelSet() == juce::AudioChannelSet::mono();
}

void SalomohAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    synth.setCurrentPlaybackSampleRate (sampleRate);
    for (int i = 0; i < synth.getNumVoices(); ++i)
        if (auto* v = dynamic_cast<SalomohVoice*> (synth.getVoice (i))) v->prepare (sampleRate, samplesPerBlock);

    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) samplesPerBlock, 2 };
    chorus.prepare (spec);
    reverb.prepare (spec);
    master.prepare (spec);
    juce::dsp::ProcessSpec mono { sampleRate, (juce::uint32) samplesPerBlock, 1 };
    delayL.prepare (mono); delayR.prepare (mono);
}

static const std::vector<std::vector<int>> kChords = {
    {}, {0,4,7}, {0,3,7}, {0,4,7,11}, {0,3,7,10}, {0,2,7}, {0,5,7}, {0,7,12}
};

void SalomohAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    keyboardState.processNextMidiBuffer (midi, 0, buffer.getNumSamples(), true);

    // Scaler : une touche -> un accord
    const int chordIdx = (int) apvts.getRawParameterValue ("chord")->load();
    if (chordIdx > 0)
    {
        chordBuffer.clear();
        for (const auto m : midi)
        {
            auto msg = m.getMessage();
            if (msg.isNoteOnOrOff())
                for (int iv : kChords[(size_t) chordIdx])
                {
                    auto n = juce::jlimit (0, 127, msg.getNoteNumber() + iv);
                    chordBuffer.addEvent (msg.isNoteOn() ? juce::MidiMessage::noteOn (msg.getChannel(), n, msg.getVelocity())
                                                         : juce::MidiMessage::noteOff (msg.getChannel(), n), m.samplePosition);
                }
            else chordBuffer.addEvent (msg, m.samplePosition);
        }
        midi.swapWith (chordBuffer);
    }

    synth.renderNextBlock (buffer, midi, 0, buffer.getNumSamples());

    auto get = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };

    // Delay
    const float dMix = get ("delayMix");
    if (dMix > 0.001f && buffer.getNumChannels() >= 2)
    {
        const float dSamples = (float) (get ("delayTime") * sr);
        delayL.setDelay (dSamples); delayR.setDelay (dSamples * 1.07f);
        auto* L = buffer.getWritePointer (0); auto* Rr = buffer.getWritePointer (1);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const float dl = delayL.popSample (0), dr = delayR.popSample (0);
            delayL.pushSample (0, L[i] + dr * 0.45f);
            delayR.pushSample (0, Rr[i] + dl * 0.45f);
            L[i] += dl * dMix; Rr[i] += dr * dMix;
        }
    }

    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> ctx (block);

    chorus.setRate (0.8f); chorus.setDepth (0.35f); chorus.setCentreDelay (8.0f);
    chorus.setFeedback (0.1f); chorus.setMix (get ("chorusMix") * 0.6f);
    chorus.process (ctx);

    juce::dsp::Reverb::Parameters rp;
    const float esp = get ("espace");
    rp.roomSize = 0.4f + esp * 0.58f; rp.damping = 0.4f; rp.width = 1.0f;
    rp.wetLevel = esp * 0.5f; rp.dryLevel = 1.0f - esp * 0.3f;
    reverb.setParameters (rp);
    reverb.process (ctx);

    master.setGainDecibels (get ("master"));
    master.process (ctx);
}

//==============================================================================
int SalomohAudioProcessor::getNumPrograms() { return (int) presets().size(); }
const juce::String SalomohAudioProcessor::getProgramName (int i) { return juce::isPositiveAndBelow (i, getNumPrograms()) ? presets()[(size_t) i].name : ""; }

void SalomohAudioProcessor::setCurrentProgram (int index)
{
    if (! juce::isPositiveAndBelow (index, getNumPrograms())) return;
    currentPreset = index;
    for (auto* p : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            if (rp->getParameterID() != "master" && rp->getParameterID() != "chord")
                rp->setValueNotifyingHost (rp->getDefaultValue());
    for (auto& [id, val] : presets()[(size_t) index].values)
        if (auto* rp = apvts.getParameter (id))
            rp->setValueNotifyingHost (rp->convertTo0to1 (val));
}

void SalomohAudioProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    state.setProperty ("preset", currentPreset, nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, dest);
}

void SalomohAudioProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto t = juce::ValueTree::fromXml (*xml);
            currentPreset = t.getProperty ("preset", 0);
            apvts.replaceState (t);
        }
}

juce::AudioProcessorEditor* SalomohAudioProcessor::createEditor() { return new SalomohAudioProcessorEditor (*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new SalomohAudioProcessor(); }
