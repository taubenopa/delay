#pragma once
#include <array>
#include "PluginProcessor.h"

class VintageLookAndFeel : public juce::LookAndFeel_V4
{
public:
    VintageLookAndFeel();

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

class SimpleDelayEditor : public juce::AudioProcessorEditor,
                          private juce::Timer
{
public:
    explicit SimpleDelayEditor (SimpleDelayProcessor&);
    ~SimpleDelayEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    static juce::Rectangle<int> cell (int row, int col);
    juce::Rectangle<int> ledBounds() const { return { getWidth() - 76, 31, 18, 18 }; }

    struct Knob
    {
        juce::Slider slider;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    SimpleDelayProcessor& proc;
    VintageLookAndFeel lnf;

    std::array<Knob, 10> knobs;
    juce::ToggleButton syncButton, pingPongButton;
    juce::ComboBox divisionBox;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> syncAttachment, pingAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> divisionAttachment;

    bool ledOn = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleDelayEditor)
};
