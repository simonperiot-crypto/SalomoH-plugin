#include "PluginEditor.h"

namespace col
{
    const juce::Colour bg      { 0xff06050b };
    const juce::Colour panel   { 0xff0f0d18 };
    const juce::Colour panelHi { 0xff17142a };
    const juce::Colour line    { 0xff2a2640 };
    const juce::Colour silver  { 0xffdcdee8 };
    const juce::Colour dim     { 0xff7f7b96 };
    const juce::Colour gold    { 0xffe8a800 };
    const juce::Colour goldHi  { 0xffffd56a };
    const juce::Colour violet  { 0xff9b5cff };
    const juce::Colour red     { 0xffff3b5c };
    const juce::Colour cyan    { 0xff4fd8ff };
}

static juce::Font font (float h, bool bold = false, float kerning = 0.0f)
{
    return juce::Font (juce::FontOptions (h, bold ? juce::Font::bold : juce::Font::plain).withKerningFactor (kerning));
}

static juce::String noteName (int midi)
{
    return juce::String (vox::kKeyNames[((midi % 12) + 12) % 12]) + juce::String (midi / 12 - 1);
}

//==============================================================================
VoxLook::VoxLook()
{
    setColour (juce::ComboBox::backgroundColourId, col::panelHi);
    setColour (juce::ComboBox::outlineColourId, col::line);
    setColour (juce::ComboBox::textColourId, col::silver);
    setColour (juce::ComboBox::arrowColourId, col::gold);
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xf2110f1c));
    setColour (juce::PopupMenu::textColourId, col::silver);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, col::gold.withAlpha (0.22f));
    setColour (juce::PopupMenu::highlightedTextColourId, col::goldHi);
    setColour (juce::PopupMenu::headerTextColourId, col::gold);
    setColour (juce::Label::textColourId, col::silver);
    setColour (juce::Slider::textBoxTextColourId, col::silver);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::TextEditor::backgroundColourId, col::panelHi);
    setColour (juce::TextEditor::highlightColourId, col::gold.withAlpha (0.3f));
    setColour (juce::CaretComponent::caretColourId, col::gold);
}

void VoxLook::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                float start, float end, juce::Slider& s)
{
    const auto accent = s.findColour (juce::Slider::rotarySliderFillColourId);
    auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (5);
    const float r = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f;
    const auto c = b.getCentre();
    const float ang = start + pos * (end - start);
    const float trackR = r - 3.0f;

    // graduations
    for (int i = 0; i <= 10; ++i)
    {
        const float a = start + (end - start) * (float) i / 10.0f;
        const auto p = c.getPointOnCircumference (r + 1.5f, a);
        g.setColour ((float) i / 10.0f <= pos + 0.001f ? accent.withAlpha (0.8f) : col::line);
        g.fillEllipse (p.x - 1.0f, p.y - 1.0f, 2.0f, 2.0f);
    }

    juce::Path track;
    track.addCentredArc (c.x, c.y, trackR, trackR, 0, start, end, true);
    g.setColour (juce::Colour (0xff1d1a2c));
    g.strokePath (track, juce::PathStrokeType (3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // arc de valeur (bipolaire si la plage passe par 0)
    float from = start;
    if (s.getMinimum() < 0.0 && s.getMaximum() > 0.0)
        from = start + (float) s.valueToProportionOfLength (0.0) * (end - start);

    juce::Path val;
    val.addCentredArc (c.x, c.y, trackR, trackR, 0, juce::jmin (from, ang), juce::jmax (from, ang), true);
    for (auto [width, alpha] : { std::pair { 10.0f, 0.07f }, { 6.0f, 0.16f } })
    {
        g.setColour (accent.withAlpha (alpha));
        g.strokePath (val, juce::PathStrokeType (width, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    g.setGradientFill (juce::ColourGradient (accent, c.x - r, c.y + r, accent.brighter (0.6f), c.x + r, c.y - r, false));
    g.strokePath (val, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // corps du bouton
    const float kr = r * 0.66f;
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillEllipse (c.x - kr - 1, c.y - kr + 2, kr * 2 + 2, kr * 2 + 2);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff34314a), c.x - kr * 0.4f, c.y - kr,
                                             juce::Colour (0xff0b0a12), c.x + kr * 0.3f, c.y + kr, true));
    g.fillEllipse (c.x - kr, c.y - kr, kr * 2, kr * 2);
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.22f), c.x, c.y - kr,
                                             juce::Colours::transparentWhite, c.x, c.y + kr * 0.2f, false));
    g.drawEllipse (c.x - kr, c.y - kr, kr * 2, kr * 2, 1.0f);

    // aiguille
    const auto p1 = c.getPointOnCircumference (kr * 0.25f, ang);
    const auto p2 = c.getPointOnCircumference (kr * 0.86f, ang);
    g.setColour (accent.withAlpha (0.35f));
    g.drawLine ({ p1, p2 }, 5.0f);
    g.setColour (accent.brighter (0.5f));
    g.drawLine ({ p1, p2 }, 2.0f);
}

