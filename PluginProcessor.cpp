#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    constexpr float kPi    = juce::MathConstants<float>::pi;
    constexpr float kTwoPi = juce::MathConstants<float>::twoPi;

    // Notenwerte in Viertelnoten: 1/2, 1/4, 1/4., 1/8, 1/8., 1/8T, 1/16, 1/16T
    const double kBeats[] = { 2.0, 1.0, 1.5, 0.5, 0.75, 1.0 / 3.0, 0.25, 1.0 / 6.0 };
}

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
        ParameterID { "time", 1 }, "Time",
        NormalisableRange<float> (20.0f, 1500.0f, 0.1f, 0.4f), 350.0f));
    p.push_back (std::make_unique<AudioParameterBool> (
        ParameterID { "sync", 1 }, "Sync", false));
    p.push_back (std::make_unique<AudioParameterChoice> (
        ParameterID { "division", 1 }, "Division",
        StringArray { "1/2", "1/4", "1/4.", "1/8", "1/8.", "1/8T", "1/16", "1/16T" }, 3));
    p.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { "feedback", 1 }, "Feedback",
        NormalisableRange<float> (0.0f, 1.1f, 0.001f), 0.4f));
    p.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { "mix", 1 }, "Mix",
        NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.35f));
    p.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { "tone", 1 }, "Tone",
        NormalisableRange<float> (500.0f, 12000.0f, 1.0f, 0.4f), 4500.0f));
    p.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { "lowcut", 1 }, "Low Cut",
        NormalisableRange<float> (20.0f, 600.0f, 1.0f, 0.5f), 40.0f));
    p.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { "drive", 1 }, "Drive",
        NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.2f));
    p.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { "wow", 1 }, "Wow",
        NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.25f));
    p.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { "rate", 1 }, "Rate",
        NormalisableRange<float> (0.1f, 6.0f, 0.01f, 0.5f), 0.7f));
    p.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { "noise", 1 }, "Noise",
        NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.1f));
    p.push_back (std::make_unique<AudioParameterBool> (
        ParameterID { "pingpong", 1 }, "Ping-Pong", false));
    p.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { "output", 1 }, "Output",
        NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f));

    return { p.begin(), p.end() };
}

