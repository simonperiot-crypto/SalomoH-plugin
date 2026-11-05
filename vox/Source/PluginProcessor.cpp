#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    using APF = juce::AudioParameterFloat;
    using APC = juce::AudioParameterChoice;
    using APB = juce::AudioParameterBool;

    juce::ParameterID pid (const char* id) { return { id, 1 }; }

    std::unique_ptr<APF> fp (const char* id, const char* name, float lo, float hi, float def, const char* unit,
                             int decimals = 1, float skewCentre = -1.0f)
    {
        juce::NormalisableRange<float> r (lo, hi);
        if (skewCentre > lo && skewCentre < hi) r.setSkewForCentre (skewCentre);
        juce::String u (unit);
        auto attrs = juce::AudioParameterFloatAttributes()
                        .withLabel (u)
                        .withStringFromValueFunction ([u, decimals] (float v, int)
                        {
                            if (std::abs (v) < 0.05f) v = 0.0f;
                            if (u == "Hz" && v >= 1000.0f) return juce::String (v / 1000.0f, 1) + " kHz";
                            return juce::String (v, decimals) + (u.isEmpty() ? "" : " " + u);
                        });
        return std::make_unique<APF> (pid (id), name, r, def, attrs);
    }

    std::unique_ptr<APB> bp (const char* id, const char* name, bool def)
    {
        return std::make_unique<APB> (pid (id), name, def);
    }

    const float kDivBeats[] = { 2.0f, 1.5f, 1.0f, 0.75f, 2.0f / 3.0f, 0.5f, 1.0f / 3.0f, 0.25f };
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout VoxAudioProcessor::createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> v;

    juce::StringArray keys, scales;
    for (auto* k : vox::kKeyNamesLong) keys.add (k);
    for (auto& s : vox::kScales) scales.add (s.name);

    v.push_back (fp ("in_gain",  "Input",  -24, 24, 0, "dB"));
    v.push_back (fp ("out_gain", "Output", -24, 24, 0, "dB"));

    v.push_back (bp ("gate_on",  "Gate On", false));
    v.push_back (fp ("gate_thr", "Gate Threshold", -80, -20, -55, "dB"));

    v.push_back (bp ("tune_on",  "Autotune On", true));
    v.push_back (std::make_unique<APC> (pid ("key"),   "Key",   keys,   0));
    v.push_back (std::make_unique<APC> (pid ("scale"), "Scale", scales, 0));
    v.push_back (fp ("retune",   "Retune Speed", 0, 400, 40, "ms", 0, 60));
    v.push_back (fp ("amount",   "Intensity", 0, 100, 100, "%", 0));
    v.push_back (fp ("humanize", "Humanize", 0, 100, 0, "%", 0));
    v.push_back (fp ("transpose","Transpose", -12, 12, 0, "st", 0));

    v.push_back (bp ("eq_on",    "EQ On", true));
    v.push_back (fp ("eq_hpf",   "Low Cut", 20, 400, 80, "Hz", 0, 100));
    v.push_back (fp ("eq_mud",   "Mud (300 Hz)", -12, 6, 0, "dB"));
    v.push_back (fp ("eq_pres",  "Presence (3.5 kHz)", -6, 12, 0, "dB"));
    v.push_back (fp ("eq_air",   "Air (12 kHz)", -6, 12, 0, "dB"));

    v.push_back (bp ("ds_on",    "De-Esser On", true));
    v.push_back (fp ("ds_freq",  "De-Ess Freq", 3000, 10000, 6500, "Hz", 0));
    v.push_back (fp ("ds_thr",   "De-Ess Threshold", -50, 0, -26, "dB"));

    v.push_back (bp ("cmp_on",   "Compressor On", true));
    v.push_back (fp ("cmp_thr",  "Comp Threshold", -50, 0, -18, "dB"));
    v.push_back (fp ("cmp_ratio","Comp Ratio", 1, 20, 3, ":1", 1, 4));
    v.push_back (fp ("cmp_att",  "Comp Attack", 0.3f, 50, 5, "ms", 1, 6));
    v.push_back (fp ("cmp_rel",  "Comp Release", 20, 600, 120, "ms", 0, 120));
    v.push_back (fp ("cmp_gain", "Comp Makeup", 0, 24, 3, "dB"));

    v.push_back (bp ("sat_on",   "Saturation On", false));
    v.push_back (fp ("sat_drive","Drive", 0, 24, 6, "dB"));
    v.push_back (fp ("sat_mix",  "Saturation Mix", 0, 100, 40, "%", 0));

    v.push_back (bp ("dbl_on",   "Doubler On", false));
    v.push_back (fp ("dbl_mix",  "Doubler Mix", 0, 100, 30, "%", 0));
    v.push_back (fp ("dbl_width","Doubler Width", 0, 100, 80, "%", 0));

    v.push_back (bp ("dly_on",   "Delay On", false));
    v.push_back (std::make_unique<APC> (pid ("dly_div"), "Delay Time",
                    juce::StringArray { "1/2", "1/4 .", "1/4", "1/8 .", "1/4 T", "1/8", "1/8 T", "1/16" }, 5));
    v.push_back (fp ("dly_fb",   "Delay Feedback", 0, 90, 30, "%", 0));
    v.push_back (fp ("dly_tone", "Delay Tone", 800, 16000, 5000, "Hz", 0, 4000));
    v.push_back (fp ("dly_mix",  "Delay Mix", 0, 100, 15, "%", 0));
    v.push_back (bp ("dly_ping", "Ping-Pong", true));

    v.push_back (bp ("rev_on",   "Reverb On", true));
    v.push_back (fp ("rev_size", "Reverb Size", 0, 100, 55, "%", 0));
    v.push_back (fp ("rev_damp", "Reverb Damping", 0, 100, 45, "%", 0));
    v.push_back (fp ("rev_width","Reverb Width", 0, 100, 90, "%", 0));
    v.push_back (fp ("rev_mix",  "Reverb Mix", 0, 100, 12, "%", 0));

    return { v.begin(), v.end() };
}

