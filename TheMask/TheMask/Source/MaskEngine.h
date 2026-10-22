// The Mask - resonance suppressor DSP core.
//
// Pure C++17, no JUCE dependency, so it can be unit-tested on its own.
// Real-time safe: all memory is allocated in prepare(); process() never
// allocates, locks or does I/O.
//
// Overview
//   * Overlap-add STFT (2048 point, 75% overlap, sqrt-Hann analysis/synthesis).
//   * Channels are analysed together (linked detection) so the stereo image
//     is not disturbed by the mask.
//   * A smoothed spectral envelope is built on a log-frequency scale. Bins that
//     poke out above it by more than a threshold are treated as resonances and
//     attenuated, scaled by Depth.
//   * "Learn" averages the excess over a few seconds and freezes it into a
//     static curve that is stored with the project.
//   * "Transient protect" backs the mask off when a sudden attack arrives.
//   * Reported latency is kLatency samples.
#pragma once

#include <array>
#include <atomic>
#include <cstdint>
#include <vector>

namespace themask
{

struct MaskParams
{
    enum Mode { Live = 0, Learn = 1, Both = 2 };

    float depth      = 0.62f;    // 0..1   how much of each resonance is removed
    float sharpness  = 0.48f;    // 0..1   higher = only narrower peaks count
    float attackMs   = 12.0f;
    float releaseMs  = 80.0f;
    float transient  = 0.40f;    // 0..1   transient protection amount
    float lowHz      = 120.0f;   // mask is active between lowHz and highHz
    float highHz     = 12000.0f;
    float mix        = 1.0f;     // 0..1   dry/wet
    int   mode       = Both;
    bool  delta      = false;    // output only what is being removed
    bool  bypass     = false;    // latency-compensated passthrough
};

class MaskEngine
{
public:
    static constexpr int kFftOrder = 11;
    static constexpr int kFftSize  = 1 << kFftOrder;      // 2048
    static constexpr int kHop      = kFftSize / 4;        // 512
    static constexpr int kBins     = kFftSize / 2 + 1;    // 1025
    static constexpr int kMaxCh    = 2;
    static constexpr int kLatency  = kFftSize;            // samples
    static constexpr int kGrid     = 256;                 // log grid, 20 Hz .. 20 kHz
    static constexpr float kGridLoHz = 20.0f;
    static constexpr float kGridHiHz = 20000.0f;

    MaskEngine();

    void prepare (double sampleRate, int numChannels);
    void reset();

    // Audio thread. Call once per block before process().
    void setParams (const MaskParams& p);

    // Audio thread. In-place processing.
    void process (float* const* channels, int numChannels, int numSamples);

    // ---- Learn (message thread) ----------------------------------------
    void startLearning()               { learnRequested_.store (true); }
    void stopLearning()                { learnRequested_.store (false); }
    bool isLearning() const            { return learnRequested_.load(); }
    int  getLearnedFrames() const      { return learnFrames_.load(); }   // frames accumulated so far
    int  getLearnedResonances() const  { return learnedResonances_.load(); }
    bool hasLearnedCurve() const       { return learnedValid_.load(); }

    // Curve of persistent excess (dB, >= 0) on the log grid; used for saving/restoring state.
    void getLearnedCurve (float* dest) const;                 // kGrid floats
    void setLearnedCurve (const float* src, bool valid);      // kGrid floats
    void clearLearnedCurve();

    // ---- Metering / display (any thread, relaxed reads) ---------------
    float getReductionDb() const       { return grDb_.load (std::memory_order_relaxed); }       // <= 0, peak
    void  getDisplay (float* inDb, float* outDb, float* envDb, float* redDb) const;             // kGrid floats each
    static float gridFrequency (int g);

    double getSampleRate() const       { return sampleRate_; }

private:
    void processFrame();
    void fft (float* re, float* im, bool inverse) const;
    void rebuildBinTables();
    void rebuildLearnedBins();
    void publishDisplay();
    void finaliseLearning();

    static constexpr float kThresholdDb = 4.0f;    // how far a bin must stick out before it counts
    static constexpr float kMaxReduceDb = 18.0f;
    static constexpr float kAvgCoef     = 0.6f;    // time-average of the detection spectrum

    double sampleRate_ = 48000.0;
    int    numCh_      = 2;
    MaskParams p_;

    // fft tables
    std::vector<float> win_, twRe_, twIm_;
    std::vector<int>   bitrev_;
    float winSum_ = 1.0f;

    // streaming state
    std::array<std::vector<float>, kMaxCh> inBuf_, outBuf_;
    std::array<std::vector<float>, kMaxCh> re_, im_;
    int pos_ = 0;
    int hopCount_ = 0;

    // per-frame working buffers (preallocated)
    std::vector<float> magDb_, avgDb_, env0_, env1_, clipped_, prefix_, target_, tmp_, applied_;
    std::vector<float> state_;              // smoothed reduction per bin (dB, <= 0)
    std::vector<float> rangeW_;             // frequency-range weight per bin
    std::vector<int>   halfWidth_;          // envelope half width per bin
    std::vector<float> binFreq_;
    std::vector<float> learnedBins_;        // learned excess per bin (dB)
    std::vector<float> learnSum_;           // accumulated excess while learning
    std::vector<int>   gridLo_, gridHi_;    // bin range per grid point

    float lastSharp_ = -1.0f, lastLow_ = -1.0f, lastHigh_ = -1.0f;
    float eAvg_ = 0.0f, tSm_ = 0.0f;
    bool  wasLearning_ = false;

    // thread-shared
    std::atomic<bool> learnRequested_ { false };
    std::atomic<int>  learnFrames_ { 0 };
    std::atomic<int>  learnedResonances_ { 0 };
    std::atomic<bool> learnedValid_ { false };
    std::atomic<bool> curveDirty_ { false };
    std::array<std::atomic<float>, kGrid> curve_;
    std::array<std::atomic<float>, kGrid> dispIn_, dispOut_, dispEnv_, dispRed_;
    std::atomic<float> grDb_ { 0.0f };
};

} // namespace themask
