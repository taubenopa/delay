#include "PluginEditor.h"

namespace
{
    const juce::Colour kInk    (0xff3b2f22);   // dunkelbraune "Gravur"
    const juce::Colour kCream  (0xffe9dfc4);
    const juce::Colour kOrange (0xffff7a3a);

    const char* kIds[10]   = { "time", "feedback", "mix", "tone", "lowcut",
                               "drive", "wow", "rate", "noise", "output" };
    const char* kNames[10] = { "TIME", "FEEDBACK", "MIX", "TONE", "LOW CUT",
                               "DRIVE", "WOW", "RATE", "NOISE", "OUTPUT" };
    const int kRow[10]     = { 0, 0, 0, 0, 0, 1, 1, 1, 1, 1 };
    const int kCol[10]     = { 0, 1, 2, 3, 4, 0, 1, 2, 3, 4 };

    void paintWood (juce::Graphics& g, juce::Rectangle<int> r, int seed)
    {
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff5e381a), (float) r.getX(), 0.0f,
                                                 juce::Colour (0xff3a2210), (float) r.getRight(), 0.0f, false));
        g.fillRect (r);

        juce::Random rnd (seed);
        for (int i = 0; i < 70; ++i)
        {
            const float x = (float) r.getX() + rnd.nextFloat() * (float) r.getWidth();
            const float w = 0.6f + rnd.nextFloat() * 1.6f;
            const juce::uint32 col = rnd.nextBool() ? 0xff2a170aU : 0xff8a5a30U;
            g.setColour (juce::Colour (col).withAlpha (0.10f + rnd.nextFloat() * 0.22f));
            g.fillRect (x, (float) r.getY(), w, (float) r.getHeight());
        }
    }

    void paintScrew (juce::Graphics& g, float cx, float cy, float angle)
    {
        const float r = 6.5f;
        g.setColour (juce::Colours::black.withAlpha (0.35f));
        g.fillEllipse (cx - r + 1.0f, cy - r + 1.5f, r * 2.0f, r * 2.0f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xffd9d4c8), cx - 3.0f, cy - 3.0f,
                                                 juce::Colour (0xff6e6a62), cx + 4.0f, cy + 5.0f, true));
        g.fillEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f);
        g.setColour (juce::Colour (0xff2a2824));
        g.drawEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f, 0.8f);
        const float dx = std::cos (angle) * (r - 1.5f);
        const float dy = std::sin (angle) * (r - 1.5f);
        g.setColour (juce::Colour (0xff23211e));
        g.drawLine (cx - dx, cy - dy, cx + dx, cy + dy, 1.8f);
    }

    void drawCaption (juce::Graphics& g, juce::Rectangle<int> span, const juce::String& text)
    {
        const int gap = text.length() * 8 + 14;
        const int cy  = span.getY() + 7;
        const int mid = span.getCentreX();

        g.setColour (kInk.withAlpha (0.55f));
        g.drawHorizontalLine (cy, (float) span.getX() + 10.0f, (float) (mid - gap / 2));
        g.drawHorizontalLine (cy, (float) (mid + gap / 2), (float) span.getRight() - 10.0f);

        g.setColour (kInk.withAlpha (0.85f));
        g.setFont (VintageLookAndFeel::uiFont (10.0f));
        g.drawText (text, mid - gap / 2, span.getY(), gap, 14, juce::Justification::centred);
    }

    void drawEngraved (juce::Graphics& g, const juce::String& text, juce::Rectangle<int> r,
                       juce::Justification j)
    {
        g.setColour (juce::Colours::white.withAlpha (0.65f));
        g.drawText (text, r.translated (0, 1), j);
        g.setColour (kInk);
        g.drawText (text, r, j);
    }
}