//==============================================================================
const std::vector<VoxPreset>& VoxAudioProcessor::presets()
{
    static const std::vector<VoxPreset> list =
    {
        { "IFMY Signature", { { "retune", 30 }, { "amount", 90 }, { "humanize", 35 }, { "eq_hpf", 90 }, { "eq_mud", -2 },
                              { "eq_pres", 2.5f }, { "eq_air", 3 }, { "cmp_thr", -20 }, { "cmp_ratio", 3.5f }, { "cmp_gain", 4 },
                              { "dbl_on", 1 }, { "dbl_mix", 18 }, { "dly_on", 1 }, { "dly_div", 5 }, { "dly_mix", 10 },
                              { "rev_mix", 14 } } },
        { "Hard Tune (Trap)", { { "retune", 0 }, { "amount", 100 }, { "humanize", 0 }, { "eq_hpf", 110 }, { "eq_pres", 3 },
                                { "eq_air", 4 }, { "cmp_thr", -24 }, { "cmp_ratio", 5 }, { "cmp_gain", 6 }, { "sat_on", 1 },
                                { "sat_drive", 7 }, { "sat_mix", 30 }, { "dly_on", 1 }, { "dly_div", 5 }, { "dly_mix", 14 },
                                { "rev_mix", 15 } } },
        { "R&B Soyeux", { { "retune", 22 }, { "amount", 90 }, { "humanize", 50 }, { "eq_hpf", 80 }, { "eq_mud", -3 },
                          { "eq_air", 4 }, { "cmp_thr", -20 }, { "cmp_ratio", 3 }, { "dbl_on", 1 }, { "dbl_mix", 28 },
                          { "dly_on", 1 }, { "dly_div", 2 }, { "dly_mix", 10 }, { "rev_size", 72 }, { "rev_mix", 22 } } },
        { "Pop Brillante", { { "retune", 35 }, { "amount", 85 }, { "humanize", 30 }, { "eq_hpf", 100 }, { "eq_pres", 3 },
                             { "eq_air", 5 }, { "cmp_thr", -22 }, { "cmp_ratio", 4 }, { "cmp_gain", 5 }, { "dbl_on", 1 },
                             { "dbl_mix", 20 }, { "rev_mix", 15 } } },
        { "Afro / Amapiano", { { "retune", 8 }, { "amount", 95 }, { "humanize", 10 }, { "eq_hpf", 100 }, { "eq_air", 3 },
                               { "cmp_thr", -22 }, { "cmp_ratio", 4 }, { "dly_on", 1 }, { "dly_div", 3 }, { "dly_fb", 35 },
                               { "dly_mix", 18 }, { "rev_mix", 18 } } },
        { "Naturel (Live)", { { "retune", 120 }, { "amount", 55 }, { "humanize", 70 }, { "eq_hpf", 70 }, { "cmp_thr", -16 },
                              { "cmp_ratio", 2.5f }, { "cmp_gain", 2 }, { "rev_mix", 10 } } },
        { "Robot", { { "scale", 4 }, { "retune", 0 }, { "amount", 100 }, { "humanize", 0 }, { "eq_hpf", 140 },
                     { "sat_on", 1 }, { "sat_drive", 10 }, { "sat_mix", 45 }, { "dbl_on", 1 }, { "dbl_mix", 25 },
                     { "rev_mix", 10 } } },
        { "Lo-Fi Bedroom", { { "retune", 45 }, { "amount", 80 }, { "eq_hpf", 260 }, { "eq_air", -4 }, { "sat_on", 1 },
                             { "sat_drive", 15 }, { "sat_mix", 60 }, { "dly_on", 1 }, { "dly_div", 2 }, { "dly_tone", 2500 },
                             { "dly_mix", 16 }, { "rev_size", 40 }, { "rev_damp", 80 }, { "rev_mix", 24 } } },
        { "Grosse Salle", { { "retune", 40 }, { "amount", 85 }, { "humanize", 40 }, { "eq_air", 3 }, { "dly_on", 1 },
                            { "dly_div", 2 }, { "dly_fb", 40 }, { "dly_mix", 14 }, { "rev_size", 90 }, { "rev_damp", 30 },
                            { "rev_mix", 32 } } },
        { "Voix seule (sans effets)", { { "eq_on", 0 }, { "ds_on", 0 }, { "cmp_on", 0 }, { "rev_on", 0 } } },
    };
    return list;
}