void VoxLook::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool hi, bool)
{
    const auto accent = b.findColour (juce::ToggleButton::tickColourId);
    const bool on = b.getToggleState();
    auto r = b.getLocalBounds().toFloat().reduced (1.5f);

    if (b.getProperties()["power"])
    {
        const float d = juce::jmin (r.getWidth(), r.getHeight());
        auto c = r.withSizeKeepingCentre (d, d);
        if (on)
        {
            g.setColour (accent.withAlpha (0.18f));
            g.fillEllipse (c.expanded (2));
        }
        g.setColour (on ? accent.withAlpha (0.16f) : juce::Colour (0xff151322));
        g.fillEllipse (c);
        g.setColour (on ? accent : (hi ? col::dim : col::line));
        g.drawEllipse (c, 1.2f);

        const auto cc = c.getCentre();
        const float ir = d * 0.24f;
        juce::Path icon;
        icon.addCentredArc (cc.x, cc.y, ir, ir, 0, 0.75f, juce::MathConstants<float>::twoPi - 0.75f, true);
        icon.startNewSubPath (cc.x, cc.y - ir * 1.25f);
        icon.lineTo (cc.x, cc.y - ir * 0.15f);
        g.setColour (on ? accent.brighter (0.4f) : col::dim);
        g.strokePath (icon, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        return;
    }

    g.setColour (on ? accent.withAlpha (0.2f) : juce::Colour (0xff151322));
    g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
    g.setColour (on ? accent : (hi ? col::dim : col::line));
    g.drawRoundedRectangle (r, r.getHeight() * 0.5f, 1.0f);
    g.setColour (on ? accent.brighter (0.4f) : col::dim);
    g.setFont (font (10.5f, true, 0.12f));
    g.drawText (b.getButtonText(), r, juce::Justification::centred);
}

void VoxLook::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box)
{
    auto r = juce::Rectangle<float> (0, 0, (float) w, (float) h).reduced (0.5f);
    g.setColour (col::panelHi);
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (box.isMouseOver (true) ? col::gold.withAlpha (0.7f) : col::line.brighter (0.2f));
    g.drawRoundedRectangle (r, 6.0f, 1.0f);

    const float ax = (float) w - 16.0f, ay = (float) h * 0.5f;
    juce::Path arrow;
    arrow.startNewSubPath (ax - 4, ay - 2);
    arrow.lineTo (ax, ay + 2);
    arrow.lineTo (ax + 4, ay - 2);
    g.setColour (col::gold);
    g.strokePath (arrow, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

juce::Font VoxLook::getComboBoxFont (juce::ComboBox&) { return font (13.5f, true); }
juce::Font VoxLook::getPopupMenuFont() { return font (14.0f); }

void VoxLook::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    label.setBounds (10, 1, box.getWidth() - 30, box.getHeight() - 2);
    label.setFont (getComboBoxFont (box));
}

juce::Label* VoxLook::createSliderTextBox (juce::Slider& s)
{
    auto* l = LookAndFeel_V4::createSliderTextBox (s);
    l->setFont (font (11.0f));
    l->setColour (juce::Label::textColourId, col::silver.withAlpha (0.85f));
    l->setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);
    l->setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    l->setJustificationType (juce::Justification::centred);
    return l;
}

void VoxLook::drawPopupMenuBackground (juce::Graphics& g, int w, int h)
{
    g.fillAll (findColour (juce::PopupMenu::backgroundColourId));
    g.setColour (col::gold.withAlpha (0.35f));
    g.drawRect (0, 0, w, h, 1);
}

//==============================================================================
Knob::Knob (juce::AudioProcessorValueTreeState& s, const juce::String& id, const juce::String& n,
            juce::Colour accent, bool isBig)
    : name (n), big (isBig)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 16);
    slider.setColour (juce::Slider::rotarySliderFillColourId, accent);
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    slider.setMouseDragSensitivity (220);
    slider.setVelocityBasedMode (false);
    addAndMakeVisible (slider);
    att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (s, id, slider);
    if (auto* p = s.getParameter (id)) slider.setDoubleClickReturnValue (true, p->convertFrom0to1 (p->getDefaultValue()));
}

void Knob::paint (juce::Graphics& g)
{
    g.setColour (col::dim.brighter (0.25f));
    g.setFont (font (big ? 11.5f : 10.0f, true, 0.14f));
    g.drawText (name.toUpperCase(), getLocalBounds().removeFromTop (big ? 16 : 14), juce::Justification::centred);
}

void Knob::resized()
{
    auto r = getLocalBounds();
    r.removeFromTop (big ? 16 : 14);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, r.getWidth(), big ? 18 : 16);
    slider.setBounds (r);
}

//==============================================================================
PowerButton::PowerButton (juce::AudioProcessorValueTreeState& s, const juce::String& id, juce::Colour accent)
{
    getProperties().set ("power", true);
    setColour (juce::ToggleButton::tickColourId, accent);
    setTooltip ("Activer / couper le module");
    att = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (s, id, *this);
}

//==============================================================================
void GrMeter::tick()
{
    const float v = source.load();
    value += (v - value) * (v > value ? 0.6f : 0.2f);
    repaint();
}

void GrMeter::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    auto text = r.removeFromLeft (24);
    g.setColour (col::dim);
    g.setFont (font (9.5f, true, 0.1f));
    g.drawText ("GR", text, juce::Justification::centredLeft);

    auto bar = r.withSizeKeepingCentre (r.getWidth(), 6.0f);
    g.setColour (juce::Colour (0xff1b1828));
    g.fillRoundedRectangle (bar, 3);
    const float frac = juce::jlimit (0.0f, 1.0f, value / 18.0f);
    if (frac > 0.002f)
    {
        auto fill = bar.withWidth (bar.getWidth() * frac).withRightX (bar.getRight());
        g.setColour (col::red.withAlpha (0.25f));
        g.fillRoundedRectangle (fill.expanded (0, 2), 4);
        g.setColour (col::red);
        g.fillRoundedRectangle (fill, 3);
    }
}