//==============================================================================
VintageLookAndFeel::VintageLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId, kInk);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxHighlightColourId, kOrange.withAlpha (0.5f));

    setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xfff3ead3));
    setColour (juce::TextEditor::textColourId, kInk);
    setColour (juce::TextEditor::outlineColourId, kInk.withAlpha (0.6f));
    setColour (juce::TextEditor::focusedOutlineColourId, kOrange);
    setColour (juce::TextEditor::highlightColourId, kOrange.withAlpha (0.4f));

    setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff2b2620));
    setColour (juce::ComboBox::textColourId, kCream);
    setColour (juce::ComboBox::outlineColourId, juce::Colour (0xff0d0b09));
    setColour (juce::ComboBox::arrowColourId, kOrange);
    setColour (juce::ComboBox::buttonColourId, juce::Colour (0xff2b2620));

    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff2b2620));
    setColour (juce::PopupMenu::textColourId, kCream);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, kOrange);
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colour (0xff1a1008));
}

juce::Font VintageLookAndFeel::uiFont (float size, bool bold)
{
    return juce::Font (juce::FontOptions ("Georgia", size, bold ? juce::Font::bold : juce::Font::plain));
}

juce::Font VintageLookAndFeel::getLabelFont (juce::Label&)        { return uiFont (13.0f); }
juce::Font VintageLookAndFeel::getComboBoxFont (juce::ComboBox&)  { return uiFont (12.0f); }
juce::Font VintageLookAndFeel::getPopupMenuFont()                 { return uiFont (13.0f); }

void VintageLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                           float sliderPos, float startAngle, float endAngle,
                                           juce::Slider& slider)
{
    const float alpha  = slider.isEnabled() ? 1.0f : 0.4f;
    auto bounds        = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (4.0f);
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto c       = bounds.getCentre();
    const float angle  = startAngle + sliderPos * (endAngle - startAngle);

    // Skalenstriche
    g.setColour (kInk.withAlpha (alpha));
    for (int i = 0; i <= 10; ++i)
    {
        const float a  = startAngle + (float) i / 10.0f * (endAngle - startAngle);
        const float r1 = radius;
        const float r2 = radius - ((i % 5 == 0) ? 9.0f : 5.0f);
        g.drawLine (c.x + r1 * std::sin (a), c.y - r1 * std::cos (a),
                    c.x + r2 * std::sin (a), c.y - r2 * std::cos (a),
                    (i % 5 == 0) ? 2.0f : 1.2f);
    }

    const float kr = radius * 0.68f;

    // Schatten
    g.setColour (juce::Colours::black.withAlpha (0.35f * alpha));
    g.fillEllipse (c.x - kr + 2.5f, c.y - kr + 4.0f, kr * 2.0f, kr * 2.0f);

    // Knopfkoerper (Bakelit)
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff4c4742).withAlpha (alpha), c.x - kr * 0.5f, c.y - kr * 0.6f,
                                             juce::Colour (0xff0e0d0c).withAlpha (alpha), c.x + kr * 0.6f, c.y + kr * 0.8f, true));
    g.fillEllipse (c.x - kr, c.y - kr, kr * 2.0f, kr * 2.0f);

    // Rillen am Rand
    g.setColour (juce::Colours::white.withAlpha (0.10f * alpha));
    for (int i = 0; i < 48; ++i)
    {
        const float a = (float) i * juce::MathConstants<float>::twoPi / 48.0f;
        g.drawLine (c.x + kr * 0.90f * std::sin (a), c.y - kr * 0.90f * std::cos (a),
                    c.x + kr * std::sin (a),         c.y - kr * std::cos (a), 1.0f);
    }

    // Kappe
    const float cr = kr * 0.72f;
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3b3631).withAlpha (alpha), c.x - cr * 0.4f, c.y - cr * 0.5f,
                                             juce::Colour (0xff151311).withAlpha (alpha), c.x + cr * 0.5f, c.y + cr * 0.6f, true));
    g.fillEllipse (c.x - cr, c.y - cr, cr * 2.0f, cr * 2.0f);
    g.setColour (juce::Colours::white.withAlpha (0.14f * alpha));
    g.drawEllipse (c.x - cr, c.y - cr, cr * 2.0f, cr * 2.0f, 1.0f);

    // Zeiger
    juce::Path pointer;
    pointer.addRoundedRectangle (-1.8f, -kr + 3.0f, 3.6f, kr * 0.55f, 1.5f);
    pointer.applyTransform (juce::AffineTransform::rotation (angle).translated (c.x, c.y));
    g.setColour (juce::Colour (0xfff3ead3).withAlpha (alpha));
    g.fillPath (pointer);
}

void VintageLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b,
                                           bool highlighted, bool)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.0f);

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3d352c), r.getX(), r.getY(),
                                             juce::Colour (0xff1f1b17), r.getX(), r.getBottom(), false));
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (juce::Colour (0xff0d0b09));
    g.drawRoundedRectangle (r, 5.0f, 1.5f);

    if (highlighted)
    {
        g.setColour (juce::Colours::white.withAlpha (0.06f));
        g.fillRoundedRectangle (r, 5.0f);
    }

    const bool on = b.getToggleState();
    const juce::Rectangle<float> led (r.getX() + 8.0f, r.getCentreY() - 5.0f, 10.0f, 10.0f);

    if (on)
    {
        g.setGradientFill (juce::ColourGradient (juce::Colour (0x66ff6a2a), led.getCentreX(), led.getCentreY(),
                                                 juce::Colours::transparentBlack, led.getCentreX() + 14.0f, led.getCentreY(), true));
        g.fillEllipse (led.expanded (10.0f));
        g.setColour (kOrange);
    }
    else
    {
        g.setColour (juce::Colour (0xff4a2a1c));
    }
    g.fillEllipse (led);
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.drawEllipse (led, 1.0f);

    g.setColour (kCream.withAlpha (b.isEnabled() ? 1.0f : 0.5f));
    g.setFont (uiFont (11.5f));
    g.drawText (b.getButtonText(), (int) (r.getX() + 24.0f), (int) r.getY(),
                (int) (r.getWidth() - 28.0f), (int) r.getHeight(), juce::Justification::centredLeft);
}

//==============================================================================
SimpleDelayEditor::SimpleDelayEditor (SimpleDelayProcessor& p)
    : juce::AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&lnf);

    for (size_t i = 0; i < knobs.size(); ++i)
    {
        auto& s = knobs[i].slider;
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 90, 20);
        s.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f,
                               juce::MathConstants<float>::pi * 2.8f, true);
        knobs[i].attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            proc.apvts, kIds[i], s);
        addAndMakeVisible (s);
    }

    // Anzeigeformate (nach dem Attachment setzen, sonst werden sie ueberschrieben)
    auto percentText  = [] (double v) { return juce::String (juce::roundToInt (v * 100.0)) + " %"; };
    auto percentParse = [] (const juce::String& t) { return t.getDoubleValue() / 100.0; };

    knobs[0].slider.textFromValueFunction = [] (double v) { return juce::String (juce::roundToInt (v)) + " ms"; };
    knobs[0].slider.valueFromTextFunction = [] (const juce::String& t) { return t.getDoubleValue(); };

    for (int i : { 1, 2, 5, 6, 8 })
    {
        knobs[(size_t) i].slider.textFromValueFunction = percentText;
        knobs[(size_t) i].slider.valueFromTextFunction = percentParse;
    }

    knobs[3].slider.textFromValueFunction = [] (double v)
    {
        return v >= 1000.0 ? juce::String (v / 1000.0, 1) + " kHz" : juce::String (juce::roundToInt (v)) + " Hz";
    };
    knobs[3].slider.valueFromTextFunction = [] (const juce::String& t)
    {
        return t.containsIgnoreCase ("k") ? t.getDoubleValue() * 1000.0 : t.getDoubleValue();
    };

    knobs[4].slider.textFromValueFunction = [] (double v) { return juce::String (juce::roundToInt (v)) + " Hz"; };
    knobs[4].slider.valueFromTextFunction = [] (const juce::String& t) { return t.getDoubleValue(); };

    knobs[7].slider.textFromValueFunction = [] (double v) { return juce::String (v, 1) + " Hz"; };
    knobs[7].slider.valueFromTextFunction = [] (const juce::String& t) { return t.getDoubleValue(); };

    knobs[9].slider.textFromValueFunction = [] (double v) { return juce::String (v, 1) + " dB"; };
    knobs[9].slider.valueFromTextFunction = [] (const juce::String& t) { return t.getDoubleValue(); };

    for (auto& k : knobs)
        k.slider.updateText();

    syncButton.setButtonText ("SYNC");
    pingPongButton.setButtonText ("PING-PONG");
    addAndMakeVisible (syncButton);
    addAndMakeVisible (pingPongButton);
    syncAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "sync", syncButton);
    pingAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "pingpong", pingPongButton);

    if (auto* dp = dynamic_cast<juce::AudioParameterChoice*> (proc.apvts.getParameter ("division")))
        divisionBox.addItemList (dp->choices, 1);
    divisionAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        proc.apvts, "division", divisionBox);
    addAndMakeVisible (divisionBox);

    setSize (800, 470);
    startTimerHz (30);
}

