// The Mask - editor. Walnut cabinet, polished silver faceplate, tube-amp dials.
// Layout matches "Option B" of the design mockup (1040 x 904).
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "PluginProcessor.h"

namespace ui
{
    // palette
    const juce::Colour ink       { 0xff1c1a17 };
    const juce::Colour cream     { 0xffe8d9a8 };
    const juce::Colour brassHi   { 0xfff3da94 };
    const juce::Colour brassMid  { 0xffb98a2e };
    const juce::Colour brassLo   { 0xff6e4f17 };
    const juce::Colour amber     { 0xffffb347 };
    const juce::Colour redHot    { 0xffff6b5e };
    const juce::Colour redSoft   { 0xffff8a7e };
    const juce::Colour teal      { 0xff5cc8c0 };
    const juce::Colour green     { 0xff7fd6a0 };
    const juce::Colour dim       { 0xffa59f94 };
    const juce::Colour darkPanel { 0xff14110d };
    const juce::Colour steel     { 0xff9ba7b8 };

    juce::Font font (float height, bool bold = false);
    juce::Font display (float height);               // condensed "stencil" face for titles
    juce::Font mono (float height);
}

//==============================================================================
class TubeLookAndFeel : public juce::LookAndFeel_V4
{
public:
    TubeLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool, bool) override;

    // reusable pieces
    static void drawKnob (juce::Graphics&, juce::Rectangle<float> bounds, float normalisedPos, bool chrome, juce::Colour scaleColour = ui::ink, bool numerals = true);
    static void drawBakelite (juce::Graphics&, juce::Rectangle<float>, float radius, bool lit);
    static void drawChromeFrame (juce::Graphics&, juce::Rectangle<float>, float radius);
    static void drawScrew (juce::Graphics&, juce::Point<float> centre, float diameter, float angleRadians);
};

//==============================================================================
class SpectrumDisplay : public juce::Component, private juce::Timer
{
public:
    explicit SpectrumDisplay (themask::MaskEngine&);
    void paint (juce::Graphics&) override;

private:
    void timerCallback() override;

    themask::MaskEngine& engine_;
    static constexpr int N = themask::MaskEngine::kGrid;
    std::array<float, N> in_, out_, env_, red_;
};

//==============================================================================
class VuMeter : public juce::Component, private juce::Timer
{
public:
    explicit VuMeter (themask::MaskEngine&);
    void paint (juce::Graphics&) override;

private:
    void timerCallback() override;
    themask::MaskEngine& engine_;
    float needle_ = 0.0f, grDb_ = 0.0f;
};

//==============================================================================
class ValveIndicator : public juce::Component
{
public:
    void paint (juce::Graphics&) override;
    void setGlowing (bool on) { if (glowing_ != on) { glowing_ = on; repaint(); } }
private:
    bool glowing_ = true;
};

//==============================================================================
class LimiterPanel : public juce::Component, private juce::Timer
{
public:
    explicit LimiterPanel (TheMaskAudioProcessor&);
    ~LimiterPanel() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    struct Cell { juce::Slider slider; juce::String label; bool chrome = false;
                  std::unique_ptr<juce::SliderParameterAttachment> attach; };

    TheMaskAudioProcessor& proc_;
    juce::TextButton onButton_ { "On" };
    std::unique_ptr<juce::ParameterAttachment> onAttach_;
    std::array<Cell, 4> cells_;
    float inLevel_ = -60.0f, outLevel_ = -60.0f, grDb_ = 0.0f;
    float inHold_ = -60.0f, outHold_ = -60.0f, grHold_ = 0.0f;
};

//==============================================================================
class GluePanel : public juce::Component
{
public:
    explicit GluePanel (TheMaskAudioProcessor&);
    ~GluePanel() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    TheMaskAudioProcessor& proc_;
    juce::Slider dial_;
    std::unique_ptr<juce::SliderParameterAttachment> attach_;
    juce::TextButton minus_ { "-" }, plus_ { "+" };
    std::array<bool, 4> included_ { { true, true, true, true } };
    std::array<juce::TextButton, 4> toggles_;
};

//==============================================================================
class TheMaskAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit TheMaskAudioProcessorEditor (TheMaskAudioProcessor&);
    ~TheMaskAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void buildBackground();
    void showPresetMenu();

    struct KnobCell { juce::Slider slider; juce::String label; bool chrome = false;
                      std::unique_ptr<juce::SliderParameterAttachment> attach; };

    TheMaskAudioProcessor& proc_;
    TubeLookAndFeel laf_;
    juce::Image background_;

    SpectrumDisplay spectrum_;
    VuMeter vu_;
    ValveIndicator valve_;
    LimiterPanel limiter_;
    GluePanel glue_;

    std::array<KnobCell, 8> knobs_;
    std::array<juce::TextButton, 3> modeButtons_;
    juce::TextButton presetButton_;
    juce::TextButton bypassButton_ { "" };
    juce::TextButton learnButton_ { "Learn" };
    juce::TextButton deltaButton_ { "Hear what's cut" };
    juce::String learnStatus_;

    std::unique_ptr<juce::ParameterAttachment> modeAttach_, bypassAttach_, deltaAttach_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TheMaskAudioProcessorEditor)
};
