#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "Limiter.h"
#include "MaskEngine.h"

namespace ids
{
    inline constexpr const char* depth      = "depth";
    inline constexpr const char* sharpness  = "sharpness";
    inline constexpr const char* attack     = "attack";
    inline constexpr const char* release    = "release";
    inline constexpr const char* transient  = "transient";
    inline constexpr const char* lowHz      = "lowHz";
    inline constexpr const char* highHz     = "highHz";
    inline constexpr const char* mix        = "mix";
    inline constexpr const char* mode       = "mode";
    inline constexpr const char* delta      = "delta";
    inline constexpr const char* bypass     = "bypass";
    inline constexpr const char* glue       = "glue";
    inline constexpr const char* limOn      = "limOn";
    inline constexpr const char* limGain    = "limGain";
    inline constexpr const char* limThresh  = "limThresh";
    inline constexpr const char* limCeiling = "limCeiling";
    inline constexpr const char* limRelease = "limRelease";
}

class TheMaskAudioProcessor : public juce::AudioProcessor
{
public:
    TheMaskAudioProcessor();
    ~TheMaskAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "The Mask"; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return (double) themask::MaskEngine::kLatency / getSampleRate(); }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorParameter* getBypassParameter() const override;

    // ---- for the editor ------------------------------------------------
    juce::AudioProcessorValueTreeState apvts;
    themask::MaskEngine& getEngine() { return engine_; }
    themask::Limiter& getLimiter()   { return limiter_; }

    struct Preset { const char* category; const char* name; float depth, sharpness, attack, release, transient, low, high; };
    static const std::vector<Preset>& getPresets();
    void applyPreset (const Preset& p);
    juce::String currentPresetName { "Vocals  /  Tame Harshness" };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    themask::MaskEngine engine_;
    themask::Limiter limiter_;

    std::atomic<float>* pDepth_ = nullptr; std::atomic<float>* pSharp_ = nullptr;
    std::atomic<float>* pAttack_ = nullptr; std::atomic<float>* pRelease_ = nullptr;
    std::atomic<float>* pTransient_ = nullptr; std::atomic<float>* pLow_ = nullptr;
    std::atomic<float>* pHigh_ = nullptr; std::atomic<float>* pMix_ = nullptr;
    std::atomic<float>* pMode_ = nullptr; std::atomic<float>* pDelta_ = nullptr;
    std::atomic<float>* pBypass_ = nullptr;
    std::atomic<float>* pLimOn_ = nullptr; std::atomic<float>* pLimGain_ = nullptr;
    std::atomic<float>* pLimThresh_ = nullptr; std::atomic<float>* pLimCeiling_ = nullptr;
    std::atomic<float>* pLimRelease_ = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TheMaskAudioProcessor)
};
