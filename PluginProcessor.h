#pragma once
#include <atomic>
#include <juce_audio_processors/juce_audio_processors.h>

class SimpleDelayProcessor : public juce::AudioProcessor
{
public:
    SimpleDelayProcessor();

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Analog Echo"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 8.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    // aktuelle Delayzeit, wird vom Editor fuer die blinkende LED gelesen
    std::atomic<float> currentDelayMs { 350.0f };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    juce::AudioBuffer<float> delayBuffer;
    int writePos = 0;
    double sampleRateHz = 44100.0;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> sTime, sFeedback, sMix,
                                                                   sOutput, sDrive, sWow;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> sTone, sLowCut;

    float hp[2] {}, lp1[2] {}, lp2[2] {}, noiseLp[2] {};
    double wowPhase = 0.0, flutterPhase = 0.0;
    juce::Random rng;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleDelayProcessor)
};
