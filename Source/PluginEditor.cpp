#include "PluginEditor.h"

namespace col
{
    const juce::Colour bg     { 0xff070708 };
    const juce::Colour panel  { 0xff111114 };
    const juce::Colour line   { 0xff2a2a30 };
    const juce::Colour silver { 0xffc9ccd3 };
    const juce::Colour dim    { 0xff7d8089 };
    const juce::Colour gold   { 0xffe8a800 };
    const juce::Colour goldHi { 0xffffd56a };
}

//==============================================================================
SalomohLook::SalomohLook()
{
    setColour (juce::ComboBox::backgroundColourId, col::panel);
    setColour (juce::ComboBox::outlineColourId, col::line);
    setColour (juce::ComboBox::textColourId, col::silver);
    setColour (juce::ComboBox::arrowColourId, col::gold);
    setColour (juce::PopupMenu::backgroundColourId, col::panel);
    setColour (juce::PopupMenu::textColourId, col::silver);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, col::gold.withAlpha (0.25f));
    setColour (juce::Label::textColourId, col::dim);
    setColour (juce::Slider::textBoxTextColourId, col::silver);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
}

void SalomohLook::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                    float start, float end, juce::Slider&)
{
    auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (4);
    const float r = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f;
    const auto c = b.getCentre();
    const float ang = start + pos * (end - start);

    juce::Path track; track.addCentredArc (c.x, c.y, r - 2, r - 2, 0, start, end, true);
    g.setColour (col::line); g.strokePath (track, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path val; val.addCentredArc (c.x, c.y, r - 2, r - 2, 0, start, ang, true);
    g.setGradientFill (juce::ColourGradient (col::gold, c.x - r, c.y + r, col::goldHi, c.x + r, c.y - r, false));
    g.strokePath (val, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const float kr = r * 0.68f;
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2c2d33), c.x, c.y - kr, juce::Colour (0xff0d0d10), c.x, c.y + kr, false));
    g.fillEllipse (c.x - kr, c.y - kr, kr * 2, kr * 2);
    g.setColour (col::line); g.drawEllipse (c.x - kr, c.y - kr, kr * 2, kr * 2, 1.0f);

    juce::Path tick; tick.addRoundedRectangle (-1.2f, -kr + 3, 2.4f, kr * 0.45f, 1.2f);
    g.setColour (col::goldHi);
    g.fillPath (tick, juce::AffineTransform::rotation (ang).translated (c));
}

//==============================================================================
void Tunnel::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xff0a0a0c)); g.fillRoundedRectangle (b, 8);
    const auto c = b.getCentre();
    for (int i = 0; i < 14; ++i)
    {
        float k = std::fmod (i / 14.0f + t * 0.35f, 1.0f);
        float depth = k * k;
        juce::Path p;
        const int pts = 90;
        for (int j = 0; j <= pts; ++j)
        {
            float a = j / (float) pts * juce::MathConstants<float>::twoPi;
            float wob = 1.0f + 0.08f * std::sin (a * 5 + t * 3 + i) ;
            float rx = b.getWidth() * 0.48f * depth * wob, ry = b.getHeight() * 0.46f * depth * wob;
            juce::Point<float> pt (c.x + std::cos (a) * rx, c.y + std::sin (a) * ry);
            j == 0 ? p.startNewSubPath (pt) : p.lineTo (pt);
        }
        g.setColour ((i % 3 == 0 ? col::gold : col::silver).withAlpha (0.15f + 0.7f * depth));
        g.strokePath (p, juce::PathStrokeType (0.6f + depth * 1.6f));
    }
    g.setColour (col::line); g.drawRoundedRectangle (b.reduced (0.5f), 8, 1);
}

//==============================================================================
SalomohAudioProcessorEditor::SalomohAudioProcessorEditor (SalomohAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p),
      keyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel (&look);

    sections = {
        { "MACROS",       { makeKnob ("brillance", "BRILLANCE", true), makeKnob ("espace", "ESPACE", true),
                            makeKnob ("mouvement", "MOUVEMENT", true), makeKnob ("grain", "GRAIN", true) } },
        { "OSCILLATEUR",  { makeKnob ("wave", "FORME"), makeKnob ("detune", "DETUNE"), makeKnob ("sub", "SUB"), makeKnob ("noise", "BRUIT") } },
        { "FILTRE",       { makeKnob ("cutoff", "CUTOFF"), makeKnob ("reso", "RESO") } },
        { "ENVELOPPE",    { makeKnob ("attack", "A"), makeKnob ("decay", "D"), makeKnob ("sustain", "S"), makeKnob ("release", "R") } },
        { "LFO",          { makeKnob ("lfoRate", "RATE"), makeKnob ("lfoAmt", "AMOUNT") } },
        { "EFFETS",       { makeKnob ("chorusMix", "CHORUS"), makeKnob ("delayMix", "DELAY"), makeKnob ("delayTime", "TEMPS"), makeKnob ("master", "VOLUME") } },
    };

    for (int i = 0; i < proc.getNumPrograms(); ++i) presetBox.addItem (proc.getProgramName (i), i + 1);
    presetBox.setSelectedId (proc.getCurrentProgram() + 1, juce::dontSendNotification);
    presetBox.onChange = [this] { proc.setCurrentProgram (presetBox.getSelectedId() - 1); };
    addAndMakeVisible (presetBox);

    chordBox.addItemList ({ "Accord : Off", "Majeur", "Mineur", "Maj7", "Min7", "Sus2", "Sus4", "Power" }, 1);
    chordAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, "chord", chordBox);
    addAndMakeVisible (chordBox);

    addAndMakeVisible (tunnel);

    keyboard.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, col::gold.withAlpha (0.7f));
    keyboard.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, col::gold.withAlpha (0.25f));
    keyboard.setColour (juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colour (0xffd9dbe0));
    keyboard.setColour (juce::MidiKeyboardComponent::blackNoteColourId, juce::Colour (0xff121214));
    keyboard.setLowestVisibleKey (36);
    addAndMakeVisible (keyboard);

    setResizable (true, true);
    setResizeLimits (900, 560, 1800, 1120);
    getConstrainer()->setFixedAspectRatio (1100.0 / 680.0);
    setSize (1100, 680);
}

