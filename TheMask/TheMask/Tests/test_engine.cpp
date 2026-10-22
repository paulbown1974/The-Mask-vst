// Stand-alone tests for the Mask DSP engine (no JUCE needed).
//   g++ -std=c++17 -O2 -I../Source test_engine.cpp ../Source/MaskEngine.cpp -o test_engine && ./test_engine
#include "MaskEngine.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <vector>

using themask::MaskEngine;
using themask::MaskParams;

static int failures = 0;
#define CHECK(cond, ...) do { if (! (cond)) { ++failures; std::printf ("  FAIL: "); std::printf (__VA_ARGS__); std::printf ("\n"); } else { std::printf ("  ok:   "); std::printf (__VA_ARGS__); std::printf ("\n"); } } while (0)

static const double kSr = 48000.0;
static const double kPi = 3.14159265358979323846;

// Power of x at frequency f using a Hann-windowed Goertzel-style single-bin DFT, in dB.
static double bandDb (const std::vector<float>& x, size_t start, size_t len, double f)
{
    double re = 0, im = 0, wsum = 0;
    for (size_t i = 0; i < len; ++i)
    {
        const double w = 0.5 * (1.0 - std::cos (2.0 * kPi * (double) i / (double) len));
        const double ph = 2.0 * kPi * f * (double) (start + i) / kSr;
        re += w * x[start + i] * std::cos (ph);
        im -= w * x[start + i] * std::sin (ph);
        wsum += w;
    }
    const double amp = 2.0 * std::sqrt (re * re + im * im) / wsum;
    return 20.0 * std::log10 (std::max (amp, 1e-9));
}

// Mean level change over several noise-only frequencies (a single-bin noise estimate is too jumpy).
static double meanChangeDb (const std::vector<float>& in, const std::vector<float>& out, size_t start, size_t len, size_t lat, const std::vector<double>& freqs)
{
    double acc = 0;
    for (double f : freqs) acc += bandDb (in, start - lat, len, f) - bandDb (out, start, len, f);
    return acc / (double) freqs.size();
}

static std::vector<float> makeSignal (size_t n, double toneHz, float toneAmp, unsigned seed)
{
    std::mt19937 rng (seed);
    std::normal_distribution<float> nd (0.0f, 0.05f);
    std::vector<float> x (n);
    float b0 = 0, b1 = 0, b2 = 0;                       // pinkish noise
    for (size_t i = 0; i < n; ++i)
    {
        const float w = nd (rng);
        b0 = 0.99765f * b0 + w * 0.0990460f;
        b1 = 0.96300f * b1 + w * 0.2965164f;
        b2 = 0.57000f * b2 + w * 1.0526913f;
        x[i] = (b0 + b1 + b2 + w * 0.1848f) * 0.5f;
        if (toneHz > 0) x[i] += toneAmp * (float) std::sin (2.0 * kPi * toneHz * (double) i / kSr);
    }
    return x;
}

static std::vector<float> run (MaskEngine& e, const std::vector<float>& in)
{
    std::vector<float> out = in;
    const size_t block = 480;
    for (size_t s = 0; s < out.size(); s += block)
    {
        float* ch = out.data() + s;
        const int n = (int) std::min (block, out.size() - s);
        float* chans[1] = { ch };
        e.process (chans, 1, n);
    }
    return out;
}

