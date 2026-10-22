#include "MaskEngine.h"

#include <algorithm>
#include <cmath>

namespace themask
{

namespace
{
constexpr float kPi = 3.14159265358979323846f;

inline float clampf (float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
inline float dbToLin (float db) { return std::pow (10.0f, db * 0.05f); }
}

MaskEngine::MaskEngine()
{
    for (int g = 0; g < kGrid; ++g)
    {
        curve_[(size_t) g].store (0.0f);
        dispIn_[(size_t) g].store (-120.0f);
        dispOut_[(size_t) g].store (-120.0f);
        dispEnv_[(size_t) g].store (-120.0f);
        dispRed_[(size_t) g].store (0.0f);
    }
}

float MaskEngine::gridFrequency (int g)
{
    const float t = (float) g / (float) (kGrid - 1);
    return kGridLoHz * std::pow (kGridHiHz / kGridLoHz, t);
}

void MaskEngine::prepare (double sampleRate, int numChannels)
{
    sampleRate_ = sampleRate > 1000.0 ? sampleRate : 48000.0;
    numCh_ = std::max (1, std::min (kMaxCh, numChannels));

    // sqrt-Hann (periodic) window: analysis * synthesis = Hann, which sums to 2 at 75% overlap.
    win_.assign ((size_t) kFftSize, 0.0f);
    winSum_ = 0.0f;
    for (int i = 0; i < kFftSize; ++i)
    {
        const float hann = 0.5f * (1.0f - std::cos (2.0f * kPi * (float) i / (float) kFftSize));
        win_[(size_t) i] = std::sqrt (hann);
        winSum_ += win_[(size_t) i];
    }

    twRe_.assign ((size_t) kFftSize / 2, 0.0f);
    twIm_.assign ((size_t) kFftSize / 2, 0.0f);
    for (int i = 0; i < kFftSize / 2; ++i)
    {
        twRe_[(size_t) i] =  std::cos (2.0f * kPi * (float) i / (float) kFftSize);
        twIm_[(size_t) i] = -std::sin (2.0f * kPi * (float) i / (float) kFftSize);
    }

    bitrev_.assign ((size_t) kFftSize, 0);
    for (int i = 0; i < kFftSize; ++i)
    {
        int r = 0;
        for (int b = 0; b < kFftOrder; ++b)
            if (i & (1 << b)) r |= 1 << (kFftOrder - 1 - b);
        bitrev_[(size_t) i] = r;
    }

    for (int c = 0; c < kMaxCh; ++c)
    {
        inBuf_[(size_t) c].assign ((size_t) kFftSize, 0.0f);
        outBuf_[(size_t) c].assign ((size_t) kFftSize, 0.0f);
        re_[(size_t) c].assign ((size_t) kFftSize, 0.0f);
        im_[(size_t) c].assign ((size_t) kFftSize, 0.0f);
    }

    const size_t nb = (size_t) kBins;
    magDb_.assign (nb, -120.0f);  avgDb_.assign (nb, -120.0f);  env0_.assign (nb, 0.0f);   env1_.assign (nb, 0.0f);
    clipped_.assign (nb, 0.0f);   prefix_.assign (nb + 1, 0.0f);
    target_.assign (nb, 0.0f);    tmp_.assign (nb, 0.0f);    applied_.assign (nb, 0.0f);
    state_.assign (nb, 0.0f);     rangeW_.assign (nb, 1.0f); halfWidth_.assign (nb, 3);
    binFreq_.assign (nb, 0.0f);   learnedBins_.assign (nb, 0.0f); learnSum_.assign (nb, 0.0f);
    gridLo_.assign ((size_t) kGrid, 0); gridHi_.assign ((size_t) kGrid, 0);

    lastSharp_ = lastLow_ = lastHigh_ = -1.0f;
    rebuildBinTables();
    curveDirty_.store (true);
    reset();
}

void MaskEngine::reset()
{
    for (int c = 0; c < kMaxCh; ++c)
    {
        std::fill (inBuf_[(size_t) c].begin(), inBuf_[(size_t) c].end(), 0.0f);
        std::fill (outBuf_[(size_t) c].begin(), outBuf_[(size_t) c].end(), 0.0f);
    }
    std::fill (state_.begin(), state_.end(), 0.0f);
    std::fill (avgDb_.begin(), avgDb_.end(), -120.0f);
    pos_ = 0;
    hopCount_ = 0;
    eAvg_ = 0.0f;
    tSm_ = 0.0f;
    grDb_.store (0.0f);
}

void MaskEngine::rebuildBinTables()
{
    const float binHz = (float) (sampleRate_ / (double) kFftSize);
    for (int k = 0; k < kBins; ++k)
        binFreq_[(size_t) k] = (float) k * binHz;

    for (int g = 0; g < kGrid; ++g)
    {
        const float f0 = gridFrequency (g);
        const float f1 = gridFrequency (std::min (g + 1, kGrid - 1));
        const float fPrev = gridFrequency (std::max (g - 1, 0));
        const float lo = std::sqrt (f0 * fPrev) / binHz;
        const float hi = std::sqrt (f0 * f1) / binHz;
        gridLo_[(size_t) g] = (int) clampf (std::floor (lo), 1.0f, (float) kBins - 1.0f);
        gridHi_[(size_t) g] = (int) clampf (std::ceil (hi), (float) gridLo_[(size_t) g], (float) kBins - 1.0f);
    }
}

void MaskEngine::setParams (const MaskParams& p)
{
    p_ = p;

    if (p_.sharpness != lastSharp_)
    {
        lastSharp_ = p_.sharpness;
        // Higher sharpness = narrower envelope window = only narrow peaks stand out.
        const float frac = 0.35f + (0.08f - 0.35f) * clampf (p_.sharpness, 0.0f, 1.0f);
        for (int k = 0; k < kBins; ++k)
            halfWidth_[(size_t) k] = std::max (3, (int) std::lround ((float) k * frac));
    }

    if (p_.lowHz != lastLow_ || p_.highHz != lastHigh_)
    {
        lastLow_ = p_.lowHz;
        lastHigh_ = p_.highHz;
        const float lo = std::max (20.0f, p_.lowHz);
        const float hi = std::max (lo * 1.05f, p_.highHz);
        for (int k = 0; k < kBins; ++k)
        {
            const float f = std::max (binFreq_[(size_t) k], 1.0f);
            const float wLow  = clampf (std::log2 (f / lo) + 0.5f, 0.0f, 1.0f);
            const float wHigh = clampf (std::log2 (hi / f) + 0.5f, 0.0f, 1.0f);
            rangeW_[(size_t) k] = wLow * wHigh;
        }
        rangeW_[0] = 0.0f;
    }
}

// ---------------------------------------------------------------------------
// Radix-2 FFT (in place, split real/imag). Inverse is unscaled.
void MaskEngine::fft (float* re, float* im, bool inverse) const
{
    for (int i = 0; i < kFftSize; ++i)
    {
        const int j = bitrev_[(size_t) i];
        if (j > i)
        {
            std::swap (re[i], re[j]);
            std::swap (im[i], im[j]);
        }
    }

    for (int len = 2; len <= kFftSize; len <<= 1)
    {
        const int half = len >> 1;
        const int step = kFftSize / len;
        for (int i = 0; i < kFftSize; i += len)
        {
            for (int j = 0; j < half; ++j)
            {
                const float wr = twRe_[(size_t) (j * step)];
                const float wi = inverse ? -twIm_[(size_t) (j * step)] : twIm_[(size_t) (j * step)];
                const int a = i + j, b = i + j + half;
                const float vr = re[b] * wr - im[b] * wi;
                const float vi = re[b] * wi + im[b] * wr;
                re[b] = re[a] - vr;  im[b] = im[a] - vi;
                re[a] += vr;         im[a] += vi;
            }
        }
    }
}

// ---------------------------------------------------------------------------
void MaskEngine::process (float* const* channels, int numChannels, int numSamples)
{
    const int nCh = std::min (numChannels, numCh_);
    constexpr int mask = kFftSize - 1;

    for (int i = 0; i < numSamples; ++i)
    {
        for (int c = 0; c < nCh; ++c)
        {
            auto& in  = inBuf_[(size_t) c];
            auto& out = outBuf_[(size_t) c];

            const float x   = channels[c][i];
            const float dry = in[(size_t) pos_];      // the sample from kLatency samples ago
            in[(size_t) pos_] = x;
            const float wet = out[(size_t) pos_];
            out[(size_t) pos_] = 0.0f;

            float y;
            if (p_.bypass)        y = dry;
            else if (p_.delta)    y = dry - wet;
            else                  y = dry + (wet - dry) * p_.mix;

            channels[c][i] = y;
        }

        pos_ = (pos_ + 1) & mask;
        if (++hopCount_ >= kHop)
        {
            hopCount_ = 0;
            processFrame();
        }
    }
}

// ---------------------------------------------------------------------------
void MaskEngine::processFrame()
{
    constexpr int mask = kFftSize - 1;
    const int nCh = numCh_;

    // 1. window + forward FFT
    for (int c = 0; c < nCh; ++c)
    {
        float* re = re_[(size_t) c].data();
        float* im = im_[(size_t) c].data();
        const float* in = inBuf_[(size_t) c].data();
        for (int i = 0; i < kFftSize; ++i)
        {
            re[i] = in[(pos_ + i) & mask] * win_[(size_t) i];
            im[i] = 0.0f;
        }
        fft (re, im, false);
    }

    // 2. linked magnitude in dBFS
    float energy = 0.0f;
    for (int k = 0; k < kBins; ++k)
    {
        float p = 0.0f;
        for (int c = 0; c < nCh; ++c)
        {
            const float r = re_[(size_t) c][(size_t) k], m = im_[(size_t) c][(size_t) k];
            p += r * r + m * m;
        }
        p /= (float) nCh;
        energy += p;
        const float amp = 2.0f * std::sqrt (p) / winSum_;
        magDb_[(size_t) k] = 20.0f * std::log10 (std::max (amp, 1.0e-7f));
    }

    // Detection works on a short time-average (in dB) so random noise fluctuations are not mistaken
    // for resonances; real resonances persist from frame to frame.
    for (int k = 0; k < kBins; ++k)
    {
        float& a = avgDb_[(size_t) k];
        a = (a < -119.0f) ? magDb_[(size_t) k] : (kAvgCoef * a + (1.0f - kAvgCoef) * magDb_[(size_t) k]);
    }

    // 3. smoothed envelope on a log-frequency scale (two passes so peaks do not lift it)
    auto envelopeOf = [this] (const float* x, float* env)
    {
        prefix_[0] = 0.0f;
        for (int k = 0; k < kBins; ++k) prefix_[(size_t) k + 1] = prefix_[(size_t) k] + x[k];
        for (int k = 0; k < kBins; ++k)
        {
            const int hw = halfWidth_[(size_t) k];
            const int lo = std::max (0, k - hw), hi = std::min (kBins - 1, k + hw);
            env[k] = (prefix_[(size_t) hi + 1] - prefix_[(size_t) lo]) / (float) (hi - lo + 1);
        }
    };
    envelopeOf (avgDb_.data(), env0_.data());
    for (int k = 0; k < kBins; ++k) clipped_[(size_t) k] = std::min (avgDb_[(size_t) k], env0_[(size_t) k] + kThresholdDb);
    envelopeOf (clipped_.data(), env1_.data());

    // 4. learn bookkeeping
    const bool wantLearn = learnRequested_.load();
    if (wantLearn && ! wasLearning_)
    {
        std::fill (learnSum_.begin(), learnSum_.end(), 0.0f);
        learnFrames_.store (0);
    }
    if (wantLearn)
    {
        for (int k = 0; k < kBins; ++k)
            learnSum_[(size_t) k] += std::max (0.0f, avgDb_[(size_t) k] - env1_[(size_t) k] - kThresholdDb);
        learnFrames_.fetch_add (1);
    }
    if (! wantLearn && wasLearning_)
        finaliseLearning();
    wasLearning_ = wantLearn;

    if (curveDirty_.exchange (false))
        rebuildLearnedBins();

    // 5. target reduction per bin
    const float depth = clampf (p_.depth, 0.0f, 1.0f);
    const bool useLive  = p_.mode == MaskParams::Live  || p_.mode == MaskParams::Both;
    const bool useLearn = p_.mode == MaskParams::Learn || p_.mode == MaskParams::Both;
    for (int k = 0; k < kBins; ++k)
    {
        float t = 0.0f;
        if (useLive)
        {
            const float excess = avgDb_[(size_t) k] - env1_[(size_t) k] - kThresholdDb;
            if (excess > 0.0f) t = -depth * std::min (kMaxReduceDb, excess);
        }
        if (useLearn)
        {
            const float l = -depth * std::min (kMaxReduceDb, learnedBins_[(size_t) k]);
            t = std::min (t, l);
        }
        target_[(size_t) k] = t * rangeW_[(size_t) k];
    }
    // smooth across frequency to avoid musical noise
    tmp_[0] = target_[0];
    tmp_[(size_t) kBins - 1] = target_[(size_t) kBins - 1];
    for (int k = 1; k < kBins - 1; ++k)
        tmp_[(size_t) k] = 0.25f * target_[(size_t) k - 1] + 0.5f * target_[(size_t) k] + 0.25f * target_[(size_t) k + 1];
    // keep the deeper of the smoothed/unsmoothed value so narrow peaks are not under-cut
    for (int k = 0; k < kBins; ++k) target_[(size_t) k] = std::min (tmp_[(size_t) k], 0.85f * target_[(size_t) k]);

    // 6. transient protection
    const float ratioDb = 10.0f * std::log10 ((energy + 1.0e-12f) / (eAvg_ + 1.0e-12f));
    const float tNow = (eAvg_ > 0.0f) ? clampf ((ratioDb - 2.5f) / 8.0f, 0.0f, 1.0f) : 0.0f;
    eAvg_ = (eAvg_ == 0.0f) ? energy : (0.93f * eAvg_ + 0.07f * energy);
    tSm_ = std::max (tNow, tSm_ * 0.7f);
    const float protect = 1.0f - clampf (p_.transient, 0.0f, 1.0f) * tSm_;

    // 7. attack / release smoothing per bin
    const float dt = (float) kHop / (float) sampleRate_;
    const float coefA = std::exp (-dt / std::max (0.0005f, p_.attackMs * 0.001f));
    const float coefR = std::exp (-dt / std::max (0.0005f, p_.releaseMs * 0.001f));
    float peakRed = 0.0f;
    for (int k = 0; k < kBins; ++k)
    {
        float& s = state_[(size_t) k];
        const float t = target_[(size_t) k];
        const float c = (t < s) ? coefA : coefR;
        s = c * s + (1.0f - c) * t;
        applied_[(size_t) k] = s * protect;
        peakRed = std::min (peakRed, applied_[(size_t) k]);
    }
    grDb_.store (peakRed, std::memory_order_relaxed);

    // 8. apply gains (mirrored bins keep the spectrum conjugate-symmetric)
    float lin[kBins];
    for (int k = 0; k < kBins; ++k) lin[k] = dbToLin (applied_[(size_t) k]);
    for (int c = 0; c < nCh; ++c)
    {
        float* re = re_[(size_t) c].data();
        float* im = im_[(size_t) c].data();
        for (int k = 0; k < kBins; ++k)
        {
            re[k] *= lin[k];  im[k] *= lin[k];
            if (k > 0 && k < kBins - 1)
            {
                re[kFftSize - k] *= lin[k];
                im[kFftSize - k] *= lin[k];
            }
        }
        fft (re, im, true);

        // 9. synthesis window + overlap-add (1/N for the inverse FFT, 1/2 for the 75% overlap sum)
        float* out = outBuf_[(size_t) c].data();
        const float norm = 0.5f / (float) kFftSize;
        for (int i = 0; i < kFftSize; ++i)
            out[(pos_ + i) & mask] += re[i] * win_[(size_t) i] * norm;
    }

    publishDisplay();
}

// ---------------------------------------------------------------------------
void MaskEngine::publishDisplay()
{
    for (int g = 0; g < kGrid; ++g)
    {
        const int lo = gridLo_[(size_t) g], hi = gridHi_[(size_t) g];
        float in = -200.0f, env = -200.0f, out = -200.0f, red = 0.0f;
        for (int k = lo; k <= hi; ++k)
        {
            in  = std::max (in,  magDb_[(size_t) k]);
            env = std::max (env, env1_[(size_t) k]);
            out = std::max (out, magDb_[(size_t) k] + applied_[(size_t) k]);
            red = std::min (red, applied_[(size_t) k]);
        }
        dispIn_[(size_t) g].store (in, std::memory_order_relaxed);
        dispEnv_[(size_t) g].store (env, std::memory_order_relaxed);
        dispOut_[(size_t) g].store (out, std::memory_order_relaxed);
        dispRed_[(size_t) g].store (red, std::memory_order_relaxed);
    }
}

void MaskEngine::getDisplay (float* inDb, float* outDb, float* envDb, float* redDb) const
{
    for (int g = 0; g < kGrid; ++g)
    {
        if (inDb)  inDb[g]  = dispIn_[(size_t) g].load (std::memory_order_relaxed);
        if (outDb) outDb[g] = dispOut_[(size_t) g].load (std::memory_order_relaxed);
        if (envDb) envDb[g] = dispEnv_[(size_t) g].load (std::memory_order_relaxed);
        if (redDb) redDb[g] = dispRed_[(size_t) g].load (std::memory_order_relaxed);
    }
}

// ---------------------------------------------------------------------------
void MaskEngine::finaliseLearning()
{
    const int frames = std::max (1, learnFrames_.load());
    int resonances = 0;
    bool inPeak = false;
    for (int g = 0; g < kGrid; ++g)
    {
        float m = 0.0f;
        for (int k = gridLo_[(size_t) g]; k <= gridHi_[(size_t) g]; ++k)
            m = std::max (m, learnSum_[(size_t) k] / (float) frames);
        if (m < 0.75f) m = 0.0f;                       // ignore weak, occasional excess
        curve_[(size_t) g].store (std::min (m, kMaxReduceDb), std::memory_order_relaxed);
        if (m > 0.0f && ! inPeak) ++resonances;
        inPeak = m > 0.0f;
    }
    learnedResonances_.store (resonances);
    learnedValid_.store (learnFrames_.load() > 0);
    curveDirty_.store (true);
}

void MaskEngine::rebuildLearnedBins()
{
    if (! learnedValid_.load())
    {
        std::fill (learnedBins_.begin(), learnedBins_.end(), 0.0f);
        return;
    }
    const float ratio = std::log (kGridHiHz / kGridLoHz);
    for (int k = 0; k < kBins; ++k)
    {
        const float f = binFreq_[(size_t) k];
        if (f < kGridLoHz || f > kGridHiHz) { learnedBins_[(size_t) k] = 0.0f; continue; }
        const float pos = (float) (kGrid - 1) * std::log (f / kGridLoHz) / ratio;
        const int   i0  = std::min (kGrid - 2, (int) pos);
        const float fr  = pos - (float) i0;
        learnedBins_[(size_t) k] = curve_[(size_t) i0].load (std::memory_order_relaxed) * (1.0f - fr)
                                 + curve_[(size_t) i0 + 1].load (std::memory_order_relaxed) * fr;
    }
}

void MaskEngine::getLearnedCurve (float* dest) const
{
    for (int g = 0; g < kGrid; ++g) dest[g] = curve_[(size_t) g].load (std::memory_order_relaxed);
}

void MaskEngine::setLearnedCurve (const float* src, bool valid)
{
    int resonances = 0;
    bool inPeak = false;
    for (int g = 0; g < kGrid; ++g)
    {
        curve_[(size_t) g].store (src[g], std::memory_order_relaxed);
        if (src[g] > 0.0f && ! inPeak) ++resonances;
        inPeak = src[g] > 0.0f;
    }
    learnedResonances_.store (resonances);
    learnedValid_.store (valid);
    curveDirty_.store (true);
}

void MaskEngine::clearLearnedCurve()
{
    std::array<float, kGrid> zeros {};
    setLearnedCurve (zeros.data(), false);
}

} // namespace themask