SimpleDelayEditor::~SimpleDelayEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

juce::Rectangle<int> SimpleDelayEditor::cell (int row, int col)
{
    return { 50 + col * 140, row == 0 ? 96 : 282, 140, 176 };
}

void SimpleDelayEditor::timerCallback()
{
    const bool sync = proc.apvts.getRawParameterValue ("sync")->load() > 0.5f;
    if (knobs[0].slider.isEnabled() == sync)  knobs[0].slider.setEnabled (! sync);
    if (divisionBox.isEnabled() != sync)      divisionBox.setEnabled (sync);

    const double delayMs = juce::jmax (20.0, (double) proc.currentDelayMs.load());
    const bool on = std::fmod (juce::Time::getMillisecondCounterHiRes(), delayMs) < juce::jmin (90.0, delayMs * 0.3);
    if (on != ledOn)
    {
        ledOn = on;
        repaint (ledBounds().expanded (14));
    }
}

void SimpleDelayEditor::paint (juce::Graphics& g)
{
    const int w = getWidth();
    const int h = getHeight();

    g.fillAll (juce::Colour (0xff1a1008));

    // Holzwangen
    paintWood (g, { 0, 0, 32, h }, 11);
    paintWood (g, { w - 32, 0, 32, h }, 29);

    // Frontplatte
    const juce::Rectangle<int> plate (32, 0, w - 64, h);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffeadfc4), 0.0f, 0.0f,
                                             juce::Colour (0xffd3c5a0), 0.0f, (float) h, false));
    g.fillRect (plate);

    juce::Random rnd (7);
    for (int i = 0; i < 260; ++i)
    {
        const float y   = rnd.nextFloat() * (float) h;
        const float x   = (float) plate.getX() + rnd.nextFloat() * (float) plate.getWidth() * 0.8f;
        const float len = 40.0f + rnd.nextFloat() * 260.0f;
        g.setColour ((rnd.nextBool() ? juce::Colours::white : juce::Colour (0xff7a6a4a)).withAlpha (0.03f + rnd.nextFloat() * 0.04f));
        g.fillRect (x, y, len, 1.0f);
    }

    // Kantenschatten
    g.setGradientFill (juce::ColourGradient (juce::Colours::black.withAlpha (0.40f), 32.0f, 0.0f,
                                             juce::Colours::transparentBlack, 46.0f, 0.0f, false));
    g.fillRect (32, 0, 14, h);
    g.setGradientFill (juce::ColourGradient (juce::Colours::black.withAlpha (0.40f), (float) (w - 32), 0.0f,
                                             juce::Colours::transparentBlack, (float) (w - 46), 0.0f, false));
    g.fillRect (w - 46, 0, 14, h);

    // Schrauben
    paintScrew (g, 52.0f, 16.0f, 0.4f);
    paintScrew (g, (float) w - 52.0f, 16.0f, 2.1f);
    paintScrew (g, 52.0f, (float) h - 16.0f, 1.3f);
    paintScrew (g, (float) w - 52.0f, (float) h - 16.0f, 0.9f);

    // Titel
    g.setFont (juce::Font (juce::FontOptions ("Georgia", 32.0f, juce::Font::bold)));
    drawEngraved (g, "ANALOG ECHO", { 62, 14, 340, 40 }, juce::Justification::centredLeft);
    g.setFont (VintageLookAndFeel::uiFont (10.0f, false));
    drawEngraved (g, "ANALOG DELAY  -  BUCKET BRIGADE STYLE", { 64, 54, 340, 16 }, juce::Justification::centredLeft);

    // Trennlinie
    g.setColour (kInk.withAlpha (0.35f));
    g.drawHorizontalLine (84, 56.0f, (float) w - 56.0f);
    g.setColour (juce::Colours::white.withAlpha (0.5f));
    g.drawHorizontalLine (85, 56.0f, (float) w - 56.0f);
    g.setColour (kInk.withAlpha (0.25f));
    g.drawHorizontalLine (276, 56.0f, (float) w - 56.0f);

    // Gruppen-Beschriftung
    drawCaption (g, cell (0, 0).getUnion (cell (0, 2)), "DELAY");
    drawCaption (g, cell (0, 3).getUnion (cell (0, 4)), "FILTER");
    drawCaption (g, cell (1, 0), "SATURATION");
    drawCaption (g, cell (1, 1).getUnion (cell (1, 3)), "MODULATION");
    drawCaption (g, cell (1, 4), "MASTER");

    // Regler-Namen
    g.setFont (VintageLookAndFeel::uiFont (12.5f));
    for (int i = 0; i < 10; ++i)
    {
        auto c = cell (kRow[i], kCol[i]);
        drawEngraved (g, kNames[i], { c.getX(), c.getY() + 15, c.getWidth(), 18 }, juce::Justification::centred);
    }

    // Echo-LED
    const auto led = ledBounds().toFloat();
    if (ledOn)
    {
        g.setGradientFill (juce::ColourGradient (juce::Colour (0x88ff3a1a), led.getCentreX(), led.getCentreY(),
                                                 juce::Colours::transparentBlack, led.getCentreX() + 22.0f, led.getCentreY(), true));
        g.fillEllipse (led.expanded (14.0f));
    }
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillEllipse (led.expanded (3.0f));
    g.setGradientFill (juce::ColourGradient (ledOn ? juce::Colour (0xffff8a60) : juce::Colour (0xff5a1c12),
                                             led.getCentreX() - 3.0f, led.getCentreY() - 3.0f,
                                             ledOn ? juce::Colour (0xffd01808) : juce::Colour (0xff2a0a06),
                                             led.getRight(), led.getBottom(), true));
    g.fillEllipse (led);
    g.setFont (VintageLookAndFeel::uiFont (9.0f));
    drawEngraved (g, "ECHO", { ledBounds().getX() - 11, ledBounds().getBottom() + 4, 40, 12 },
                  juce::Justification::centred);
}

void SimpleDelayEditor::resized()
{
    for (size_t i = 0; i < knobs.size(); ++i)
    {
        auto c = cell (kRow[i], kCol[i]);
        knobs[i].slider.setBounds (c.withTrimmedTop (32).reduced (4, 0));
    }

    syncButton.setBounds (430, 24, 70, 30);
    divisionBox.setBounds (508, 24, 78, 30);
    pingPongButton.setBounds (598, 24, 104, 30);
}
