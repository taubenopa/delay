#include "PluginEditor.h"

namespace
{
    const juce::Colour kAccent (0xffe23b2e);
    const juce::Colour kAmber  (0xffffb02e);
    const juce::Colour kGreen  (0xff3fd16b);
    const juce::Colour kText   (0xffd8d8de);

    const char* kIds[18] = { "input", "threshold", "ratio", "attack", "release", "knee",
                             "makeup", "schpf", "detector", "link", "deess", "deessfreq",
                             "body", "air", "drive", "mix", "ceiling", "output" };
    const char* kNames[18] = { "INPUT", "THRESHOLD", "RATIO", "ATTACK", "RELEASE", "KNEE",
                               "MAKEUP", "SC HPF", "PEAK / RMS", "ST. LINK", "DE-ESS", "DE-ESS FREQ",
                               "BODY", "AIR", "DRIVE", "MIX", "CEILING", "OUTPUT" };

    struct PresetValue { const char* id; float value; };
    struct Preset { const char* name; std::vector<PresetValue> values; };

    const std::vector<Preset>& presets()
    {
        static const std::vector<Preset> p =
        {
            { "Init (Default)", {} },
            { "Lead Rap - Forward",
              { { "threshold", -22.0f }, { "ratio", 5.0f }, { "attack", 4.0f }, { "release", 90.0f },
                { "knee", 6.0f }, { "mode", 1.0f }, { "detector", 0.35f }, { "schpf", 120.0f },
                { "deess", 0.35f }, { "deessfreq", 7000.0f }, { "drive", 0.15f }, { "air", 3.0f }, { "body", 1.0f } } },
            { "Dark Basement Grit",
              { { "threshold", -26.0f }, { "ratio", 8.0f }, { "attack", 2.0f }, { "release", 60.0f },
                { "knee", 3.0f }, { "mode", 1.0f }, { "drive", 0.55f }, { "mix", 0.75f },
                { "body", 2.0f }, { "air", 1.0f }, { "deess", 0.25f }, { "deessfreq", 6000.0f },
                { "ceiling", -1.5f } } },
            { "Smooth Melodic",
              { { "threshold", -18.0f }, { "ratio", 3.0f }, { "attack", 12.0f }, { "release", 220.0f },
                { "knee", 12.0f }, { "mode", 2.0f }, { "autorelease", 1.0f }, { "drive", 0.10f },
                { "air", 2.0f }, { "deess", 0.20f } } },
            { "Ad-libs Crush",
              { { "threshold", -32.0f }, { "ratio", 12.0f }, { "attack", 1.0f }, { "release", 45.0f },
                { "knee", 2.0f }, { "mode", 1.0f }, { "mix", 0.60f }, { "drive", 0.40f },
                { "air", 4.0f }, { "deess", 0.40f }, { "schpf", 200.0f } } },
            { "Gentle Leveler",
              { { "threshold", -16.0f }, { "ratio", 2.5f }, { "attack", 20.0f }, { "release", 300.0f },
                { "knee", 18.0f }, { "mode", 0.0f }, { "drive", 0.0f } } },
            { "Heavy Parallel (NY)",
              { { "threshold", -34.0f }, { "ratio", 10.0f }, { "attack", 0.5f }, { "release", 100.0f },
                { "knee", 4.0f }, { "mode", 1.0f }, { "mix", 0.45f }, { "drive", 0.30f }, { "air", 2.0f } } },
        };
        return p;
    }

    juce::String signedDb (double v)
    {
        return juce::String (v > 0.049 ? "+" : "") + juce::String (v, 1) + " dB";
    }

