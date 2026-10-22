#include "PluginProcessor.h"
#include "PluginEditor.h"

using juce::AudioParameterBool;
using juce::AudioParameterChoice;
using juce::AudioParameterFloat;
using juce::NormalisableRange;
using juce::ParameterID;

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout TheMaskAudioProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    auto pct = [] (float v, int) { return juce::String (juce::roundToInt (v * 100.0f)) + " %"; };
    auto ms  = [] (float v, int) { return juce::String (juce::roundToInt (v)) + " ms"; };
    auto hz  = [] (float v, int) { return v >= 1000.0f ? juce::String (v / 1000.0f, 1) + " kHz" : juce::String (juce::roundToInt (v)) + " Hz"; };
    auto db  = [] (float v, int) { return juce::String (v, 1) + " dB"; };

    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ids::depth, 1 }, "Depth",
        NormalisableRange<float> (0.0f, 1.0f), 0.62f, juce::AudioParameterFloatAttributes().withStringFromValueFunction (pct)));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ids::sharpness, 1 }, "Sharpness",
        NormalisableRange<float> (0.0f, 1.0f), 0.48f));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ids::attack, 1 }, "Attack",
        NormalisableRange<float> (1.0f, 100.0f, 0.0f, 0.4f), 12.0f, juce::AudioParameterFloatAttributes().withStringFromValueFunction (ms)));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ids::release, 1 }, "Release",
        NormalisableRange<float> (10.0f, 500.0f, 0.0f, 0.5f), 80.0f, juce::AudioParameterFloatAttributes().withStringFromValueFunction (ms)));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ids::transient, 1 }, "Transient Protect",
        NormalisableRange<float> (0.0f, 1.0f), 0.40f, juce::AudioParameterFloatAttributes().withStringFromValueFunction (pct)));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ids::lowHz, 1 }, "Low",
        NormalisableRange<float> (20.0f, 2000.0f, 0.0f, 0.35f), 120.0f, juce::AudioParameterFloatAttributes().withStringFromValueFunction (hz)));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ids::highHz, 1 }, "High",
        NormalisableRange<float> (2000.0f, 20000.0f, 0.0f, 0.5f), 12000.0f, juce::AudioParameterFloatAttributes().withStringFromValueFunction (hz)));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ids::mix, 1 }, "Mix",
        NormalisableRange<float> (0.0f, 1.0f), 1.0f, juce::AudioParameterFloatAttributes().withStringFromValueFunction (pct)));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ids::mode, 1 }, "Mode",
        juce::StringArray { "Live", "Learn", "Both" }, 2));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { ids::delta, 1 }, "Hear What's Cut (Delta)", false));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { ids::bypass, 1 }, "Bypass", false));

    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ids::glue, 1 }, "Glue",
        NormalisableRange<float> (0.0f, 1.0f), 0.6f, juce::AudioParameterFloatAttributes().withStringFromValueFunction (pct)));

    layout.add (std::make_unique<AudioParameterBool> (ParameterID { ids::limOn, 1 }, "Limiter On", true));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ids::limGain, 1 }, "Limiter Gain",
        NormalisableRange<float> (-12.0f, 12.0f, 0.0f), 0.0f, juce::AudioParameterFloatAttributes().withStringFromValueFunction (db)));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ids::limThresh, 1 }, "Limiter Threshold",
        NormalisableRange<float> (-24.0f, 0.0f, 0.0f), -6.0f, juce::AudioParameterFloatAttributes().withStringFromValueFunction (db)));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ids::limCeiling, 1 }, "Limiter Ceiling",
        NormalisableRange<float> (-6.0f, 0.0f, 0.0f), -0.3f, juce::AudioParameterFloatAttributes().withStringFromValueFunction (db)));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ids::limRelease, 1 }, "Limiter Release",
        NormalisableRange<float> (10.0f, 500.0f, 0.0f, 0.5f), 120.0f, juce::AudioParameterFloatAttributes().withStringFromValueFunction (ms)));
    return layout;
}