void VoxAudioProcessor::applyPreset (int index)
{
    if (index < 0 || index >= (int) presets().size()) return;
    currentPreset = index;

    // Le preset garde la tonalité choisie par le chanteur
    for (auto* param : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (param))
            if (rp->paramID != "key" && rp->paramID != "scale" && rp->paramID != "in_gain" && rp->paramID != "out_gain")
                rp->setValueNotifyingHost (rp->getDefaultValue());

    for (auto& [id, value] : presets()[(size_t) index].values)
        if (auto* rp = apvts.getParameter (id))
            rp->setValueNotifyingHost (rp->convertTo0to1 (value));
}

int VoxAudioProcessor::getNumPrograms() { return (int) presets().size(); }
void VoxAudioProcessor::setCurrentProgram (int index) { applyPreset (index); }
const juce::String VoxAudioProcessor::getProgramName (int index)
{
    return juce::isPositiveAndBelow (index, (int) presets().size()) ? presets()[(size_t) index].name : "";
}

//==============================================================================
VoxAudioProcessor::VoxAudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "VOX", createLayout())
{
}

bool VoxAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in = layouts.getMainInputChannelSet(), out = layouts.getMainOutputChannelSet();
    const auto mono = juce::AudioChannelSet::mono(), stereo = juce::AudioChannelSet::stereo();
    if (out != mono && out != stereo) return false;
    if (in != mono && in != stereo) return false;
    if (in == stereo && out == mono) return false;
    return true;
}

void VoxAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    tuner.prepare (sr, 2);
    setLatencySamples (tuner.getLatency());

    gate.prepare (sr);
    for (int c = 0; c < 2; ++c) { hpf[c].reset(); mud[c].reset(); pres[c].reset(); air[c].reset(); }
    for (auto& e : lastEq) e = -999.0f;
    deess.prepare (sr);
    comp.prepare (sr);
    dbl.prepare (sr);
    dly.prepare (sr);
    reverb.setSampleRate (sr);
    reverb.reset();
    revBuf.setSize (2, juce::jmax (512, samplesPerBlock));

    inGain.reset (sr, 0.03);  inGain.setCurrentAndTargetValue (vox::dbToGain (*p ("in_gain")));
    outGain.reset (sr, 0.03); outGain.setCurrentAndTargetValue (vox::dbToGain (*p ("out_gain")));
    revMix.reset (sr, 0.05);  revMix.setCurrentAndTargetValue (0);
}

void VoxAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    const int numIn = getTotalNumInputChannels(), numOut = getTotalNumOutputChannels();
    if (n == 0 || numOut == 0) return;

    if (numIn == 1 && numOut >= 2) buffer.copyFrom (1, 0, buffer, 0, 0, n);
    const int nc = juce::jmin (2, numOut, buffer.getNumChannels());

    // --- Paramètres --------------------------------------------------------
    vox::AutoTune::Params tp;
    tp.on        = *p ("tune_on") > 0.5f;
    tp.key       = (int) *p ("key");
    tp.scale     = vox::kScales[juce::jlimit (0, vox::kNumScales - 1, (int) *p ("scale"))].mask;
    tp.retuneMs  = *p ("retune");
    tp.amount    = *p ("amount") * 0.01f;
    tp.humanize  = *p ("humanize") * 0.01f;
    tp.transpose = std::round (p ("transpose")->load());
    tuner.setParams (tp);

    const bool gateOn = *p ("gate_on") > 0.5f, eqOn = *p ("eq_on") > 0.5f, dsOn = *p ("ds_on") > 0.5f,
               cmpOn = *p ("cmp_on") > 0.5f, satOn = *p ("sat_on") > 0.5f, dblOn = *p ("dbl_on") > 0.5f,
               dlyOn = *p ("dly_on") > 0.5f, revOn = *p ("rev_on") > 0.5f, ping = *p ("dly_ping") > 0.5f;

    gate.setThreshold (*p ("gate_thr"));

    const float eqv[4] = { *p ("eq_hpf"), *p ("eq_mud"), *p ("eq_pres"), *p ("eq_air") };
    if (eqv[0] != lastEq[0] || eqv[1] != lastEq[1] || eqv[2] != lastEq[2] || eqv[3] != lastEq[3])
    {
        for (int c = 0; c < 2; ++c)
        {
            hpf[c].highpass (sr, eqv[0], 0.7071);
            mud[c].peak (sr, 300.0, 1.0, eqv[1]);
            pres[c].peak (sr, 3500.0, 0.8, eqv[2]);
            air[c].highShelf (sr, 12000.0, eqv[3]);
        }
        std::copy (eqv, eqv + 4, lastEq);
    }

    deess.setFreq (*p ("ds_freq"));
    deess.setThreshold (*p ("ds_thr"));
    comp.set (*p ("cmp_thr"), *p ("cmp_ratio"), *p ("cmp_att"), *p ("cmp_rel"), *p ("cmp_gain"));
    sat.set (*p ("sat_drive"), *p ("sat_mix") * 0.01f);

    const float dblMix = *p ("dbl_mix") * 0.01f, dblWidth = *p ("dbl_width") * 0.01f;

    double bpm = 120.0;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto b = pos->getBpm())
                if (*b > 20.0) bpm = *b;
    const int div = juce::jlimit (0, 7, (int) *p ("dly_div"));
    const float dlySamples = (float) (60.0 / bpm * kDivBeats[div] * sr);
    const float dlyFb = *p ("dly_fb") * 0.01f, dlyMix = *p ("dly_mix") * 0.01f;
    const float dlyTone = (float) (1.0 - std::exp (-2.0 * vox::kPiD * *p ("dly_tone") / sr));

    inGain.setTargetValue (vox::dbToGain (*p ("in_gain")));
    outGain.setTargetValue (vox::dbToGain (*p ("out_gain")));

    // --- Chaîne échantillon par échantillon ---------------------------------
    float* ch[2] = { buffer.getWritePointer (0), nc > 1 ? buffer.getWritePointer (1) : nullptr };
    float inPk = 0;

    for (int i = 0; i < n; ++i)
    {
        float f[2] = { ch[0][i], nc > 1 ? ch[1][i] : 0.0f };
        const float g = inGain.getNextValue();
        for (int c = 0; c < nc; ++c) { f[c] *= g; inPk = std::max (inPk, std::abs (f[c])); }

        if (gateOn) gate.process (f, nc);
        tuner.process (f, nc);

        if (eqOn)
            for (int c = 0; c < nc; ++c)
                f[c] = air[c].process (pres[c].process (mud[c].process (hpf[c].process (f[c]))));

        if (dsOn)  deess.process (f, nc); else deess.grDb = 0;
        if (cmpOn) comp.process (f, nc);  else comp.grDb = 0;
        if (satOn) sat.process (f, nc);
        if (dblOn) dbl.process (f, nc, dblMix, dblWidth);
        if (dlyOn) dly.process (f, nc, dlySamples, dlyFb, dlyTone, dlyMix, ping);

        ch[0][i] = f[0];
        if (nc > 1) ch[1][i] = f[1];
    }

    // --- Réverbe ------------------------------------------------------------
    revMix.setTargetValue (revOn ? *p ("rev_mix") * 0.01f : 0.0f);
    if (revOn || revMix.isSmoothing() || revMix.getCurrentValue() > 0.0001f)
    {
        juce::Reverb::Parameters rp;
        rp.roomSize = 0.3f + 0.69f * (*p ("rev_size") * 0.01f);
        rp.damping  = *p ("rev_damp") * 0.01f;
        rp.width    = *p ("rev_width") * 0.01f;
        rp.wetLevel = 1.0f;
        rp.dryLevel = 0.0f;
        reverb.setParameters (rp);

        revBuf.setSize (2, n, false, false, true);
        for (int c = 0; c < nc; ++c) revBuf.copyFrom (c, 0, buffer, c, 0, n);
        if (nc > 1) reverb.processStereo (revBuf.getWritePointer (0), revBuf.getWritePointer (1), n);
        else        reverb.processMono   (revBuf.getWritePointer (0), n);

        for (int i = 0; i < n; ++i)
        {
            const float m = revMix.getNextValue() * 0.35f;
            for (int c = 0; c < nc; ++c) ch[c][i] += m * revBuf.getSample (c, i);
        }
    }
    else
    {
        reverb.reset();
    }

    // --- Sortie -------------------------------------------------------------
    float outPk = 0;
    for (int i = 0; i < n; ++i)
    {
        const float g = outGain.getNextValue();
        for (int c = 0; c < nc; ++c) { ch[c][i] *= g; outPk = std::max (outPk, std::abs (ch[c][i])); }
    }

    for (int c = nc; c < numOut; ++c) buffer.clear (c, 0, n);

    inPeak.store (std::max (inPeak.load(), inPk));
    outPeak.store (std::max (outPeak.load(), outPk));
    compGr.store (comp.grDb);
    deessGr.store (deess.grDb);
}

//==============================================================================
void VoxAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("preset", currentPreset, nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, destData);
}

void VoxAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto state = juce::ValueTree::fromXml (*xml);
            currentPreset = state.getProperty ("preset", 0);
            apvts.replaceState (state);
        }
}

juce::AudioProcessorEditor* VoxAudioProcessor::createEditor() { return new VoxAudioProcessorEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new VoxAudioProcessor(); }