    void drawLedBar (juce::Graphics& g, juce::Rectangle<float> r, float norm, bool fromTop, bool isGr)
    {
        const int n = 24;
        const float segH = r.getHeight() / (float) n;

        for (int k = 0; k < n; ++k)
        {
            const float pos = ((float) k + 0.5f) / (float) n;       // 0 = unten (bzw. oben bei fromTop)
            const bool lit  = pos <= norm;
            const float y   = fromTop ? r.getY() + (float) k * segH
                                      : r.getBottom() - (float) (k + 1) * segH;
            const juce::Rectangle<float> seg (r.getX(), y + 1.0f, r.getWidth(), segH - 2.0f);

            if (! lit)
            {
                g.setColour (juce::Colours::white.withAlpha (0.06f));
            }
            else if (isGr)
            {
                g.setColour (pos > 0.6f ? kAccent : kAmber);
            }
            else
            {
                g.setColour (pos > 0.88f ? kAccent : (pos > 0.68f ? kAmber : kGreen));
            }
            g.fillRoundedRectangle (seg, 1.5f);
        }
    }

    void drawCaption (juce::Graphics& g, int x, int y, const juce::String& text)
    {
        g.setColour (kAccent);
        g.setFont (CompLookAndFeel::uiFont (10.5f));
        g.drawText (text, x, y, 400, 14, juce::Justification::centredLeft);
    }
}

//==============================================================================
CompLookAndFeel::CompLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId, juce::Colour (0xffc4c4cc));
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxHighlightColourId, kAccent.withAlpha (0.5f));

    setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff25252b));
    setColour (juce::TextEditor::textColourId, juce::Colours::white);
    setColour (juce::TextEditor::outlineColourId, juce::Colour (0xff3a3a42));
    setColour (juce::TextEditor::focusedOutlineColourId, kAccent);
    setColour (juce::TextEditor::highlightColourId, kAccent.withAlpha (0.4f));

    setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff222228));
    setColour (juce::ComboBox::textColourId, kText);
    setColour (juce::ComboBox::outlineColourId, juce::Colour (0xff3a3a42));
    setColour (juce::ComboBox::arrowColourId, kAccent);
    setColour (juce::ComboBox::buttonColourId, juce::Colour (0xff222228));

    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff1c1c21));
    setColour (juce::PopupMenu::textColourId, kText);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, kAccent);
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
}

juce::Font CompLookAndFeel::uiFont (float size, bool bold)
{
    return juce::Font (juce::FontOptions ("Segoe UI", size, bold ? juce::Font::bold : juce::Font::plain));
}

juce::Font CompLookAndFeel::getLabelFont (juce::Label&)        { return uiFont (12.0f); }
juce::Font CompLookAndFeel::getComboBoxFont (juce::ComboBox&)  { return uiFont (12.5f); }
juce::Font CompLookAndFeel::getPopupMenuFont()                 { return uiFont (13.0f); }

void CompLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                        float sliderPos, float startAngle, float endAngle,
                                        juce::Slider& slider)
{
    const float alpha  = slider.isEnabled() ? 1.0f : 0.35f;
    auto bounds        = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (4.0f);
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto c       = bounds.getCentre();
    const float angle  = startAngle + sliderPos * (endAngle - startAngle);

    // Punktering: leuchtet bis zum aktuellen Wert
    const int dots = 21;
    for (int i = 0; i < dots; ++i)
    {
        const float t = (float) i / (float) (dots - 1);
        const float a = startAngle + t * (endAngle - startAngle);
        const float px = c.x + (radius - 3.0f) * std::sin (a);
        const float py = c.y - (radius - 3.0f) * std::cos (a);
        const bool lit = t <= sliderPos + 0.001f;

        if (lit)
        {
            g.setColour (kAccent.withAlpha (0.25f * alpha));
            g.fillEllipse (px - 4.5f, py - 4.5f, 9.0f, 9.0f);
            g.setColour (kAccent.withAlpha (alpha));
        }
        else
        {
            g.setColour (juce::Colour (0xff3a3a42).withAlpha (alpha));
        }
        g.fillEllipse (px - 2.2f, py - 2.2f, 4.4f, 4.4f);
    }

    const float kr = radius * 0.70f;

    // Schatten
    g.setColour (juce::Colours::black.withAlpha (0.5f * alpha));
    g.fillEllipse (c.x - kr + 2.0f, c.y - kr + 4.0f, kr * 2.0f, kr * 2.0f);

    // Metallkoerper
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff595960).withAlpha (alpha), c.x - kr * 0.5f, c.y - kr * 0.7f,
                                             juce::Colour (0xff141416).withAlpha (alpha), c.x + kr * 0.6f, c.y + kr * 0.9f, true));
    g.fillEllipse (c.x - kr, c.y - kr, kr * 2.0f, kr * 2.0f);

    // Rillen
    g.setColour (juce::Colours::white.withAlpha (0.09f * alpha));
    for (int i = 0; i < 40; ++i)
    {
        const float a = (float) i * juce::MathConstants<float>::twoPi / 40.0f;
        g.drawLine (c.x + kr * 0.88f * std::sin (a), c.y - kr * 0.88f * std::cos (a),
                    c.x + kr * std::sin (a),         c.y - kr * std::cos (a), 1.0f);
    }

    // Kappe
    const float cr = kr * 0.70f;
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2e2e33).withAlpha (alpha), c.x - cr * 0.4f, c.y - cr * 0.5f,
                                             juce::Colour (0xff111113).withAlpha (alpha), c.x + cr * 0.5f, c.y + cr * 0.6f, true));
    g.fillEllipse (c.x - cr, c.y - cr, cr * 2.0f, cr * 2.0f);
    g.setColour (juce::Colours::white.withAlpha (0.12f * alpha));
    g.drawEllipse (c.x - cr, c.y - cr, cr * 2.0f, cr * 2.0f, 1.0f);

    // Zeiger
    juce::Path pointer;
    pointer.addRoundedRectangle (-1.9f, -kr + 2.0f, 3.8f, kr * 0.55f, 1.5f);
    pointer.applyTransform (juce::AffineTransform::rotation (angle).translated (c.x, c.y));
    g.setColour (juce::Colour (0xfff2f2f4).withAlpha (alpha));
    g.fillPath (pointer);
}

void CompLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool highlighted, bool)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.0f);

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2c2c33), r.getX(), r.getY(),
                                             juce::Colour (0xff1a1a1f), r.getX(), r.getBottom(), false));
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (b.getToggleState() ? kAccent.withAlpha (0.7f) : juce::Colour (0xff3a3a42));
    g.drawRoundedRectangle (r, 6.0f, 1.3f);

    if (highlighted)
    {
        g.setColour (juce::Colours::white.withAlpha (0.05f));
        g.fillRoundedRectangle (r, 6.0f);
    }

    const bool on = b.getToggleState();
    const juce::Rectangle<float> led (r.getX() + 10.0f, r.getCentreY() - 5.0f, 10.0f, 10.0f);

    if (on)
    {
        g.setGradientFill (juce::ColourGradient (kAccent.withAlpha (0.45f), led.getCentreX(), led.getCentreY(),
                                                 juce::Colours::transparentBlack, led.getCentreX() + 14.0f, led.getCentreY(), true));
        g.fillEllipse (led.expanded (10.0f));
        g.setColour (kAccent);
    }
    else
    {
        g.setColour (juce::Colour (0xff4a2420));
    }
    g.fillEllipse (led);
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.drawEllipse (led, 1.0f);

    g.setColour (kText.withAlpha (b.isEnabled() ? 1.0f : 0.5f));
    g.setFont (uiFont (12.0f));
    g.drawText (b.getButtonText(), (int) (r.getX() + 28.0f), (int) r.getY(),
                (int) (r.getWidth() - 32.0f), (int) r.getHeight(), juce::Justification::centredLeft);
}

