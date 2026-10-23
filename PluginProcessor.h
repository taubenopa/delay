#pragma once
#include <atomic>
#include <cmath>
#include <juce_audio_processors/juce_audio_processors.h>

class CompProcessor : public juce::AudioProcessor
{
public:
    CompProcessor();

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Underground Comp"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    // Pegel fuer die Anzeige (linear bzw. dB). Der Editor liest und setzt sie zurueck.
    std::atomic<float> inPeak { 0.0f }, outPeak { 0.0f }, grPeak { 0.0f };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    struct Biquad
    {
        float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f;
        float z1[2] {}, z2[2] {};

        void reset() { z1[0] = z1[1] = z2[0] = z2[1] = 0.0f; }

        float process (int ch, float x)
        {
            const float y = b0 * x + z1[ch];
            z1[ch] = b1 * x - a1 * y + z2[ch];
            z2[ch] = b2 * x - a2 * y;
            return y;
        }

        void set (double nb0, double nb1, double nb2, double na0, double na1, double na2)
        {
            b0 = (float) (nb0 / na0);
            b1 = (float) (nb1 / na0);
            b2 = (float) (nb2 / na0);
            a1 = (float) (na1 / na0);
            a2 = (float) (na2 / na0);
        }

        void setLowShelf (double sr, double f, double gainDb)
        {
            const double A  = std::pow (10.0, gainDb / 40.0);
            const double w0 = 2.0 * 3.14159265358979323846 * f / sr;
            const double cs = std::cos (w0), sn = std::sin (w0);
            const double alpha = sn / (2.0 * 0.7071);
            const double t = 2.0 * std::sqrt (A) * alpha;
            set (A * ((A + 1.0) - (A - 1.0) * cs + t),
                 2.0 * A * ((A - 1.0) - (A + 1.0) * cs),
                 A * ((A + 1.0) - (A - 1.0) * cs - t),
                 (A + 1.0) + (A - 1.0) * cs + t,
                 -2.0 * ((A - 1.0) + (A + 1.0) * cs),
                 (A + 1.0) + (A - 1.0) * cs - t);
        }

        void setHighShelf (double sr, double f, double gainDb)
        {
            const double A  = std::pow (10.0, gainDb / 40.0);
            const double w0 = 2.0 * 3.14159265358979323846 * f / sr;
            const double cs = std::cos (w0), sn = std::sin (w0);
            const double alpha = sn / (2.0 * 0.7071);
            const double t = 2.0 * std::sqrt (A) * alpha;
            set (A * ((A + 1.0) + (A - 1.0) * cs + t),
                 -2.0 * A * ((A - 1.0) + (A + 1.0) * cs),
                 A * ((A + 1.0) + (A - 1.0) * cs - t),
                 (A + 1.0) - (A - 1.0) * cs + t,
                 2.0 * ((A - 1.0) - (A + 1.0) * cs),
                 (A + 1.0) - (A - 1.0) * cs - t);
        }
    };

    double sampleRateHz = 44100.0;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> sInput, sOutput, sMix, sDrive,
                                                                   sDeess, sMakeup, sBypass;

    // Zustaende pro Kanal
    float lpA[2] {}, lpB[2] {}, envHi[2] {}, envFull[2] {}, deState[2] {};   // De-Esser
    float hpA[2] {}, hpB[2] {}, msq[2] {};                                    // Sidechain
    float grState[2] {};                                                      // Gain Reduction (dB, positiv)
    float dcX[2] {}, dcY[2] {};                                               // DC-Blocker
    Biquad lowShelf, highShelf;
    float limGain = 1.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CompProcessor)
};