//==============================================================================
void LevelMeter::tick()
{
    const float v = source.exchange (0.0f);
    const float db = vox::gainToDb (v);
    const float norm = juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 63.0f);
    level = norm > level ? norm : level - 0.025f;
    if (norm >= peakHold) { peakHold = norm; holdFrames = 45; }
    else if (--holdFrames < 0) peakHold = juce::jmax (0.0f, peakHold - 0.012f);
    repaint();
}

void LevelMeter::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    auto label = r.removeFromBottom (16);
    g.setColour (col::dim);
    g.setFont (font (9.5f, true, 0.12f));
    g.drawText (text, label, juce::Justification::centred);

    auto bar = r.withSizeKeepingCentre (8.0f, r.getHeight());
    g.setColour (juce::Colour (0xff15121f));
    g.fillRoundedRectangle (bar, 4);

    auto fill = bar.withTop (bar.getBottom() - bar.getHeight() * level);
    juce::ColourGradient grad (col::violet, 0, bar.getBottom(), col::red, 0, bar.getY(), false);
    grad.addColour (0.7, col::gold);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (fill, 4);

    const float py = bar.getBottom() - bar.getHeight() * peakHold;
    g.setColour (peakHold > 0.95f ? col::red : col::goldHi);
    g.fillRect (bar.getX() - 2, py - 1, bar.getWidth() + 4, 2.0f);
}

//==============================================================================
ModuleCard::ModuleCard (juce::AudioProcessorValueTreeState& s, const juce::String& t, const juce::String& powerID,
                        juce::Colour a, int cols)
    : state (s), title (t), accent (a), columns (cols)
{
    power = std::make_unique<PowerButton> (s, powerID, accent);
    addAndMakeVisible (*power);
}

void ModuleCard::addKnob (const juce::String& id, const juce::String& name)
{
    addAndMakeVisible (knobs.add (new Knob (state, id, name, accent)));
}

void ModuleCard::addExtra (juce::Component& c, int)
{
    extras.add (&c);
    addAndMakeVisible (c);
}

int ModuleCard::preferredWidth() const { return columns * 80 + 28; }

void ModuleCard::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (0.5f);
    g.setGradientFill (juce::ColourGradient (col::panelHi.withAlpha (0.92f), 0, 0, col::panel.withAlpha (0.92f), 0, r.getBottom(), false));
    g.fillRoundedRectangle (r, 10);
    g.setGradientFill (juce::ColourGradient (accent.withAlpha (lastOn ? 0.55f : 0.15f), 0, 0, col::line.withAlpha (0.6f), 0, r.getBottom() * 0.6f, false));
    g.drawRoundedRectangle (r, 10, 1.0f);

    // liseré lumineux en haut
    auto top = r.removeFromTop (2.0f).reduced (18, 0);
    g.setGradientFill (juce::ColourGradient (accent.withAlpha (0.0f), top.getX(), 0, accent.withAlpha (0.0f), top.getRight(), 0, false));
    juce::ColourGradient glow (accent.withAlpha (0.0f), top.getX(), 0, accent.withAlpha (0.0f), top.getRight(), 0, false);
    glow.addColour (0.5, accent.withAlpha (lastOn ? 0.9f : 0.2f));
    g.setGradientFill (glow);
    g.fillRect (top);

    g.setColour (lastOn ? col::silver : col::dim);
    g.setFont (font (11.5f, true, columns > 1 ? 0.22f : 0.1f));
    g.drawText (title, 14, 10, getWidth() - 44, 20, juce::Justification::centredLeft);
}

void ModuleCard::resized()
{
    power->setBounds (getWidth() - 34, 8, 24, 24);

    auto area = getLocalBounds().reduced (10, 0).withTrimmedTop (40).withTrimmedBottom (10);
    const int cellW = area.getWidth() / columns, cellH = 100;
    const int rows = (knobs.size() + columns - 1) / columns;
    for (int i = 0; i < knobs.size(); ++i)
        knobs[i]->setBounds (area.getX() + (i % columns) * cellW, area.getY() + (i / columns) * cellH, cellW, cellH);

    if (extras.isEmpty()) return;

    juce::Rectangle<int> spot;
    if (knobs.size() < rows * columns)
    {
        const int i = knobs.size();
        spot = { area.getX() + (i % columns) * cellW, area.getY() + (i / columns) * cellH, cellW, cellH };
        spot = spot.reduced (4, 0).withTrimmedTop (16);
    }
    else
    {
        spot = area.withTrimmedTop (rows * cellH + 6);
    }

    const int itemH = 24, gap = 8;
    const int total = extras.size() * itemH + (extras.size() - 1) * gap;
    auto y = spot.getY() + juce::jmax (0, (juce::jmin (spot.getHeight(), 70) - total) / 2);
    for (auto* e : extras)
    {
        e->setBounds (spot.getX(), y, spot.getWidth(), itemH);
        y += itemH + gap;
    }
}

void ModuleCard::refreshPower()
{
    const bool on = power->getToggleState();
    if (on == lastOn) return;
    lastOn = on;
    for (auto* k : knobs) k->setAlpha (on ? 1.0f : 0.4f);
    for (auto* e : extras) e->setAlpha (on ? 1.0f : 0.4f);
    repaint();
}

//==============================================================================
void PitchView::push (float d, float o)
{
    det[(size_t) head] = d;
    out[(size_t) head] = o;
    head = (head + 1) % N;
    const float ref = o >= 0 ? o : d;
    if (ref >= 0)
    {
        if (! init) { centre = ref; init = true; }
        centre += (ref - centre) * 0.06f;
    }
}