//==============================================================================
CompEditor::CompEditor (CompProcessor& p)
    : juce::AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&lnf);

    for (size_t i = 0; i < knobs.size(); ++i)
    {
        auto& s = knobs[i].slider;
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 100, 18);
        s.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f,
                               juce::MathConstants<float>::pi * 2.8f, true);
        knobs[i].attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            proc.apvts, kIds[i], s);
        addAndMakeVisible (s);
    }

    // Anzeigeformate (nach dem Attachment setzen, sonst werden sie ueberschrieben)
    auto setText = [this] (int idx, std::function<juce::String (double)> toText,
                           std::function<double (const juce::String&)> fromText)
    {
        knobs[(size_t) idx].slider.textFromValueFunction = std::move (toText);
        knobs[(size_t) idx].slider.valueFromTextFunction = std::move (fromText);
    };

    auto num      = [] (const juce::String& t) { return t.getDoubleValue(); };
    auto pct      = [] (double v) { return juce::String (juce::roundToInt (v * 100.0)) + " %"; };
    auto pctParse = [] (const juce::String& t) { return t.getDoubleValue() / 100.0; };
    auto db1      = [] (double v) { return juce::String (v, 1) + " dB"; };
    auto sdb      = [] (double v) { return signedDb (v); };
    auto hz       = [] (double v) { return juce::String (juce::roundToInt (v)) + " Hz"; };

    setText (0,  sdb, num);
    setText (1,  db1, num);
    setText (2,  [] (double v) { return v >= 19.9 ? juce::String ("INF") : juce::String (v, 1) + " : 1"; }, num);
    setText (3,  [] (double v) { return v < 1.0 ? juce::String (v, 2) + " ms" : juce::String (v, 1) + " ms"; }, num);
    setText (4,  [] (double v) { return juce::String (juce::roundToInt (v)) + " ms"; }, num);
    setText (5,  db1, num);
    setText (6,  sdb, num);
    setText (7,  hz, num);
    setText (8,  [] (double v) { return v < 0.05 ? juce::String ("PEAK")
                                       : (v > 0.95 ? juce::String ("RMS")
                                                   : juce::String (juce::roundToInt (v * 100.0)) + " % RMS"); },
                 pctParse);
    setText (9,  pct, pctParse);
    setText (10, [] (double v) { return v < 0.005 ? juce::String ("OFF")
                                                  : juce::String (juce::roundToInt (v * 100.0)) + " %"; },
                 pctParse);
    setText (11, [] (double v) { return juce::String (v / 1000.0, 1) + " kHz"; },
                 [] (const juce::String& t) { return t.containsIgnoreCase ("k") ? t.getDoubleValue() * 1000.0
                                                                                : t.getDoubleValue(); });
    setText (12, sdb, num);
    setText (13, sdb, num);
    setText (14, pct, pctParse);
    setText (15, pct, pctParse);
    setText (16, db1, num);
    setText (17, sdb, num);

    for (auto& k : knobs)
        k.slider.updateText();

    // Buttons
    bypassButton.setButtonText ("BYPASS");
    autoMakeupButton.setButtonText ("AUTO MAKEUP");
    autoReleaseButton.setButtonText ("AUTO RELEASE");
    scListenButton.setButtonText ("SC LISTEN");
    limiterButton.setButtonText ("LIMITER");
    for (auto* b : { &bypassButton, &autoMakeupButton, &autoReleaseButton, &scListenButton, &limiterButton })
        addAndMakeVisible (*b);

    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;
    bypassAtt  = std::make_unique<BA> (proc.apvts, "bypass", bypassButton);
    makeupAtt  = std::make_unique<BA> (proc.apvts, "automakeup", autoMakeupButton);
    releaseAtt = std::make_unique<BA> (proc.apvts, "autorelease", autoReleaseButton);
    scAtt      = std::make_unique<BA> (proc.apvts, "sclisten", scListenButton);
    limiterAtt = std::make_unique<BA> (proc.apvts, "limiter", limiterButton);

    // Mode (VCA / FET / OPTO)
    if (auto* mp = dynamic_cast<juce::AudioParameterChoice*> (proc.apvts.getParameter ("mode")))
        modeBox.addItemList (mp->choices, 1);
    modeAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        proc.apvts, "mode", modeBox);
    addAndMakeVisible (modeBox);

    // Presets
    int id = 1;
    for (const auto& pr : presets())
        presetBox.addItem (pr.name, id++);
    presetBox.setTextWhenNothingSelected ("PRESETS");
    presetBox.onChange = [this] { applyPreset (presetBox.getSelectedItemIndex()); };
    addAndMakeVisible (presetBox);

    setSize (1000, 606);
    startTimerHz (30);
}

