#include "PluginEditor.h"

namespace
{
    const juce::Colour accent (0xffb066ff);
}

//==============================================================================
void KnobLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                        float sliderPos, float startAngle, float endAngle,
                                        juce::Slider&)
{
    auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (10.0f);
    const float radius    = juce::jmin (bounds.getWidth(), bounds.getHeight()) / 2.0f;
    const auto centre     = bounds.getCentre();
    const float angle     = startAngle + sliderPos * (endAngle - startAngle);
    const float arcRadius = radius - 4.0f;

    const juce::PathStrokeType stroke (5.0f, juce::PathStrokeType::curved,
                                       juce::PathStrokeType::rounded);

    // Hintergrund-Bogen
    juce::Path track;
    track.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                         startAngle, endAngle, true);
    g.setColour (juce::Colour (0xff2a2738));
    g.strokePath (track, stroke);

    // Wert-Bogen
    juce::Path value;
    value.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                         startAngle, angle, true);
    g.setColour (accent);
    g.strokePath (value, stroke);

    // Knopf
    const float knobR = arcRadius - 12.0f;
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3a3650), centre.x, centre.y - knobR,
                                             juce::Colour (0xff1a1826), centre.x, centre.y + knobR,
                                             false));
    g.fillEllipse (centre.x - knobR, centre.y - knobR, knobR * 2.0f, knobR * 2.0f);
    g.setColour (juce::Colour (0xff4a4666));
    g.drawEllipse (centre.x - knobR, centre.y - knobR, knobR * 2.0f, knobR * 2.0f, 1.5f);

    // Zeiger
    juce::Path pointer;
    pointer.addRoundedRectangle (-1.75f, -knobR + 5.0f, 3.5f, knobR * 0.5f, 1.5f);
    pointer.applyTransform (juce::AffineTransform::rotation (angle).translated (centre.x, centre.y));
    g.setColour (accent);
    g.fillPath (pointer);
}

//==============================================================================
SimpleDelayEditor::SimpleDelayEditor (SimpleDelayProcessor& p)
    : juce::AudioProcessorEditor (&p), proc (p)
{
    static const char* ids[]   = { "time", "feedback", "mix" };
    static const char* names[] = { "TIME", "FEEDBACK", "MIX" };

    for (size_t i = 0; i < knobs.size(); ++i)
    {
        auto& k = knobs[i];

        k.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        k.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 100, 24);
        k.slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f,
                                      juce::MathConstants<float>::pi * 2.8f, true);
        k.slider.setLookAndFeel (&lnf);
        k.slider.setColour (juce::Slider::textBoxTextColourId, juce::Colours::white.withAlpha (0.9f));
        k.slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        k.slider.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);

        k.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            proc.apvts, ids[i], k.slider);

        k.label.setText (names[i], juce::dontSendNotification);
        k.label.setJustificationType (juce::Justification::centred);
        k.label.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.6f));
        k.label.setFont (k.label.getFont().withHeight (13.0f).boldened());

        addAndMakeVisible (k.slider);
        addAndMakeVisible (k.label);
    }

    // Anzeige der Werte (nach dem Attachment setzen, sonst wird es ueberschrieben)
    knobs[0].slider.textFromValueFunction = [] (double v) { return juce::String (v, 0) + " ms"; };
    knobs[0].slider.valueFromTextFunction = [] (const juce::String& t) { return t.getDoubleValue(); };

    auto percentText = [] (double v) { return juce::String (juce::roundToInt (v * 100.0)) + " %"; };
    auto percentParse = [] (const juce::String& t) { return t.getDoubleValue() / 100.0; };
    for (size_t i = 1; i < knobs.size(); ++i)
    {
        knobs[i].slider.textFromValueFunction = percentText;
        knobs[i].slider.valueFromTextFunction = percentParse;
    }

    for (auto& k : knobs)
        k.slider.updateText();

    setSize (480, 320);
}

SimpleDelayEditor::~SimpleDelayEditor()
{
    for (auto& k : knobs)
        k.slider.setLookAndFeel (nullptr);
}

void SimpleDelayEditor::paint (juce::Graphics& g)
{
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff17141f), 0.0f, 0.0f,
                                             juce::Colour (0xff0a090f), 0.0f, (float) getHeight(),
                                             false));
    g.fillAll();

    g.setColour (accent);
    g.setFont (g.getCurrentFont().withHeight (24.0f).boldened());
    g.drawText ("SIMPLE DELAY", 0, 14, getWidth(), 30, juce::Justification::centred);

    g.setColour (juce::Colours::white.withAlpha (0.35f));
    g.setFont (12.0f);
    g.drawText ("ECHO", 0, 44, getWidth(), 16, juce::Justification::centred);

    auto panel = getLocalBounds().toFloat().reduced (20.0f);
    panel.removeFromTop (56.0f);
    g.setColour (juce::Colour (0xff1b1827));
    g.fillRoundedRectangle (panel, 16.0f);
    g.setColour (juce::Colour (0xff2c2840));
    g.drawRoundedRectangle (panel, 16.0f, 1.0f);
}

void SimpleDelayEditor::resized()
{
    auto area = getLocalBounds().reduced (20);
    area.removeFromTop (56);
    area = area.reduced (10);

    const int w = area.getWidth() / 3;
    for (auto& k : knobs)
    {
        auto col = area.removeFromLeft (w);
        k.label.setBounds (col.removeFromTop (22));
        k.slider.setBounds (col);
    }
}