void PitchView::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xff09080f));
    g.fillRoundedRectangle (r, 8);

    const float span = 14.0f, lo = centre - span / 2, hi = centre + span / 2;
    auto yOf = [&] (float m) { return r.getBottom() - (m - lo) / (hi - lo) * r.getHeight(); };
    const float labelW = 38.0f;
    auto plot = r.withTrimmedLeft (labelW);

    {
        juce::Graphics::ScopedSaveState ss (g);
        juce::Path clip;
        clip.addRoundedRectangle (r, 8);
        g.reduceClipRegion (clip);

        for (int n = (int) std::floor (lo); n <= (int) std::ceil (hi); ++n)
        {
            const int pc = ((n % 12) + 12) % 12;
            const bool in = (absMask >> pc) & 1u;
            const bool root = pc == key;
            const float y = yOf ((float) n);
            if (in)
            {
                g.setColour ((root ? col::violet : col::gold).withAlpha (root ? 0.10f : 0.045f));
                g.fillRect (plot.getX(), y - 6, plot.getWidth(), 12.0f);
            }
            g.setColour (root ? col::violet.withAlpha (0.55f) : in ? col::silver.withAlpha (0.16f) : col::silver.withAlpha (0.045f));
            g.drawHorizontalLine ((int) y, plot.getX(), plot.getRight());
            if (in)
            {
                g.setColour (root ? col::violet.brighter (0.3f) : col::dim);
                g.setFont (font (10.0f, root));
                g.drawText (noteName (n), juce::Rectangle<float> (r.getX() + 4, y - 7, labelW - 6, 14), juce::Justification::centredLeft);
            }
        }

        // balayage temps
        g.setColour (col::line.withAlpha (0.5f));
        for (int i = 1; i < 6; ++i)
            g.drawVerticalLine ((int) (plot.getX() + plot.getWidth() * i / 6.0f), r.getY(), r.getBottom());

        auto buildPath = [&] (const std::array<float, N>& data)
        {
            juce::Path path;
            bool pen = false;
            for (int i = 0; i < N; ++i)
            {
                const float v = data[(size_t) ((head + i) % N)];
                const float x = plot.getX() + plot.getWidth() * (float) i / (float) (N - 1);
                if (v < 0) { pen = false; continue; }
                const float y = yOf (v);
                if (! pen) { path.startNewSubPath (x, y); pen = true; }
                else path.lineTo (x, y);
            }
            return path;
        };

        const auto pDet = buildPath (det), pOut = buildPath (out);
        g.setColour (col::cyan.withAlpha (0.45f));
        g.strokePath (pDet, juce::PathStrokeType (1.3f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        for (auto [w, a] : { std::pair { 9.0f, 0.06f }, { 5.0f, 0.14f } })
        {
            g.setColour (col::gold.withAlpha (a));
            g.strokePath (pOut, juce::PathStrokeType (w, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
        g.setColour (col::goldHi);
        g.strokePath (pOut, juce::PathStrokeType (2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        const float last = out[(size_t) ((head + N - 1) % N)];
        if (last >= 0)
        {
            const float y = yOf (last), x = plot.getRight() - 3;
            g.setColour (col::gold.withAlpha (0.25f)); g.fillEllipse (x - 8, y - 8, 16, 16);
            g.setColour (col::goldHi);                 g.fillEllipse (x - 3.5f, y - 3.5f, 7, 7);
        }

        // vignette gauche
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff09080f), plot.getX(), 0, juce::Colour (0x0009080f), plot.getX() + 60, 0, false));
        g.fillRect (plot.withWidth (60));
    }

    // légende
    g.setFont (font (9.5f, true, 0.1f));
    g.setColour (col::cyan.withAlpha (0.8f));
    g.drawText ("VOIX", r.getRight() - 112, r.getY() + 6, 40, 12, juce::Justification::centredRight);
    g.setColour (col::goldHi);
    g.drawText ("CORRIGEE", r.getRight() - 68, r.getY() + 6, 60, 12, juce::Justification::centredRight);

    g.setColour (col::line);
    g.drawRoundedRectangle (r.reduced (0.5f), 8, 1.0f);
}

//==============================================================================
static const int kWhite[7] = { 0, 2, 4, 5, 7, 9, 11 };
static const int kBlack[5] = { 1, 3, 6, 8, 10 };

juce::Rectangle<float> KeyboardView::keyRect (int pc) const
{
    auto r = getLocalBounds().toFloat();
    const float ww = r.getWidth() / 7.0f;
    for (int i = 0; i < 7; ++i)
        if (kWhite[i] == pc) return { r.getX() + i * ww, r.getY(), ww, r.getHeight() };

    const float bw = ww * 0.58f;
    const int after[5] = { 0, 1, 3, 4, 5 }; // touche blanche à gauche
    for (int i = 0; i < 5; ++i)
        if (kBlack[i] == pc) return { r.getX() + (after[i] + 1) * ww - bw / 2, r.getY(), bw, r.getHeight() * 0.6f };
    return {};
}

void KeyboardView::paint (juce::Graphics& g)
{
    auto drawKey = [&] (int pc, bool black)
    {
        auto k = keyRect (pc).reduced (black ? 0.0f : 1.5f, 0.0f);
        const bool in = (absMask >> pc) & 1u, root = pc == key, cur = pc == current;
        const auto accent = root ? col::violet : col::gold;

        juce::Colour base = black ? juce::Colour (0xff0b0a12) : juce::Colour (0xff1d1a2b);
        g.setGradientFill (juce::ColourGradient (base.brighter (0.15f), 0, k.getY(), base, 0, k.getBottom(), false));
        g.fillRoundedRectangle (k, 5);

        if (in)
        {
            g.setColour (accent.withAlpha (cur ? 0.5f : 0.09f));
            g.fillRoundedRectangle (k, 5);
            auto bar = k.removeFromBottom (4).reduced (5, 0);
            g.setColour (accent.withAlpha (0.4f)); g.fillRoundedRectangle (bar.expanded (0, 2), 3);
            g.setColour (accent.brighter (0.3f));  g.fillRoundedRectangle (bar, 2);
        }
        if (cur)
        {
            g.setColour (col::goldHi);
            g.drawRoundedRectangle (keyRect (pc).reduced (black ? 0.5f : 2.0f, 0.5f), 5, 1.6f);
        }

        g.setColour (black ? juce::Colours::black : col::line);
        g.drawRoundedRectangle (keyRect (pc).reduced (black ? 0.0f : 1.5f, 0.0f), 5, 1.0f);

        g.setColour (in ? (root ? col::violet.brighter (0.5f) : col::goldHi) : col::dim.withAlpha (0.6f));
        g.setFont (font (black ? 9.5f : 11.0f, in));
        auto t = keyRect (pc);
        g.drawText (vox::kKeyNames[pc], t.withTrimmedBottom (8).removeFromBottom (16), juce::Justification::centred);
    };

    for (int pc : kWhite) drawKey (pc, false);
    for (int pc : kBlack) drawKey (pc, true);
}

void KeyboardView::mouseDown (const juce::MouseEvent& e)
{
    const auto p = e.position;
    for (int pc : kBlack) if (keyRect (pc).contains (p)) { if (onKeyClicked) onKeyClicked (pc); return; }
    for (int pc : kWhite) if (keyRect (pc).contains (p)) { if (onKeyClicked) onKeyClicked (pc); return; }
}

//==============================================================================
void NoteReadout::set (float d, float t)
{
    det = d;
    if (t >= 0) tgt = t;
    if (d >= 0 && tgt >= 0)
    {
        const float c = (d - tgt) * 100.0f;
        cents += (c - cents) * 0.35f;
    }
    repaint();
}

void NoteReadout::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xff09080f));
    g.fillRoundedRectangle (r, 8);
    g.setColour (col::line);
    g.drawRoundedRectangle (r.reduced (0.5f), 8, 1.0f);

    const bool live = det >= 0;
    auto top = r.reduced (12, 8);
    g.setColour (col::dim);
    g.setFont (font (9.5f, true, 0.2f));
    g.drawText ("NOTE", top.removeFromTop (12), juce::Justification::centredLeft);

    const juce::String name = tgt >= 0 ? noteName ((int) tgt) : "--";
    auto nameArea = top.removeFromTop (46);
    g.setFont (font (40.0f, true));
    if (live)
    {
        g.setColour (col::gold.withAlpha (0.18f));
        g.drawText (name, nameArea.translated (0, 1).expanded (2), juce::Justification::centred);
    }
    g.setColour (live ? col::goldHi : col::dim.withAlpha (0.6f));
    g.drawText (name, nameArea, juce::Justification::centred);

    // aiguille d'accordage -50 / +50 cents
    auto meter = top.removeFromTop (16).reduced (6, 5);
    g.setColour (juce::Colour (0xff1b1828));
    g.fillRoundedRectangle (meter, 3);
    g.setColour (col::silver.withAlpha (0.4f));
    g.fillRect (meter.getCentreX() - 0.5f, meter.getY() - 3, 1.0f, meter.getHeight() + 6);
    if (live)
    {
        const float c = juce::jlimit (-50.0f, 50.0f, cents);
        const float x = meter.getCentreX() + c / 50.0f * meter.getWidth() * 0.5f;
        const auto colr = std::abs (c) < 8 ? col::gold : col::red;
        g.setColour (colr.withAlpha (0.3f)); g.fillEllipse (x - 7, meter.getCentreY() - 7, 14, 14);
        g.setColour (colr);                  g.fillEllipse (x - 3.5f, meter.getCentreY() - 3.5f, 7, 7);
    }
    g.setFont (font (10.0f));
    g.setColour (col::dim);
    g.drawText (live ? juce::String (cents >= 0 ? "+" : "") + juce::String ((int) std::round (cents)) + " ct  (voix brute)" : "en attente de voix",
                top.removeFromTop (14), juce::Justification::centred);
}