CompEditor::~CompEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void CompEditor::applyPreset (int index)
{
    const auto& list = presets();
    if (index < 0 || index >= (int) list.size())
        return;

    // erst alles auf Standard, dann die Werte des Presets setzen
    for (auto* prm : proc.getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (prm))
            rp->setValueNotifyingHost (rp->getDefaultValue());

    for (const auto& v : list[(size_t) index].values)
        if (auto* rp = proc.apvts.getParameter (v.id))
            rp->setValueNotifyingHost (rp->convertTo0to1 (v.value));
}

juce::Rectangle<int> CompEditor::panelRect (int row)
{
    return { 24, 92 + row * 170, 712, 162 };
}

juce::Rectangle<int> CompEditor::cell (int row, int col)
{
    return { 32 + col * 116, 92 + row * 170 + 18, 116, 142 };
}

void CompEditor::timerCallback()
{
    auto toDb = [] (float lin) { return 20.0f * std::log10 (juce::jmax (lin, 1.0e-5f)); };
    auto fall = [] (float& disp, float v)
    {
        if (v > disp) disp = v;
        else          disp = juce::jmax (v, disp - 1.2f);
    };

    fall (dispIn,  toDb (proc.inPeak.exchange (0.0f)));
    fall (dispOut, toDb (proc.outPeak.exchange (0.0f)));

    const float grNew = proc.grPeak.exchange (0.0f);
    if (grNew > dispGr) dispGr = grNew;
    else                dispGr = juce::jmax (grNew, dispGr - 0.8f);

    // Regler ausgrauen, die gerade keine Wirkung haben
    const bool autoMk  = proc.apvts.getRawParameterValue ("automakeup")->load() > 0.5f;
    const bool autoRel = proc.apvts.getRawParameterValue ("autorelease")->load() > 0.5f;
    const bool limiter = proc.apvts.getRawParameterValue ("limiter")->load() > 0.5f;

    if (knobs[6].slider.isEnabled()  == autoMk)  knobs[6].slider.setEnabled (! autoMk);
    if (knobs[4].slider.isEnabled()  == autoRel) knobs[4].slider.setEnabled (! autoRel);
    if (knobs[16].slider.isEnabled() != limiter) knobs[16].slider.setEnabled (limiter);

    repaint (760, 100, 216, 280);
}