SalomohAudioProcessorEditor::~SalomohAudioProcessorEditor() { setLookAndFeel (nullptr); }

SalomohAudioProcessorEditor::Knob* SalomohAudioProcessorEditor::makeKnob (const juce::String& id, const juce::String& name, bool)
{
    auto k = std::make_unique<Knob>();
    k->slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k->slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 64, 14);
    k->slider.setPopupDisplayEnabled (false, false, nullptr);
    k->att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, id, k->slider);
    k->label.setText (name, juce::dontSendNotification);
    k->label.setJustificationType (juce::Justification::centred);
    k->label.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    addAndMakeVisible (k->slider);
    addAndMakeVisible (k->label);
    knobs.push_back (std::move (k));
    return knobs.back().get();
}

void SalomohAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (col::bg);
    const float s = getWidth() / 1100.0f;

    // Logo
    g.setGradientFill (juce::ColourGradient (juce::Colours::white, 24 * s, 0, col::silver, 300 * s, 60 * s, false));
    g.setFont (juce::FontOptions (34.0f * s, juce::Font::bold));
    g.drawText ("SALOMOH", juce::Rectangle<float> (24 * s, 12 * s, 320 * s, 44 * s), juce::Justification::centredLeft);
    g.setColour (col::gold);
    g.setFont (juce::FontOptions (11.0f * s, juce::Font::bold));
    g.drawText ("BY IFMY  ·  V0.1", juce::Rectangle<float> (26 * s, 52 * s, 300 * s, 16 * s), juce::Justification::centredLeft);

    for (auto& sec : sections)
    {
        auto a = sec.area.toFloat();
        g.setColour (col::panel); g.fillRoundedRectangle (a, 8 * s);
        g.setColour (col::line); g.drawRoundedRectangle (a.reduced (0.5f), 8 * s, 1);
        g.setColour (sec.title == "MACROS" ? col::gold : col::silver);
        g.setFont (juce::FontOptions (11.0f * s, juce::Font::bold));
        g.drawText (sec.title, a.reduced (12 * s, 6 * s).removeFromTop (16 * s), juce::Justification::centredLeft);
    }
}

void SalomohAudioProcessorEditor::resized()
{
    const float s = getWidth() / 1100.0f;
    auto S = [s] (float v) { return juce::roundToInt (v * s); };
    auto r = getLocalBounds().reduced (S (16));

    auto header = r.removeFromTop (S (64));
    chordBox.setBounds (header.removeFromRight (S (170)).withSizeKeepingCentre (S (170), S (30)));
    header.removeFromRight (S (10));
    presetBox.setBounds (header.removeFromRight (S (240)).withSizeKeepingCentre (S (240), S (30)));

    keyboard.setBounds (r.removeFromBottom (S (96)));
    keyboard.setKeyWidth (keyboard.getWidth() / 36.0f);
    r.removeFromBottom (S (12));

    auto top = r.removeFromTop (r.getHeight() / 2 - S (6));
    r.removeFromTop (S (12));
    auto bottom = r;

    auto macroArea = top.removeFromLeft (S (360));
    top.removeFromLeft (S (12));
    auto rightTop = top.removeFromRight (S (300));
    top.removeFromRight (S (12));
    tunnel.setBounds (top);

    auto oscArea = rightTop;
    const int bw = (bottom.getWidth() - S (36)) / 4;
    std::vector<juce::Rectangle<int>> areas { macroArea, oscArea };
    for (int i = 0; i < 4; ++i) { areas.push_back (bottom.removeFromLeft (bw)); bottom.removeFromLeft (S (12)); }

    for (size_t i = 0; i < sections.size(); ++i)
    {
        auto& sec = sections[i];
        sec.area = areas[i];
        auto inner = sec.area.reduced (S (10)).withTrimmedTop (S (18));
        const int n = (int) sec.knobs.size();
        const int cols = (i == 0 || i == 1) ? 2 : n;
        const int rows = (n + cols - 1) / cols;
        const int cw = inner.getWidth() / cols, ch = inner.getHeight() / rows;
        for (int k = 0; k < n; ++k)
        {
            juce::Rectangle<int> cell (inner.getX() + (k % cols) * cw, inner.getY() + (k / cols) * ch, cw, ch);
            auto* kn = sec.knobs[(size_t) k];
            kn->label.setFont (juce::FontOptions (10.5f * s, juce::Font::bold));
            kn->label.setBounds (cell.removeFromBottom (S (16)));
            kn->slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, S (64), S (14));
            kn->slider.setBounds (cell.reduced (S (2)));
        }
    }
}
