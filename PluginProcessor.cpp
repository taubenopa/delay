#include "PluginProcessor.h"
#include "PluginEditor.h"

SimpleDelayProcessor::SimpleDelayProcessor()
    : AudioProcessor (BusesProperties()
          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout SimpleDelayProcessor::createLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;
    p.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { "time", 1 }, "Time (ms)",
        NormalisableRange<float> (1.0f, 2000.0f, 0.1f, 0.4f), 350.0f));
    p.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { "feedback", 1 }, "Feedback",
        NormalisableRange<float> (0.0f, 0.95f, 0.001f), 0.4f));
    p.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { "mix", 1 }, "Mix",
        NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.3f));
    return { p.begin(), p.end() };
}

void SimpleDelayProcessor::prepareToPlay (double sampleRate, int)
{
    currentSampleRate = sampleRate;
    const int size = (int) (sampleRate * 2.5) + 1; // genug fuer 2 s + Reserve
    delayBuffer.setSize (2, size);
    delayBuffer.clear();
    writePos = 0;
}

bool SimpleDelayProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return out == layouts.getMainInputChannelSet();
}

void SimpleDelayProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const float timeMs   = apvts.getRawParameterValue ("time")->load();
    const float feedback = apvts.getRawParameterValue ("feedback")->load();
    const float mix      = apvts.getRawParameterValue ("mix")->load();

    const int bufSize     = delayBuffer.getNumSamples();
    const float delaySamp = juce::jlimit (1.0f, (float) (bufSize - 2),
                                          timeMs * 0.001f * (float) currentSampleRate);
    const int numCh = juce::jmin (buffer.getNumChannels(), 2);

    for (int ch = 0; ch < numCh; ++ch)
    {
        float* data  = buffer.getWritePointer (ch);
        float* delay = delayBuffer.getWritePointer (ch);
        int wp = writePos;

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            float rp = (float) wp - delaySamp;
            if (rp < 0.0f) rp += (float) bufSize;

            const int i0 = (int) rp;
            const int i1 = (i0 + 1) % bufSize;
            const float frac = rp - (float) i0;
            const float delayed = delay[i0] + frac * (delay[i1] - delay[i0]);

            const float in = data[i];
            delay[wp] = in + delayed * feedback;
            data[i]   = in * (1.0f - mix) + delayed * mix;

            if (++wp >= bufSize) wp = 0;
        }
    }
    writePos = (writePos + buffer.getNumSamples()) % bufSize;
}

juce::AudioProcessorEditor* SimpleDelayProcessor::createEditor()
{
    return new SimpleDelayEditor (*this);
}

void SimpleDelayProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void SimpleDelayProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SimpleDelayProcessor();
}