void CompEditor::paint (juce::Graphics& g)
{
    const int w = getWidth();
    const int h = getHeight();

    // Hintergrund
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff131316), 0.0f, 0.0f,
                                             juce::Colour (0xff09090b), 0.0f, (float) h, false));
    g.fillAll();
    g.setColour (juce::Colours::white.withAlpha (0.018f));
    for (int x = -h; x < w; x += 7)
        g.drawLine ((float) x, 0.0f, (float) (x + h), (float) h, 1.0f);

    // Titel
    g.setColour (kAccent);
    g.fillRect (24, 18, 6, 42);
    g.setColour (juce::Colours::white);
    g.setFont (juce::Font (juce::FontOptions ("Impact", 40.0f, juce::Font::plain)));
    g.drawText ("UNDERGROUND COMP", 40, 12, 470, 48, juce::Justification::centredLeft);
    g.setColour (juce::Colour (0xff8a8a94));
    g.setFont (CompLookAndFeel::uiFont (10.5f, false));
    g.drawText ("VOCAL COMPRESSOR  /  BUILT FOR THE UNDERGROUND", 42, 60, 470, 16, juce::Justification::centredLeft);

    // Paneele
    const char* rowTitles[3] = { "COMPRESSOR", "SIDECHAIN  /  DE-ESSER", "TONE  /  SATURATION  /  OUTPUT" };
    for (int row = 0; row < 3; ++row)
    {
        const auto pr = panelRect (row).toFloat();
        g.setColour (juce::Colour (0xff18181d));
        g.fillRoundedRectangle (pr, 10.0f);
        g.setColour (juce::Colour (0xff2a2a31));
        g.drawRoundedRectangle (pr, 10.0f, 1.0f);
        drawCaption (g, (int) pr.getX() + 14, (int) pr.getY() + 6, rowTitles[row]);
    }

    // Reglernamen
    g.setFont (CompLookAndFeel::uiFont (11.5f));
    g.setColour (kText);
    for (int i = 0; i < 18; ++i)
    {
        const auto c = cell (i / 6, i % 6);
        g.drawText (kNames[i], c.getX(), c.getY(), c.getWidth(), 16, juce::Justification::centred);
    }

    // Rechtes Paneel
    const juce::Rectangle<float> rp (748.0f, 92.0f, 228.0f, 502.0f);
    g.setColour (juce::Colour (0xff18181d));
    g.fillRoundedRectangle (rp, 10.0f);
    g.setColour (juce::Colour (0xff2a2a31));
    g.drawRoundedRectangle (rp, 10.0f, 1.0f);
    drawCaption (g, 762, 98, "METERS");
    drawCaption (g, 762, 366, "CHARACTER");

    // Meter: IN | GR | OUT
    const float barTop = 122.0f, barH = 200.0f, barW = 34.0f;
    const float xs[3] = { 781.0f, 845.0f, 909.0f };

    auto levelNorm = [] (float db) { return juce::jlimit (0.0f, 1.0f, (db + 48.0f) / 48.0f); };
    drawLedBar (g, { xs[0], barTop, barW, barH }, levelNorm (dispIn), false, false);
    drawLedBar (g, { xs[1], barTop, barW, barH }, juce::jlimit (0.0f, 1.0f, dispGr / 24.0f), true, true);
    drawLedBar (g, { xs[2], barTop, barW, barH }, levelNorm (dispOut), false, false);

    g.setFont (CompLookAndFeel::uiFont (10.5f));
    g.setColour (juce::Colour (0xff9a9aa4));
    const char* meterNames[3] = { "IN", "GR", "OUT" };
    for (int i = 0; i < 3; ++i)
        g.drawText (meterNames[i], (int) xs[i] - 10, (int) (barTop + barH) + 4, (int) barW + 20, 14,
                    juce::Justification::centred);

    g.setColour (kText);
    g.setFont (CompLookAndFeel::uiFont (12.0f));
    g.drawText ("GR  -" + juce::String (dispGr, 1) + " dB", 762, 344, 200, 16, juce::Justification::centred);
}

void CompEditor::resized()
{
    for (size_t i = 0; i < knobs.size(); ++i)
    {
        const auto c = cell ((int) i / 6, (int) i % 6);
        knobs[i].slider.setBounds (c.withTrimmedTop (16));
    }

    presetBox.setBounds (520, 26, 220, 34);
    bypassButton.setBounds (766, 26, 192, 34);

    modeBox.setBounds (766, 388, 192, 30);
    autoMakeupButton.setBounds (766, 432, 192, 30);
    autoReleaseButton.setBounds (766, 468, 192, 30);
    scListenButton.setBounds (766, 504, 192, 30);
    limiterButton.setBounds (766, 540, 192, 30);
}
