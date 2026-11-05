#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "AutoTune.h"
#include "Effects.h"

struct VoxPreset
{
    const char* name;
    std::vector<std::pair<const char*, float>> values; // valeurs réelles (pas normalisées)
};

class VoxAudioProcessor : public juce::AudioProcessor
{
public:
    VoxAudioProcessor();
    ~VoxAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "IFMY VOX"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 6.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentPreset; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static const std::vector<VoxPreset>& presets();
    void applyPreset (int index);

    juce::AudioProcessorValueTreeState apvts;

    // Mesures pour l'interface
    std::atomic<float> inPeak { 0 }, outPeak { 0 }, compGr { 0 }, deessGr { 0 };
    vox::AutoTune tuner;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    std::atomic<float>* p (const char* id) { return apvts.getRawParameterValue (id); }

    double sr = 48000;
    int currentPreset = 0;

    vox::Gate gate;
    vox::Biquad hpf[2], mud[2], pres[2], air[2];
    vox::DeEsser deess;
    vox::Compressor comp;
    vox::Saturator sat;
    vox::Doubler dbl;
    vox::StereoDelay dly;
    juce::Reverb reverb;
    juce::AudioBuffer<float> revBuf;

    juce::SmoothedValue<float> inGain, outGain, revMix;
    float lastEq[4] { -1, -1, -1, -1 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoxAudioProcessor)
};