void SimpleDelayProcessor::prepareToPlay (double newSampleRate, int)
{
    sampleRateHz = newSampleRate;

    const int size = (int) (sampleRateHz * 3.2) + 8;
    delayBuffer.setSize (2, size);
    delayBuffer.clear();
    writePos = 0;

    auto get = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };

    sTime.reset (sampleRateHz, 0.15);     // langsames Gleiten = Pitch-Effekt wie bei Analog-Delays
    sTime.setCurrentAndTargetValue (get ("time"));
    sFeedback.reset (sampleRateHz, 0.03);
    sFeedback.setCurrentAndTargetValue (get ("feedback"));
    sMix.reset (sampleRateHz, 0.03);
    sMix.setCurrentAndTargetValue (get ("mix"));
    sOutput.reset (sampleRateHz, 0.03);
    sOutput.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (get ("output")));
    sDrive.reset (sampleRateHz, 0.03);
    sDrive.setCurrentAndTargetValue (get ("drive"));
    sWow.reset (sampleRateHz, 0.05);
    sWow.setCurrentAndTargetValue (get ("wow"));
    sTone.reset (sampleRateHz, 0.03);
    sTone.setCurrentAndTargetValue (get ("tone"));
    sLowCut.reset (sampleRateHz, 0.03);
    sLowCut.setCurrentAndTargetValue (get ("lowcut"));

    for (int c = 0; c < 2; ++c)
        hp[c] = lp1[c] = lp2[c] = noiseLp[c] = 0.0f;
    wowPhase = flutterPhase = 0.0;
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

    const int numSamples = buffer.getNumSamples();
    const int numCh      = juce::jmin (buffer.getNumChannels(), 2);

    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, numSamples);

    const int bufSize = delayBuffer.getNumSamples();
    if (bufSize < 16 || numCh < 1)
        return;

    auto get = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };

    // --- Delayzeit (frei oder zum Host-Tempo) ---
    double timeMs = get ("time");
    if (get ("sync") > 0.5f)
    {
        double bpm = 120.0;
        if (auto* ph = getPlayHead())
            if (auto pos = ph->getPosition())
                if (auto b = pos->getBpm())
                    bpm = *b;
        if (bpm < 20.0) bpm = 20.0;
        const int idx = juce::jlimit (0, 7, (int) get ("division"));
        timeMs = 60000.0 / bpm * kBeats[idx];
    }
    timeMs = juce::jlimit (5.0, 2800.0, timeMs);
    currentDelayMs.store ((float) timeMs);

    sTime.setTargetValue ((float) timeMs);
    sFeedback.setTargetValue (get ("feedback"));
    sMix.setTargetValue (get ("mix"));
    sOutput.setTargetValue (juce::Decibels::decibelsToGain (get ("output")));
    sDrive.setTargetValue (get ("drive"));
    sWow.setTargetValue (get ("wow"));
    sTone.setTargetValue (get ("tone"));
    sLowCut.setTargetValue (get ("lowcut"));

    const float rate     = get ("rate");
    const float noiseAmt = get ("noise");
    const bool pingPong  = get ("pingpong") > 0.5f && numCh == 2;
    const float sr       = (float) sampleRateHz;

    float* io[2] = { buffer.getWritePointer (0), numCh > 1 ? buffer.getWritePointer (1) : nullptr };
    float* dl[2] = { delayBuffer.getWritePointer (0), delayBuffer.getWritePointer (1) };

    for (int i = 0; i < numSamples; ++i)
    {
        const float tMs    = sTime.getNextValue();
        const float fb     = sFeedback.getNextValue();
        const float mix    = sMix.getNextValue();
        const float outG   = sOutput.getNextValue();
        const float drive  = sDrive.getNextValue();
        const float tone   = sTone.getNextValue();
        const float lowcut = sLowCut.getNextValue();
        const float wow    = sWow.getNextValue();

        const float g    = 1.0f + drive * 3.0f;
        const float aLP  = std::exp (-kTwoPi * tone   / sr);
        const float aHP  = std::exp (-kTwoPi * lowcut / sr);
        const float dryG = std::cos (mix * 0.5f * kPi);
        const float wetG = std::sin (mix * 0.5f * kPi);

        wowPhase += (double) rate / (double) sr;
        if (wowPhase >= 1.0) wowPhase -= 1.0;
        flutterPhase += 6.3 / (double) sr;
        if (flutterPhase >= 1.0) flutterPhase -= 1.0;

        const float baseDelay = tMs * 0.001f * sr;

        float in[2]  = { 0.0f, 0.0f };
        float wet[2] = { 0.0f, 0.0f };

        // 1) Lesen + Filtern (Low Cut, dann 2 Tiefpass-Stufen = dunkle Wiederholungen)
        for (int ch = 0; ch < numCh; ++ch)
        {
            in[ch] = io[ch][i];

            const double off = (ch == 0) ? 0.0 : 0.25;
            const float mod = wow * (std::sin (kTwoPi * (float) std::fmod (wowPhase + off, 1.0)) * 0.0025f
                                   + std::sin (kTwoPi * (float) std::fmod (flutterPhase + off * 1.7, 1.0)) * 0.0004f) * sr;

            const double dly = juce::jlimit (2.0, (double) (bufSize - 4), (double) (baseDelay + mod));
            double rp = (double) writePos - dly;
            if (rp < 0.0) rp += (double) bufSize;

            int i0 = (int) rp;
            const float frac = (float) (rp - (double) i0);
            if (i0 >= bufSize) i0 -= bufSize;
            int i1 = i0 + 1;
            if (i1 >= bufSize) i1 = 0;

            float x = dl[ch][i0] + frac * (dl[ch][i1] - dl[ch][i0]);

            hp[ch] = aHP * hp[ch] + (1.0f - aHP) * x;
            x -= hp[ch];

            lp1[ch] = (1.0f - aLP) * x + aLP * lp1[ch];
            lp2[ch] = (1.0f - aLP) * lp1[ch] + aLP * lp2[ch];
            wet[ch] = lp2[ch];
        }

        // 2) Schreiben: Eingang + Feedback, Saettigung, Rauschen
        for (int ch = 0; ch < numCh; ++ch)
        {
            float src, fbIn;
            if (pingPong)
            {
                src  = (ch == 0) ? 0.5f * (in[0] + in[1]) : 0.0f;
                fbIn = fb * wet[1 - ch];
            }
            else
            {
                src  = in[ch];
                fbIn = fb * wet[ch];
            }

            noiseLp[ch] = 0.6f * noiseLp[ch] + 0.4f * (rng.nextFloat() * 2.0f - 1.0f);

            const float w = std::tanh (g * (src + fbIn)) / g + noiseLp[ch] * noiseAmt * 0.01f;
            dl[ch][writePos] = w;

            io[ch][i] = (in[ch] * dryG + wet[ch] * wetG) * outG;
        }

        if (++writePos >= bufSize)
            writePos = 0;
    }
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
