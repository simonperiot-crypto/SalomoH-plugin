#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"

class SalomohLook : public juce::LookAndFeel_V4
{
public:
    SalomohLook();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float start, float end, juce::Slider&) override;
};

class Tunnel : public juce::Component, private juce::Timer
{
public:
    Tunnel() { startTimerHz (40); }
    void paint (juce::Graphics&) override;
private:
    void timerCallback() override { t += 0.025f; repaint(); }
    float t = 0;
};

class SalomohAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit SalomohAudioProcessorEditor (SalomohAudioProcessor&);
    ~SalomohAudioProcessorEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
    };
    struct Section { juce::String title; std::vector<Knob*> knobs; juce::Rectangle<int> area; };

    Knob* makeKnob (const juce::String& id, const juce::String& name, bool big = false);

    SalomohAudioProcessor& proc;
    SalomohLook look;
    std::vector<std::unique_ptr<Knob>> knobs;
    std::vector<Section> sections;
    juce::ComboBox presetBox, chordBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> chordAtt;
    Tunnel tunnel;
    juce::MidiKeyboardComponent keyboard;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SalomohAudioProcessorEditor)
};