int main()
{
    const size_t N = (size_t) kSr * 6;

    std::printf ("1. Latency / transparency with depth = 0\n");
    {
        MaskEngine e; e.prepare (kSr, 1);
        MaskParams p; p.depth = 0.0f; e.setParams (p);
        auto in = makeSignal (N, 0.0, 0.0f, 1);
        auto out = run (e, in);
        double err = 0, ref = 0;
        for (size_t i = MaskEngine::kLatency + 4096; i < N; ++i)
        {
            const double d = (double) out[i] - (double) in[i - MaskEngine::kLatency];
            err += d * d; ref += (double) in[i - MaskEngine::kLatency] * in[i - MaskEngine::kLatency];
        }
        const double snr = 10.0 * std::log10 (ref / std::max (err, 1e-30));
        CHECK (snr > 60.0, "depth 0 reconstructs input delayed by %d samples (SNR %.1f dB)", MaskEngine::kLatency, snr);
    }

    std::printf ("2. Bypass is a latency-compensated passthrough\n");
    {
        MaskEngine e; e.prepare (kSr, 1);
        MaskParams p; p.bypass = true; e.setParams (p);
        auto in = makeSignal (N / 2, 2800.0, 0.2f, 2);
        auto out = run (e, in);
        double err = 0;
        for (size_t i = MaskEngine::kLatency; i < in.size(); ++i)
            err = std::max (err, (double) std::fabs (out[i] - in[i - MaskEngine::kLatency]));
        CHECK (err < 1e-6, "bypass output == input delayed (max error %.2e)", err);
    }

    std::printf ("3. Live mode masks a narrow resonance and leaves the rest alone\n");
    {
        MaskEngine e; e.prepare (kSr, 1);
        MaskParams p; p.mode = MaskParams::Live; p.depth = 1.0f; p.sharpness = 0.6f; p.transient = 0.0f;
        p.attackMs = 5.0f; p.releaseMs = 60.0f; e.setParams (p);
        auto in = makeSignal (N, 2800.0, 0.12f, 3);
        auto out = run (e, in);
        const size_t start = (size_t) kSr * 3, len = (size_t) kSr * 2;
        const double toneIn  = bandDb (in,  start - MaskEngine::kLatency, len, 2800.0);
        const double toneOut = bandDb (out, start, len, 2800.0);
        const double lowNoise  = meanChangeDb (in, out, start, len, MaskEngine::kLatency, { 1500, 1700, 1900, 2100, 2300 });
        const double highNoise = meanChangeDb (in, out, start, len, MaskEngine::kLatency, { 3600, 3900, 4200, 4500, 4800 });
        std::printf ("      tone %.1f -> %.1f dB | noise below changes %.2f dB | noise above changes %.2f dB | GR %.1f dB\n",
                     toneIn, toneOut, lowNoise, highNoise, e.getReductionDb());
        CHECK (toneIn - toneOut > 8.0, "2.8 kHz resonance reduced by %.1f dB (want > 8)", toneIn - toneOut);
        CHECK (std::fabs (lowNoise) < 1.5 && std::fabs (highNoise) < 1.5,
               "neighbouring noise changes by %.2f / %.2f dB on average (want < 1.5)", lowNoise, highNoise);
    }

    std::printf ("4. Frequency range protects bins outside Low..High\n");
    {
        MaskEngine e; e.prepare (kSr, 1);
        MaskParams p; p.mode = MaskParams::Live; p.depth = 1.0f; p.sharpness = 0.6f; p.transient = 0.0f;
        p.lowHz = 4000.0f; p.highHz = 12000.0f; e.setParams (p);       // tone at 2.8 kHz is out of range
        auto in = makeSignal (N, 2800.0, 0.12f, 3);
        auto out = run (e, in);
        const size_t start = (size_t) kSr * 3, len = (size_t) kSr * 2;
        const double d = bandDb (in, start - MaskEngine::kLatency, len, 2800.0) - bandDb (out, start, len, 2800.0);
        CHECK (std::fabs (d) < 1.0, "tone outside range untouched (change %.2f dB)", d);
    }

    std::printf ("5. Delta outputs what the mask removes\n");
    {
        MaskEngine e; e.prepare (kSr, 1);
        MaskParams p; p.mode = MaskParams::Live; p.depth = 1.0f; p.sharpness = 0.6f; p.transient = 0.0f; p.delta = true;
        e.setParams (p);
        auto in = makeSignal (N, 2800.0, 0.12f, 3);
        auto out = run (e, in);
        const size_t start = (size_t) kSr * 3, len = (size_t) kSr * 2;
        const double tone  = bandDb (out, start, len, 2800.0);
        const double other = bandDb (out, start, len, 2000.0);
        CHECK (tone - other > 10.0, "delta is dominated by the removed resonance (tone %.1f dB vs 2 kHz %.1f dB)", tone, other);
    }

    std::printf ("6. Learn captures a resonance, then masks it without live detection\n");
    {
        MaskEngine e; e.prepare (kSr, 1);
        MaskParams p; p.mode = MaskParams::Live; p.depth = 1.0f; p.sharpness = 0.6f; p.transient = 0.0f; p.depth = 0.0f;
        e.setParams (p);                                    // depth 0: live masking off while learning
        e.startLearning();
        auto learnSig = makeSignal ((size_t) kSr * 4, 2800.0, 0.12f, 4);
        run (e, learnSig);
        e.stopLearning();
        auto flush = makeSignal ((size_t) kSr / 2, 0.0, 0.0f, 5);
        run (e, flush);
        CHECK (e.hasLearnedCurve(), "a curve was learned (%d resonance(s) found over %d frames)", e.getLearnedResonances(), e.getLearnedFrames());
        CHECK (e.getLearnedResonances() >= 1 && e.getLearnedResonances() <= 4, "resonance count is sensible (%d)", e.getLearnedResonances());

        p.mode = MaskParams::Learn; p.depth = 1.0f; e.setParams (p);
        auto in = makeSignal (N, 2800.0, 0.12f, 6);
        auto out = run (e, in);
        const size_t start = (size_t) kSr * 3, len = (size_t) kSr * 2;
        const double d = bandDb (in, start - MaskEngine::kLatency, len, 2800.0) - bandDb (out, start, len, 2800.0);
        CHECK (d > 6.0, "learned curve reduces the resonance by %.1f dB (want > 6)", d);

        float curve[MaskEngine::kGrid];
        e.getLearnedCurve (curve);
        MaskEngine e2; e2.prepare (kSr, 1);
        e2.setLearnedCurve (curve, true);
        e2.setParams (p);
        auto out2 = run (e2, in);
        const double d2 = bandDb (in, start - MaskEngine::kLatency, len, 2800.0) - bandDb (out2, start, len, 2800.0);
        CHECK (d2 > 6.0, "restored curve (state reload) reduces the resonance by %.1f dB", d2);
    }

    std::printf ("7. Stereo, linked detection keeps channels matched\n");
    {
        MaskEngine e; e.prepare (kSr, 2);
        MaskParams p; p.mode = MaskParams::Live; p.depth = 1.0f; p.sharpness = 0.6f; p.transient = 0.0f; e.setParams (p);
        auto l = makeSignal (N / 2, 2800.0, 0.12f, 7);
        auto r = l;                                          // identical channels
        const size_t block = 512;
        for (size_t s = 0; s < l.size(); s += block)
        {
            float* ch[2] = { l.data() + s, r.data() + s };
            e.process (ch, 2, (int) std::min (block, l.size() - s));
        }
        double d = 0;
        for (size_t i = 0; i < l.size(); ++i) d = std::max (d, (double) std::fabs (l[i] - r[i]));
        CHECK (d < 1e-5, "identical channels stay identical (max diff %.2e)", d);
    }

    std::printf ("\n%s (%d failure%s)\n", failures == 0 ? "ALL PASSED" : "FAILED", failures, failures == 1 ? "" : "s");
    return failures == 0 ? 0 : 1;
}
