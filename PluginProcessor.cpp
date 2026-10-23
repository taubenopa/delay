#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    constexpr float kTwoPi = juce::MathConstants<float>::twoPi;
}

CompProcessor::CompProcessor()
    : AudioProcessor (BusesProperties()
          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout CompProcessor::createLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;

    auto addF = [&p] (const char* id, const char* name, float lo, float hi, float step, float skew, float def)
    {
        p.push_back (std::make_unique<AudioParameterFloat> (
            ParameterID { id, 1 }, name, NormalisableRange<float> (lo, hi, step, skew), def));
    };
    auto addB = [&p] (const char* id, const char* name, bool def)
    {
        p.push_back (std::make_unique<AudioParameterBool> (ParameterID { id, 1 }, name, def));
    };

    addB ("bypass", "Bypass", false);
    addF ("input",     "Input",       -24.0f,  24.0f, 0.1f,  1.0f,   0.0f);
    addF ("threshold", "Threshold",   -60.0f,   0.0f, 0.1f,  1.0f, -20.0f);
    addF ("ratio",     "Ratio",         1.0f,  20.0f, 0.1f,  0.6f,   4.0f);
    addF ("attack",    "Attack",        0.1f, 100.0f, 0.01f, 0.4f,   5.0f);
    addF ("release",   "Release",      10.0f,1000.0f, 1.0f,  0.4f, 120.0f);
    addF ("knee",      "Knee",          0.0f,  24.0f, 0.1f,  1.0f,   6.0f);
    addF ("makeup",    "Makeup",        0.0f,  24.0f, 0.1f,  1.0f,   0.0f);
    addB ("automakeup",  "Auto Makeup",  true);
    addB ("autorelease", "Auto Release", false);
    p.push_back (std::make_unique<AudioParameterChoice> (
        ParameterID { "mode", 1 }, "Mode", StringArray { "VCA", "FET", "OPTO" }, 0));
    addF ("detector",  "Detector",      0.0f,   1.0f, 0.001f, 1.0f,  0.3f);
    addF ("schpf",     "SC HPF",       20.0f, 500.0f, 1.0f,  0.5f,  90.0f);
    addB ("sclisten",  "SC Listen",    false);
    addF ("link",      "Stereo Link",   0.0f,   1.0f, 0.001f, 1.0f,  1.0f);
    addF ("deess",     "De-Ess",        0.0f,   1.0f, 0.001f, 1.0f,  0.0f);
    addF ("deessfreq", "De-Ess Freq", 3000.0f,10000.0f, 10.0f, 1.0f, 6500.0f);
    addF ("body",      "Body",         -6.0f,   6.0f, 0.1f,  1.0f,   0.0f);
    addF ("air",       "Air",          -6.0f,  10.0f, 0.1f,  1.0f,   0.0f);
    addF ("drive",     "Drive",         0.0f,   1.0f, 0.001f, 1.0f,  0.1f);
    addF ("mix",       "Mix",           0.0f,   1.0f, 0.001f, 1.0f,  1.0f);
    addB ("limiter",   "Limiter",      true);
    addF ("ceiling",   "Ceiling",     -12.0f,   0.0f, 0.1f,  1.0f,  -1.0f);
    addF ("output",    "Output",      -24.0f,  12.0f, 0.1f,  1.0f,   0.0f);

    return { p.begin(), p.end() };
}

void CompProcessor::prepareToPlay (double newSampleRate, int)
{
    sampleRateHz = newSampleRate;

    auto get = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };
    auto dbToGain = [] (float db) { return juce::Decibels::decibelsToGain (db); };

    sInput.reset (sampleRateHz, 0.03);   sInput.setCurrentAndTargetValue (dbToGain (get ("input")));
    sOutput.reset (sampleRateHz, 0.03);  sOutput.setCurrentAndTargetValue (dbToGain (get ("output")));
    sMix.reset (sampleRateHz, 0.03);     sMix.setCurrentAndTargetValue (get ("mix"));
    sDrive.reset (sampleRateHz, 0.03);   sDrive.setCurrentAndTargetValue (get ("drive"));
    sDeess.reset (sampleRateHz, 0.03);   sDeess.setCurrentAndTargetValue (get ("deess"));
    sMakeup.reset (sampleRateHz, 0.05);  sMakeup.setCurrentAndTargetValue (1.0f);
    sBypass.reset (sampleRateHz, 0.02);  sBypass.setCurrentAndTargetValue (get ("bypass") > 0.5f ? 1.0f : 0.0f);

    for (int c = 0; c < 2; ++c)
    {
        lpA[c] = lpB[c] = envHi[c] = envFull[c] = deState[c] = 0.0f;
        hpA[c] = hpB[c] = msq[c] = grState[c] = dcX[c] = dcY[c] = 0.0f;
    }
    lowShelf.reset();
    highShelf.reset();
    limGain = 1.0f;
}

bool CompProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return out == layouts.getMainInputChannelSet();
}

void CompProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    const int numCh      = juce::jmin (buffer.getNumChannels(), 2);

    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, numSamples);
    if (numCh < 1)
        return;

    auto get = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };
    auto dbToGain = [] (float db) { return juce::Decibels::decibelsToGain (db); };

    // ---------------- Parameter ----------------
    const float thr        = get ("threshold");
    const float ratioParam = get ("ratio");
    const float ratio      = ratioParam >= 19.9f ? 1000.0f : ratioParam;
    const float knee       = get ("knee");
    const float attackMs   = get ("attack");
    const float releaseMs  = get ("release");
    const int   mode       = juce::jlimit (0, 2, (int) get ("mode"));
    const bool  autoRel    = get ("autorelease") > 0.5f;
    const bool  autoMk     = get ("automakeup") > 0.5f;
    const bool  scListen   = get ("sclisten") > 0.5f;
    const bool  limiterOn  = get ("limiter") > 0.5f;
    const float link       = get ("link");
    const float detMix     = get ("detector");
    const float ceilLin    = dbToGain (get ("ceiling"));

    const float srF = (float) sampleRateHz;

    // Charakter der drei Modi
    const bool  opto      = (mode == 2);
    const float attScale  = (mode == 0) ? 1.0f : (mode == 1 ? 0.4f : 2.5f);
    const float relScale  = (mode == 0) ? 1.0f : (mode == 1 ? 0.8f : 1.5f);
    const float driveAdd  = (mode == 1) ? 0.12f : 0.0f;

    // Koeffizienten
    const float attCoef = std::exp (-1.0f / (juce::jmax (0.03f, attackMs * attScale) * 0.001f * srF));
    const float aHp     = std::exp (-kTwoPi * get ("schpf") / srF);
    const float aDe     = std::exp (-kTwoPi * get ("deessfreq") / srF);
    const float rmsCoef = std::exp (-1.0f / (0.010f * srF));
    const float envRel  = std::exp (-1.0f / (0.020f * srF));
    const float deAtt   = std::exp (-1.0f / (0.0004f * srF));
    const float deRel   = std::exp (-1.0f / (0.030f * srF));
    const float limRel  = std::exp (-1.0f / (0.080f * srF));
    const float dcR     = 1.0f - kTwoPi * 5.0f / srF;

    lowShelf.setLowShelf (sampleRateHz, 150.0, get ("body"));
    highShelf.setHighShelf (sampleRateHz, juce::jmin (9000.0, sampleRateHz * 0.45), get ("air"));

    // Auto-Makeup: grobe Schaetzung aus Threshold und Ratio
    const float makeupDb = autoMk ? juce::jlimit (0.0f, 24.0f, -thr * (1.0f - 1.0f / ratio) * 0.5f)
                                  : get ("makeup");

    sInput.setTargetValue (dbToGain (get ("input")));
    sOutput.setTargetValue (dbToGain (get ("output")));
    sMix.setTargetValue (get ("mix"));
    sDrive.setTargetValue (get ("drive"));
    sDeess.setTargetValue (get ("deess"));
    sMakeup.setTargetValue (dbToGain (makeupDb));
    sBypass.setTargetValue (get ("bypass") > 0.5f ? 1.0f : 0.0f);

    // Release-Zeit: manuell, automatisch (programmabhaengig) oder Opto-Verhalten
    auto relCoefFor = [&] (float st)
    {
        const float depth = juce::jlimit (0.0f, 1.0f, st / 12.0f);
        float tauMs;
        if (autoRel)    tauMs = 40.0f + 410.0f * depth;
        else if (opto)  tauMs = releaseMs * 1.5f * (0.6f + 0.9f * depth);
        else            tauMs = releaseMs * relScale;
        return std::exp (-1.0f / (tauMs * 0.001f * srF));
    };

    float* io[2] = { buffer.getWritePointer (0), numCh > 1 ? buffer.getWritePointer (1) : nullptr };

    float inPk = 0.0f, outPk = 0.0f, grPk = 0.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        const float inGain = sInput.getNextValue();
        const float outG   = sOutput.getNextValue();
        const float mixV   = sMix.getNextValue();
        const float driveV = juce::jlimit (0.0f, 1.0f, sDrive.getNextValue() + driveAdd);
        const float deAmt  = sDeess.getNextValue();
        const float mkG    = sMakeup.getNextValue();
        const float byp    = sBypass.getNextValue();

        float orig[2] = { 0.0f, 0.0f }, xg[2] = { 0.0f, 0.0f }, xd[2] = { 0.0f, 0.0f };
        float sc[2]   = { 0.0f, 0.0f }, det[2] = { 0.0f, 0.0f };

        // 1) Eingang, De-Esser, Sidechain-Detektor
        for (int c = 0; c < numCh; ++c)
        {
            orig[c] = io[c][i];
            xg[c]   = orig[c] * inGain;
            inPk    = juce::jmax (inPk, std::abs (xg[c]));

            // De-Esser: Hoehen abtrennen und nur dort absenken
            lpA[c] = (1.0f - aDe) * xg[c] + aDe * lpA[c];
            lpB[c] = (1.0f - aDe) * lpA[c] + aDe * lpB[c];
            const float hi = xg[c] - lpB[c];
            envHi[c]   = juce::jmax (std::abs (hi),   envHi[c]   * envRel);
            envFull[c] = juce::jmax (std::abs (xg[c]), envFull[c] * envRel);

            float redDe = 0.0f;
            if (deAmt > 0.0005f)
            {
                const float ratioDb = 20.0f * std::log10 ((envHi[c] + 1.0e-6f) / (envFull[c] + 1.0e-6f));
                redDe = juce::jmin (15.0f, deAmt * 1.6f * juce::jmax (0.0f, ratioDb + 10.0f));
            }
            const float cDe = redDe > deState[c] ? deAtt : deRel;
            deState[c] = cDe * deState[c] + (1.0f - cDe) * redDe;
            xd[c] = lpB[c] + hi * std::pow (10.0f, -deState[c] / 20.0f);

            // Sidechain: 12 dB/Okt Hochpass, dann Peak/RMS-Mischung
            hpA[c] = aHp * hpA[c] + (1.0f - aHp) * xd[c];
            const float s1 = xd[c] - hpA[c];
            hpB[c] = aHp * hpB[c] + (1.0f - aHp) * s1;
            sc[c]  = s1 - hpB[c];

            msq[c] = rmsCoef * msq[c] + (1.0f - rmsCoef) * sc[c] * sc[c];
            const float peak = std::abs (sc[c]);
            const float rms  = std::sqrt (msq[c]) * 1.4142f;
            det[c] = peak * (1.0f - detMix) + rms * detMix;
        }

        float mx = det[0];
        if (numCh > 1) mx = juce::jmax (mx, det[1]);

        // 2) Kompression, Saettigung, EQ, Mix
        float out[2] = { 0.0f, 0.0f };
        for (int c = 0; c < numCh; ++c)
        {
            const float d       = det[c] + (mx - det[c]) * link;
            const float levelDb = 20.0f * std::log10 (juce::jmax (d, 1.0e-6f));
            const float over    = levelDb - thr;

            float red = 0.0f;
            if (2.0f * over >= -knee)
            {
                const float slope = 1.0f - 1.0f / ratio;
                if (knee > 0.01f && 2.0f * std::abs (over) <= knee)
                {
                    const float t = over + knee * 0.5f;
                    red = slope * t * t / (2.0f * knee);
                }
                else
                {
                    red = slope * over;
                }
            }
            if (red < 0.0f) red = 0.0f;

            float& st = grState[c];
            const float coef = red > st ? attCoef : relCoefFor (st);
            st = coef * st + (1.0f - coef) * red;
            grPk = juce::jmax (grPk, st);

            float y = xd[c] * std::pow (10.0f, -st / 20.0f) * mkG;

            if (driveV > 0.001f)
            {
                const float a    = 1.0f + 4.0f * driveV;
                const float bias = 0.12f * driveV;
                const float tb   = std::tanh (bias);
                y = (std::tanh (a * y + bias) - tb) / (a * (1.0f - tb * tb));
            }

            const float yb = y - dcX[c] + dcR * dcY[c];
            dcX[c] = y;
            dcY[c] = yb;
            y = yb;

            y = lowShelf.process (c, y);
            y = highShelf.process (c, y);

            float o = (xg[c] * (1.0f - mixV) + y * mixV) * outG;
            if (scListen)
                o = sc[c] * outG;
            out[c] = o;
        }

        // 3) Limiter (sofortiger Einsatz, 80 ms Release) und Bypass
        float limApplied = 1.0f;
        if (limiterOn && ! scListen)
        {
            float pk = std::abs (out[0]);
            if (numCh > 1) pk = juce::jmax (pk, std::abs (out[1]));
            const float target = pk > ceilLin ? ceilLin / pk : 1.0f;
            if (target < limGain) limGain = target;
            else                  limGain = limRel * limGain + (1.0f - limRel) * target;
            limApplied = limGain;
        }
        else
        {
            limGain = 1.0f;
        }

        for (int c = 0; c < numCh; ++c)
        {
            float o = out[c] * limApplied;
            o = o * (1.0f - byp) + orig[c] * byp;
            outPk = juce::jmax (outPk, std::abs (o));
            io[c][i] = o;
        }
    }

    if (inPk  > inPeak.load())  inPeak.store (inPk);
    if (outPk > outPeak.load()) outPeak.store (outPk);
    if (grPk  > grPeak.load())  grPeak.store (grPk);
}

juce::AudioProcessorEditor* CompProcessor::createEditor()
{
    return new CompEditor (*this);
}

void CompProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void CompProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CompProcessor();
}