//==============================================================================
TheMaskAudioProcessor::TheMaskAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",     juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output",    juce::AudioChannelSet::stereo(), true)
                          .withInput  ("Sidechain", juce::AudioChannelSet::stereo(), false)),
      apvts (*this, nullptr, "THEMASK", createLayout())
{
    pDepth_ = apvts.getRawParameterValue (ids::depth);        pSharp_ = apvts.getRawParameterValue (ids::sharpness);
    pAttack_ = apvts.getRawParameterValue (ids::attack);      pRelease_ = apvts.getRawParameterValue (ids::release);
    pTransient_ = apvts.getRawParameterValue (ids::transient); pLow_ = apvts.getRawParameterValue (ids::lowHz);
    pHigh_ = apvts.getRawParameterValue (ids::highHz);        pMix_ = apvts.getRawParameterValue (ids::mix);
    pMode_ = apvts.getRawParameterValue (ids::mode);          pDelta_ = apvts.getRawParameterValue (ids::delta);
    pBypass_ = apvts.getRawParameterValue (ids::bypass);
    pLimOn_ = apvts.getRawParameterValue (ids::limOn);        pLimGain_ = apvts.getRawParameterValue (ids::limGain);
    pLimThresh_ = apvts.getRawParameterValue (ids::limThresh); pLimCeiling_ = apvts.getRawParameterValue (ids::limCeiling);
    pLimRelease_ = apvts.getRawParameterValue (ids::limRelease);
}

juce::AudioProcessorParameter* TheMaskAudioProcessor::getBypassParameter() const
{
    return apvts.getParameter (ids::bypass);
}

bool TheMaskAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& in  = layouts.getMainInputChannelSet();
    const auto& out = layouts.getMainOutputChannelSet();
    if (in != out) return false;
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo()) return false;

    const auto& sc = layouts.getChannelSet (true, 1);          // sidechain (Glue reference)
    return sc.isDisabled() || sc == juce::AudioChannelSet::mono() || sc == juce::AudioChannelSet::stereo();
}

void TheMaskAudioProcessor::prepareToPlay (double sampleRate, int)
{
    engine_.prepare (sampleRate, getMainBusNumOutputChannels());
    limiter_.prepare (sampleRate);
    setLatencySamples (themask::MaskEngine::kLatency);
}

void TheMaskAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    auto main = getBusBuffer (buffer, false, 0);
    const int nCh = main.getNumChannels();
    const int n   = main.getNumSamples();

    themask::MaskParams mp;
    mp.depth     = pDepth_->load();
    mp.sharpness = pSharp_->load();
    mp.attackMs  = pAttack_->load();
    mp.releaseMs = pRelease_->load();
    mp.transient = pTransient_->load();
    mp.lowHz     = pLow_->load();
    mp.highHz    = pHigh_->load();
    mp.mix       = pMix_->load();
    mp.mode      = juce::jlimit (0, 2, (int) std::lround (pMode_->load()));
    mp.delta     = pDelta_->load() > 0.5f;
    mp.bypass    = pBypass_->load() > 0.5f;
    engine_.setParams (mp);
    engine_.process (main.getArrayOfWritePointers(), nCh, n);

    themask::LimiterParams lp;
    lp.on        = pLimOn_->load() > 0.5f && ! mp.bypass;
    lp.gainDb    = mp.bypass ? 0.0f : pLimGain_->load();
    lp.threshDb  = pLimThresh_->load();
    lp.ceilingDb = pLimCeiling_->load();
    lp.releaseMs = pLimRelease_->load();
    limiter_.setParams (lp);
    limiter_.process (main.getArrayOfWritePointers(), nCh, n);
}

