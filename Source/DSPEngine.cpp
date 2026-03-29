#include "DSPEngine.h"

DSPEngine::DSPEngine() {}

void DSPEngine::prepare (double sampleRate, int samplesPerBlock)
{
    sampleRate_   = sampleRate;
    maxBlockSize_ = samplesPerBlock;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate       = sampleRate;
    spec.maximumBlockSize = (juce::uint32) samplesPerBlock;
    spec.numChannels      = 1;

    filterL_.prepare(spec);
    filterR_.prepare(spec);
    filterL_.reset();
    filterR_.reset();

    panSmoother_.reset(sampleRate, 0.02);
    panSmoother_.setCurrentAndTargetValue(0.0f);
}

void DSPEngine::reset()
{
    filterL_.reset();
    filterR_.reset();
    lastCenterHz_ = -1.0f;
}

void DSPEngine::updateParams (const TrackDSPParams& params)
{
    juce::SpinLock::ScopedLockType lock(pendingLock_);
    pendingParams_ = params;
    paramsDirty_.store(true, std::memory_order_release);
}

void DSPEngine::initParams (const TrackDSPParams& params)
{
    activeParams_  = params;
    pendingParams_ = params;
    paramsDirty_.store(false, std::memory_order_release);
    rebuildFilter(params.eqCenterHz, params.eqBandwidth, params.eqGainDb);
    lastCenterHz_  = params.eqCenterHz;
    lastBandwidth_ = params.eqBandwidth;
    lastEqGainDb_  = params.eqGainDb;
    panSmoother_.setCurrentAndTargetValue(params.panNormalized);
}

void DSPEngine::syncParams()
{
    if (!paramsDirty_.load(std::memory_order_acquire)) return;

    {
        juce::SpinLock::ScopedTryLockType lock(pendingLock_);
        if (!lock.isLocked()) return;
        activeParams_ = pendingParams_;
        paramsDirty_.store(false, std::memory_order_release);
    }

    float center = juce::jlimit(20.0f, 20000.0f, activeParams_.eqCenterHz);
    if (std::abs(center             - lastCenterHz_)  > 0.5f  ||
        std::abs(activeParams_.eqBandwidth - lastBandwidth_) > 0.01f ||
        std::abs(activeParams_.eqGainDb    - lastEqGainDb_)  > 0.1f)
    {
        rebuildFilter(center, activeParams_.eqBandwidth, activeParams_.eqGainDb);
        lastCenterHz_  = center;
        lastBandwidth_ = activeParams_.eqBandwidth;
        lastEqGainDb_  = activeParams_.eqGainDb;
    }

    panSmoother_.setTargetValue(activeParams_.panNormalized);
}

void DSPEngine::processBlock (float* L, float* R, int numSamples)
{
    for (int n = 0; n < numSamples; ++n)
    {
        // Sum to mono, apply EQ
        float mono = (L[n] + R[n]) * 0.5f;
        mono = filterL_.processSample(mono);

        // Constant-power pan
        float pan   = panSmoother_.getNextValue();
        float angle = (pan + 1.0f) * 0.5f * juce::MathConstants<float>::halfPi;
        L[n] = mono * std::cos(angle);
        R[n] = mono * std::sin(angle);
    }
}

void DSPEngine::rebuildFilter (float centerHz, float bandwidthOct, float gainDb)
{
    float bw   = juce::jlimit(0.1f, 4.0f, bandwidthOct);
    float root = std::pow(2.0f, bw);
    float Q    = juce::jlimit(0.1f, 30.0f, std::sqrt(root) / (root - 1.0f));

    auto coeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(
        sampleRate_, (double)centerHz, (double)Q,
        (double)juce::Decibels::decibelsToGain(gainDb));

    *filterL_.coefficients = *coeffs;
    *filterR_.coefficients = *coeffs;
}
