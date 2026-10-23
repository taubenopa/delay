#pragma once
#include <array>
#include "PluginProcessor.h"

class CompLookAndFeel : public juce::LookAndFeel_V4
{
public:
    CompLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&,
                           bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    juce::Font getLabelFont (juce::Label&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getPopupMenuFont() override;

    static juce::Font uiFont (float size, bool bold = true);
};

class CompEditor : public juce::AudioProcessorEditor,
                   private juce::Timer
{
public:
    explicit CompEditor (CompProcessor&);
    ~CompEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void applyPreset (int index);

    static juce::Rectangle<int> panelRect (int row);
    static juce::Rectangle<int> cell (int row, int col);

    struct Knob
    {
        juce::Slider slider;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    CompProcessor& proc;
    CompLookAndFeel lnf;

    std::array<Knob, 18> knobs;
    juce::ToggleButton bypassButton, autoMakeupButton, autoReleaseButton, scListenButton, limiterButton;
    juce::ComboBox modeBox, presetBox;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAtt, makeupAtt, releaseAtt,
                                                                           scAtt, limiterAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modeAtt;

    float dispIn = -80.0f, dispOut = -80.0f, dispGr = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CompEditor)
};