//==============================================================================
VoxAudioProcessorEditor::VoxAudioProcessorEditor (VoxAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p),
      inMeter (p.inPeak, "IN"), outMeter (p.outPeak, "OUT"),
      compMeter (p.compGr), dsMeter (p.deessGr)
{
    setLookAndFeel (&look);
    auto& s = proc.apvts;

    // --- En-tête : presets
    for (int i = 0; i < (int) VoxAudioProcessor::presets().size(); ++i)
        presetBox.addItem (VoxAudioProcessor::presets()[(size_t) i].name, i + 1);
    presetBox.setTextWhenNothingSelected ("Presets");
    presetBox.setSelectedId (proc.getCurrentProgram() + 1, juce::dontSendNotification);
    presetBox.onChange = [this] { proc.applyPreset (presetBox.getSelectedId() - 1); };
    addAndMakeVisible (presetBox);

    // --- Autotune
    for (int i = 0; i < 12; ++i) keyBox.addItem (vox::kKeyNamesLong[i], i + 1);
    keyAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (s, "key", keyBox);
    addAndMakeVisible (keyBox);

    juce::PopupMenu* menu = scaleBox.getRootMenu();
    auto addHeader = [&] (const char* t) { menu->addSectionHeader (t); };
    for (int i = 0; i < vox::kNumScales; ++i)
    {
        if (i == 0)  addHeader ("ESSENTIELLES");
        if (i == 5)  addHeader ("MODES");
        if (i == 10) addHeader ("PENTATONIQUES & BLUES");
        if (i == 14) addHeader ("ORIENTALES & MONDE");
        if (i == 25) addHeader ("JAZZ");
        if (i == 30) addHeader ("SYMETRIQUES & EXOTIQUES");
        if (i == 37) addHeader ("ASIE");
        if (i == 42) addHeader ("ACCORDS (EFFET HARD TUNE)");
        scaleBox.addItem (vox::kScales[i].name, i + 1);
    }
    scaleAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (s, "scale", scaleBox);
    addAndMakeVisible (scaleBox);

    tunePower = std::make_unique<PowerButton> (s, "tune_on", col::gold);
    addAndMakeVisible (*tunePower);

    keyboard.onKeyClicked = [this] (int pc)
    {
        if (auto* prm = proc.apvts.getParameter ("key")) prm->setValueNotifyingHost (prm->convertTo0to1 ((float) pc));
    };
    addAndMakeVisible (keyboard);
    addAndMakeVisible (pitchView);
    addAndMakeVisible (readout);

    retune    = std::make_unique<Knob> (s, "retune",    "Retune",    col::gold,   true);
    amount    = std::make_unique<Knob> (s, "amount",    "Intensite", col::gold,   true);
    humanize  = std::make_unique<Knob> (s, "humanize",  "Humanize",  col::violet, true);
    transpose = std::make_unique<Knob> (s, "transpose", "Transpose", col::violet, true);
    retune->slider.setTooltip ("Vitesse de correction : 0 ms = effet robot / hard tune, plus haut = plus naturel");
    amount->slider.setTooltip ("Intensite de la correction : 0 % = voix brute, 100 % = correction totale");
    humanize->slider.setTooltip ("Laisse respirer les notes tenues (vibrato naturel)");
    for (auto* k : { retune.get(), amount.get(), humanize.get(), transpose.get() }) addAndMakeVisible (*k);

    // --- Entrée / sortie
    inKnob  = std::make_unique<Knob> (s, "in_gain",  "Input",  col::silver, true);
    outKnob = std::make_unique<Knob> (s, "out_gain", "Output", col::silver, true);
    addAndMakeVisible (*inKnob);
    addAndMakeVisible (*outKnob);
    addAndMakeVisible (inMeter);
    addAndMakeVisible (outMeter);

    // --- Modules d'effets
    gateCard = std::make_unique<ModuleCard> (s, "GATE", "gate_on", col::cyan, 1);
    gateCard->addKnob ("gate_thr", "Seuil");

    eqCard = std::make_unique<ModuleCard> (s, "EQ", "eq_on", col::cyan, 2);
    eqCard->addKnob ("eq_hpf", "Low Cut");
    eqCard->addKnob ("eq_mud", "Boue");
    eqCard->addKnob ("eq_pres", "Presence");
    eqCard->addKnob ("eq_air", "Air");

    dsCard = std::make_unique<ModuleCard> (s, "DE-ESS", "ds_on", col::cyan, 1);
    dsCard->addKnob ("ds_freq", "Freq");
    dsCard->addKnob ("ds_thr", "Seuil");
    dsCard->addExtra (dsMeter);

    cmpCard = std::make_unique<ModuleCard> (s, "COMPRESSEUR", "cmp_on", col::red, 3);
    cmpCard->addKnob ("cmp_thr", "Seuil");
    cmpCard->addKnob ("cmp_ratio", "Ratio");
    cmpCard->addKnob ("cmp_gain", "Gain");
    cmpCard->addKnob ("cmp_att", "Attaque");
    cmpCard->addKnob ("cmp_rel", "Release");
    cmpCard->addExtra (compMeter);

    satCard = std::make_unique<ModuleCard> (s, "TUBE", "sat_on", col::red, 1);
    satCard->addKnob ("sat_drive", "Drive");
    satCard->addKnob ("sat_mix", "Mix");

    dblCard = std::make_unique<ModuleCard> (s, "DOUBLE", "dbl_on", col::violet, 1);
    dblCard->addKnob ("dbl_mix", "Mix");
    dblCard->addKnob ("dbl_width", "Largeur");

    divBox.addItemList ({ "1/2", "1/4 .", "1/4", "1/8 .", "1/4 T", "1/8", "1/8 T", "1/16" }, 1);
    divAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (s, "dly_div", divBox);
    pingButton.setColour (juce::ToggleButton::tickColourId, col::violet);
    pingAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (s, "dly_ping", pingButton);

    dlyCard = std::make_unique<ModuleCard> (s, "DELAY", "dly_on", col::violet, 2);
    dlyCard->addKnob ("dly_fb", "Repet.");
    dlyCard->addKnob ("dly_tone", "Ton");
    dlyCard->addKnob ("dly_mix", "Mix");
    dlyCard->addExtra (divBox);
    dlyCard->addExtra (pingButton);

    revCard = std::make_unique<ModuleCard> (s, "REVERB", "rev_on", col::violet, 2);
    revCard->addKnob ("rev_size", "Taille");
    revCard->addKnob ("rev_damp", "Douceur");
    revCard->addKnob ("rev_width", "Largeur");
    revCard->addKnob ("rev_mix", "Mix");

    for (auto* c : { gateCard.get(), eqCard.get(), dsCard.get(), cmpCard.get(), satCard.get(), dblCard.get(), dlyCard.get(), revCard.get() })
    {
        cards.add (c);
        addAndMakeVisible (*c);
    }

    setSize (1200, 790);
    startTimerHz (40);
}