//==============================================================================
// Presets: settings only. (A preset can also carry a learned curve once the preset browser is built.)
const std::vector<TheMaskAudioProcessor::Preset>& TheMaskAudioProcessor::getPresets()
{
    static const std::vector<Preset> presets
    {
        //  category   name                depth sharp atk  rel   trans low    high
        { "Vocals", "Tame Harshness",   0.62f, 0.48f, 12.f,  80.f, 0.40f,  150.f, 12000.f },
        { "Vocals", "De-box",           0.55f, 0.35f, 20.f, 120.f, 0.30f,  180.f,  1200.f },
        { "Vocals", "Gentle Air",       0.35f, 0.30f, 25.f, 150.f, 0.30f, 5000.f, 16000.f },
        { "Bass",   "Tighten Low End",  0.50f, 0.40f, 30.f, 200.f, 0.20f,   40.f,   400.f },
        { "Bass",   "Room Mode Fix",    0.70f, 0.60f, 40.f, 250.f, 0.15f,   40.f,   250.f },
        { "Bass",   "Sub Clean-up",     0.45f, 0.30f, 40.f, 300.f, 0.10f,   25.f,   120.f },
        { "Drums",  "Kill Snare Ring",  0.80f, 0.70f,  4.f,  40.f, 0.70f,  200.f,  6000.f },
        { "Drums",  "Tame Cymbals",     0.55f, 0.45f,  6.f,  60.f, 0.60f, 4000.f, 18000.f },
        { "Drums",  "Tom Cleanup",      0.70f, 0.65f,  5.f,  50.f, 0.65f,   80.f,  1500.f },
        { "Synths", "Smooth Saw",       0.45f, 0.35f, 15.f, 100.f, 0.30f,  300.f, 16000.f },
        { "Synths", "Tame Resonance",   0.65f, 0.65f, 10.f,  90.f, 0.25f,  150.f, 12000.f },
        { "Synths", "Pad Clean-up",     0.40f, 0.30f, 40.f, 220.f, 0.10f,  120.f,  6000.f },
    };
    return presets;
}

void TheMaskAudioProcessor::applyPreset (const Preset& p)
{
    auto set = [this] (const char* id, float value)
    {
        if (auto* prm = apvts.getParameter (id))
            prm->setValueNotifyingHost (prm->convertTo0to1 (value));
    };
    set (ids::depth, p.depth);       set (ids::sharpness, p.sharpness);
    set (ids::attack, p.attack);     set (ids::release, p.release);
    set (ids::transient, p.transient);
    set (ids::lowHz, p.low);         set (ids::highHz, p.high);
    currentPresetName = juce::String (p.category) + "  /  " + p.name;
}

//==============================================================================
void TheMaskAudioProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();

    float curve[themask::MaskEngine::kGrid];
    engine_.getLearnedCurve (curve);
    juce::MemoryBlock mb (curve, sizeof (curve));
    state.setProperty ("learnedCurve", mb.toBase64Encoding(), nullptr);
    state.setProperty ("learnedValid", engine_.hasLearnedCurve(), nullptr);
    state.setProperty ("presetName", currentPresetName, nullptr);

    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, dest);
}

void TheMaskAudioProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
    {
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto state = juce::ValueTree::fromXml (*xml);

            juce::MemoryBlock mb;
            if (mb.fromBase64Encoding (state.getProperty ("learnedCurve").toString())
                && mb.getSize() == sizeof (float) * (size_t) themask::MaskEngine::kGrid)
            {
                float curve[themask::MaskEngine::kGrid];
                std::memcpy (curve, mb.getData(), sizeof (curve));
                engine_.setLearnedCurve (curve, (bool) state.getProperty ("learnedValid", false));
            }
            if (state.hasProperty ("presetName"))
                currentPresetName = state.getProperty ("presetName").toString();

            apvts.replaceState (state);
        }
    }
}

juce::AudioProcessorEditor* TheMaskAudioProcessor::createEditor()
{
    return new TheMaskAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TheMaskAudioProcessor();
}
