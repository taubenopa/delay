#pragma once
#include <array>
#include "PluginProcessor.h"

class KnobLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider&) override;
};

class SimpleDelayEditor : public juce::AudioProcessorEditor
{
public:
    explicit SimpleDelayEditor (SimpleDelayProcessor&);
    ~SimpleDelayEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    SimpleDelayProcessor& proc;
    KnobLookAndFeel lnf;
    std::array<Knob, 3> knobs;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleDelayEditor)
};