VoxAudioProcessorEditor::~VoxAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void VoxAudioProcessorEditor::drawPanel (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour accent) const
{
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.fillRoundedRectangle (r.translated (0, 4), 14);
    g.setGradientFill (juce::ColourGradient (col::panelHi.withAlpha (0.88f), r.getX(), r.getY(), col::panel.withAlpha (0.88f), r.getX(), r.getBottom(), false));
    g.fillRoundedRectangle (r, 14);
    juce::ColourGradient border (accent.withAlpha (0.7f), r.getX(), r.getY(), col::violet.withAlpha (0.25f), r.getRight(), r.getBottom(), false);
    border.addColour (0.5, col::line);
    g.setGradientFill (border);
    g.drawRoundedRectangle (r.reduced (0.5f), 14, 1.2f);
}

void VoxAudioProcessorEditor::renderBackground()
{
    const int w = getWidth(), h = getHeight();
    background = juce::Image (juce::Image::ARGB, w, h, true);
    juce::Graphics g (background);

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff0d0a1c), 0, 0, col::bg, 0, (float) h, false));
    g.fillAll();

    auto glow = [&] (float cx, float cy, float rad, juce::Colour c)
    {
        juce::ColourGradient rg (c, cx, cy, c.withAlpha (0.0f), cx + rad, cy, true);
        g.setGradientFill (rg);
        g.fillEllipse (cx - rad, cy - rad, rad * 2, rad * 2);
    };
    glow (140, 40, 520, col::violet.withAlpha (0.16f));
    glow ((float) w - 120, (float) h - 60, 560, col::gold.withAlpha (0.08f));
    glow ((float) w * 0.55f, 260, 420, col::red.withAlpha (0.05f));

    // étoiles
    juce::Random rnd (2027);
    for (int i = 0; i < 260; ++i)
    {
        const float x = rnd.nextFloat() * w, y = rnd.nextFloat() * h, s = rnd.nextFloat() * 1.4f + 0.3f;
        g.setColour (juce::Colours::white.withAlpha (rnd.nextFloat() * 0.35f + 0.05f));
        g.fillEllipse (x, y, s, s);
    }

    // grille fine
    g.setColour (juce::Colours::white.withAlpha (0.018f));
    for (int x = 0; x < w; x += 24) g.drawVerticalLine (x, 0, (float) h);
    for (int y = 0; y < h; y += 24) g.drawHorizontalLine (y, 0, (float) w);

    // orbite / lune derrière le logo
    g.setColour (col::gold.withAlpha (0.08f));
    g.drawEllipse (-120, -260, 520, 360, 1.0f);
    g.setColour (col::violet.withAlpha (0.07f));
    g.drawEllipse (-60, -220, 460, 300, 1.0f);

    // panneaux
    drawPanel (g, tuneArea.toFloat(), col::gold);
    drawPanel (g, sideArea.toFloat(), col::silver);

    // séparateur d'en-tête
    juce::ColourGradient sep (col::gold.withAlpha (0.0f), 16, 0, col::violet.withAlpha (0.0f), (float) w - 16, 0, false);
    sep.addColour (0.3, col::gold.withAlpha (0.5f));
    sep.addColour (0.7, col::violet.withAlpha (0.4f));
    g.setGradientFill (sep);
    g.fillRect (16, headerArea.getBottom() - 1, w - 32, 1);
}

void VoxAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.drawImageAt (background, 0, 0);

    // logo
    auto h = headerArea.reduced (22, 0);
    const auto fLogo = font (34.0f, true, 0.06f);
    g.setFont (fLogo);
    const int ifmyW = (int) juce::GlyphArrangement::getStringWidth (fLogo, "IFMY");
    auto logo = h.removeFromLeft (ifmyW + 6);
    for (auto [off, a] : { std::pair { 2.0f, 0.12f }, { 1.0f, 0.2f } })
    {
        g.setColour (col::gold.withAlpha (a));
        g.drawText ("IFMY", logo.toFloat().expanded (off), juce::Justification::centredLeft);
    }
    g.setGradientFill (juce::ColourGradient (col::goldHi, 0, (float) logo.getY() + 14, col::gold, 0, (float) logo.getBottom() - 14, false));
    g.drawText ("IFMY", logo, juce::Justification::centredLeft);

    g.setColour (col::silver);
    g.setFont (font (34.0f, false, 0.32f));
    g.drawText ("VOX", h.removeFromLeft (110), juce::Justification::centredLeft);

    g.setColour (col::dim);
    g.setFont (font (10.0f, true, 0.3f));
    g.drawText (juce::String::fromUTF8 ("VOCAL SUITE  \xc2\xb7  AUTOTUNE  \xc2\xb7  FX"), h.removeFromLeft (260).translated (0, 2), juce::Justification::centredLeft);

    g.setColour (col::dim.withAlpha (0.8f));
    g.setFont (font (9.5f, true, 0.2f));
    g.drawText ("PRESET", presetBox.getBounds().translated (-62, 0).withWidth (56), juce::Justification::centredRight);

    // titres de panneaux
    g.setFont (font (13.0f, true, 0.24f));
    g.setColour (tuneOnLast ? col::goldHi : col::dim);
    g.drawText ("AUTOTUNE", tuneArea.getX() + 22, tuneArea.getY() + 16, 140, 26, juce::Justification::centredLeft);
    g.setColour (col::dim);
    g.setFont (font (9.5f, true, 0.2f));
    g.drawText ("TONALITE", keyBox.getBounds().translated (0, -14).withHeight (12), juce::Justification::centredLeft);
    g.drawText ("GAMME", scaleBox.getBounds().translated (0, -14).withHeight (12), juce::Justification::centredLeft);

    g.setFont (font (13.0f, true, 0.24f));
    g.setColour (col::silver);
    g.drawText ("NIVEAUX", sideArea.getX() + 22, sideArea.getY() + 16, 140, 26, juce::Justification::centredLeft);

    // version
    g.setColour (col::dim.withAlpha (0.5f));
    g.setFont (font (9.0f, false, 0.2f));
    g.drawText (juce::String::fromUTF8 ("v1.0  \xc2\xb7  IFMY"), getLocalBounds().removeFromBottom (14).reduced (18, 0), juce::Justification::centredRight);
}

