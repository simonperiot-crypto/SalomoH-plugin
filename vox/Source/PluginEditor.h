#pragma once
#include "PluginProcessor.h"

//==============================================================================
class VoxLook : public juce::LookAndFeel_V4
{
public:
    VoxLook();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float start, float end, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool down, int bx, int by, int bw, int bh, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
    juce::Font getPopupMenuFont() override;
    juce::Label* createSliderTextBox (juce::Slider&) override;
    void drawPopupMenuBackground (juce::Graphics&, int w, int h) override;
};

//==============================================================================
class Knob : public juce::Component
{
public:
    Knob (juce::AudioProcessorValueTreeState&, const juce::String& paramID, const juce::String& name,
          juce::Colour accent, bool big = false);
    void paint (juce::Graphics&) override;
    void resized() override;
    juce::Slider slider;

private:
    juce::String name;
    bool big;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
};

//==============================================================================
class PowerButton : public juce::ToggleButton
{
public:
    PowerButton (juce::AudioProcessorValueTreeState&, const juce::String& paramID, juce::Colour accent);
private:
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> att;
};

//==============================================================================
class GrMeter : public juce::Component
{
public:
    explicit GrMeter (std::atomic<float>& src) : source (src) {}
    void tick();
    void paint (juce::Graphics&) override;
private:
    std::atomic<float>& source;
    float value = 0;
};

//==============================================================================
class LevelMeter : public juce::Component
{
public:
    LevelMeter (std::atomic<float>& src, juce::String label) : source (src), text (std::move (label)) {}
    void tick();
    void paint (juce::Graphics&) override;
private:
    std::atomic<float>& source;
    juce::String text;
    float level = 0, peakHold = 0;
    int holdFrames = 0;
};

//==============================================================================
class ModuleCard : public juce::Component
{
public:
    ModuleCard (juce::AudioProcessorValueTreeState&, const juce::String& title, const juce::String& powerID,
                juce::Colour accent, int columns);
    void addKnob (const juce::String& paramID, const juce::String& name);
    void addExtra (juce::Component& c, int heightRows = 0);
    void paint (juce::Graphics&) override;
    void resized() override;
    void refreshPower();
    int preferredWidth() const;

    juce::OwnedArray<Knob> knobs;
    juce::Array<juce::Component*> extras;

private:
    juce::AudioProcessorValueTreeState& state;
    juce::String title;
    juce::Colour accent;
    int columns;
    std::unique_ptr<PowerButton> power;
    bool lastOn = true;
};

//==============================================================================
class PitchView : public juce::Component
{
public:
    static constexpr int N = 300;
    void push (float detected, float output);
    void paint (juce::Graphics&) override;
    uint16_t absMask = 0x0FFF;
    int key = 0;
private:
    std::array<float, N> det {}, out {};
    int head = 0;
    float centre = 60.0f;
    bool init = false;
};

//==============================================================================
class KeyboardView : public juce::Component
{
public:
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    std::function<void (int)> onKeyClicked;
    uint16_t absMask = 0x0FFF;
    int key = 0, current = -1;
private:
    juce::Rectangle<float> keyRect (int pc) const;
};

//==============================================================================
class NoteReadout : public juce::Component
{
public:
    void set (float detected, float target);
    void paint (juce::Graphics&) override;
private:
    float det = -1, tgt = -1, cents = 0;
};

//==============================================================================
class VoxAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit VoxAudioProcessorEditor (VoxAudioProcessor&);
    ~VoxAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void renderBackground();
    void drawPanel (juce::Graphics&, juce::Rectangle<float>, juce::Colour accent) const;

    VoxAudioProcessor& proc;
    VoxLook look;
    juce::Image background;

    juce::ComboBox presetBox, keyBox, scaleBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> keyAtt, scaleAtt;
    std::unique_ptr<PowerButton> tunePower;

    KeyboardView keyboard;
    PitchView pitchView;
    NoteReadout readout;
    std::unique_ptr<Knob> retune, amount, humanize, transpose, inKnob, outKnob;
    std::unique_ptr<ModuleCard> gateCard, eqCard, dsCard, cmpCard, satCard, dblCard, dlyCard, revCard;
    juce::Array<ModuleCard*> cards;

    LevelMeter inMeter, outMeter;
    GrMeter compMeter, dsMeter;
    juce::ComboBox divBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> divAtt;
    juce::ToggleButton pingButton { "PING-PONG" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> pingAtt;

    juce::Rectangle<int> tuneArea, sideArea, headerArea;
    bool tuneOnLast = true;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoxAudioProcessorEditor)
};
