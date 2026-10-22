#include "Limiter.h"

#include <algorithm>
#include <cmath>

namespace themask
{

void Limiter::prepare (double sampleRate)
{
    sampleRate_ = sampleRate > 1000.0 ? sampleRate : 48000.0;
    reset();
}

void Limiter::reset()
{
    env_ = 1.0f;
    inPeak_.store (0.0f);
    outPeak_.store (0.0f);
    grDb_.store (0.0f);
}

void Limiter::process (float* const* channels, int numChannels, int numSamples)
{
    const float gain    = std::pow (10.0f, p_.gainDb * 0.05f);
    const float thresh  = std::pow (10.0f, p_.threshDb * 0.05f);
    const float ceiling = std::pow (10.0f, p_.ceilingDb * 0.05f);
    const float relCoef = std::exp (-1.0f / (std::max (1.0f, p_.releaseMs) * 0.001f * (float) sampleRate_));

    float inPk = 0.0f, outPk = 0.0f, minEnv = 1.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        float level = 0.0f;
        for (int c = 0; c < numChannels; ++c)
            level = std::max (level, std::fabs (channels[c][i]) * gain);
        inPk = std::max (inPk, level);

        float target = 1.0f;
        if (p_.on && level > thresh)
            target = thresh / level;

        // instant attack, smoothed release
        env_ = (target < env_) ? target : (env_ + (1.0f - relCoef) * (target - env_));
        minEnv = std::min (minEnv, env_);

        for (int c = 0; c < numChannels; ++c)
        {
            float y = channels[c][i] * gain * (p_.on ? env_ : 1.0f);
            if (p_.on) y = std::max (-ceiling, std::min (ceiling, y));
            channels[c][i] = y;
            outPk = std::max (outPk, std::fabs (y));
        }
    }

    inPeak_.store (std::max (inPeak_.load (std::memory_order_relaxed), inPk), std::memory_order_relaxed);
    outPeak_.store (std::max (outPeak_.load (std::memory_order_relaxed), outPk), std::memory_order_relaxed);
    grDb_.store (20.0f * std::log10 (std::max (minEnv, 1.0e-4f)), std::memory_order_relaxed);
}

} // namespace themask
