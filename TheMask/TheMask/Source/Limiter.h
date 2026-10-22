// Basic peak limiter used by the right-hand "Limiter" column.
// First-pass design: instant attack (no look-ahead), smoothed release, hard ceiling.
// A look-ahead / true-peak version is planned for the next milestone.
#pragma once

#include <array>
#include <atomic>

namespace themask
{

struct LimiterParams
{
    bool  on        = true;
    float gainDb    = 0.0f;     // input gain, -12 .. +12
    float threshDb  = -6.0f;    // limiting starts here
    float ceilingDb = -0.3f;    // output never exceeds this
    float releaseMs = 120.0f;
};

class Limiter
{
public:
    void prepare (double sampleRate);
    void reset();
    void setParams (const LimiterParams& p) { p_ = p; }
    void process (float* const* channels, int numChannels, int numSamples);

    // meters (linear peak, held/decayed by the reader)
    float getInputPeak()  { return inPeak_.exchange (0.0f); }
    float getOutputPeak() { return outPeak_.exchange (0.0f); }
    float getReductionDb() const { return grDb_.load (std::memory_order_relaxed); }   // <= 0

private:
    LimiterParams p_;
    double sampleRate_ = 48000.0;
    float  env_ = 1.0f;
    std::atomic<float> inPeak_ { 0.0f }, outPeak_ { 0.0f }, grDb_ { 0.0f };
};

} // namespace themask