void VoxAudioProcessorEditor::resized()
{
    auto r = getLocalBounds().reduced (16, 0);
    headerArea = r.removeFromTop (72);
    presetBox.setBounds (headerArea.getRight() - 300, headerArea.getCentreY() - 16, 290, 32);

    r.removeFromTop (12);
    auto main = r.removeFromTop (384);
    sideArea = main.removeFromRight (300);
    main.removeFromRight (12);
    tuneArea = main;

    // --- panneau autotune
    auto t = tuneArea.reduced (20, 16);
    auto titleRow = t.removeFromTop (30);
    tunePower->setBounds (tuneArea.getX() + 128, titleRow.getY() + 2, 26, 26);
    titleRow.removeFromLeft (160);
    keyBox.setBounds (titleRow.removeFromLeft (190).withTrimmedTop (2));
    titleRow.removeFromLeft (12);
    scaleBox.setBounds (titleRow.removeFromLeft (300).withTrimmedTop (2));

    t.removeFromTop (14);
    auto right = t.removeFromRight (280);
    t.removeFromRight (16);
    keyboard.setBounds (t.removeFromTop (74));
    t.removeFromTop (12);
    pitchView.setBounds (t);

    readout.setBounds (right.removeFromTop (118));
    right.removeFromTop (6);
    const int kw = right.getWidth() / 2, kh = right.getHeight() / 2;
    retune->setBounds    (right.getX(),      right.getY(),      kw, kh);
    amount->setBounds    (right.getX() + kw, right.getY(),      kw, kh);
    humanize->setBounds  (right.getX(),      right.getY() + kh, kw, kh);
    transpose->setBounds (right.getX() + kw, right.getY() + kh, kw, kh);

    // --- panneau niveaux
    auto sd = sideArea.reduced (18, 16);
    sd.removeFromTop (38);
    auto levels = sd.removeFromTop (150);
    inKnob->setBounds (levels.removeFromLeft (92).withSizeKeepingCentre (92, 112));
    outKnob->setBounds (levels.removeFromRight (92).withSizeKeepingCentre (92, 112));
    auto meters = levels.withSizeKeepingCentre (70, 160);
    inMeter.setBounds (meters.removeFromLeft (35));
    outMeter.setBounds (meters);
    sd.removeFromTop (6);
    gateCard->setBounds (sd.withSizeKeepingCentre (sd.getWidth(), juce::jmin (sd.getHeight(), 162)));

    // --- rangée de modules
    r.removeFromTop (12);
    auto row = r.removeFromTop (290);
    juce::Array<ModuleCard*> rowCards { eqCard.get(), dsCard.get(), cmpCard.get(), satCard.get(), dblCard.get(), dlyCard.get(), revCard.get() };
    int total = 0;
    for (auto* c : rowCards) total += c->preferredWidth();
    const int gap = juce::jmax (6, (row.getWidth() - total) / (rowCards.size() - 1));
    int x = row.getX();
    for (auto* c : rowCards)
    {
        const int w = c == rowCards.getLast() ? row.getRight() - x : c->preferredWidth();
        c->setBounds (x, row.getY(), w, row.getHeight());
        x += w + gap;
    }

    renderBackground();
}

void VoxAudioProcessorEditor::timerCallback()
{
    const float det = proc.tuner.uiDetected.load(), out = proc.tuner.uiOutput.load(), tgt = proc.tuner.uiTarget.load();
    const int key = (int) proc.apvts.getRawParameterValue ("key")->load();
    const int sc = juce::jlimit (0, vox::kNumScales - 1, (int) proc.apvts.getRawParameterValue ("scale")->load());
    const uint16_t mask = vox::absoluteMask (key, vox::kScales[sc].mask);
    const bool tuneOn = proc.apvts.getRawParameterValue ("tune_on")->load() > 0.5f;

    pitchView.absMask = keyboard.absMask = mask;
    pitchView.key = keyboard.key = key;
    pitchView.push (det, out);
    pitchView.repaint();

    const int cur = (det >= 0 && tgt >= 0) ? ((int) tgt) % 12 : -1;
    if (cur != keyboard.current) keyboard.current = cur;
    keyboard.repaint();
    readout.set (det, tgt);

    if (tuneOn != tuneOnLast)
    {
        tuneOnLast = tuneOn;
        for (auto* k : { retune.get(), amount.get(), humanize.get() }) k->setAlpha (tuneOn ? 1.0f : 0.4f);
        repaint (tuneArea);
    }

    inMeter.tick();
    outMeter.tick();
    compMeter.tick();
    dsMeter.tick();
    for (auto* c : cards) c->refreshPower();

    if (presetBox.getSelectedId() != proc.getCurrentProgram() + 1)
        presetBox.setSelectedId (proc.getCurrentProgram() + 1, juce::dontSendNotification);
}
