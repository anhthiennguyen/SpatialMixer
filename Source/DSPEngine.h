#pragma once
#include <JuceHeader.h>
#include "TrackState.h"

// Single-track stereo DSP engine.
// UI thread writes params via updateParams().
// Audio thread reads via syncParams() + processBlock().
class DSPEngine
{
public:
    DSPEngine();

    void prepare (double sampleRate, int samplesPerBlock);
    void reset();

    // UI thread: stage new params
    void updateParams (const TrackDSPParams& params);

    // Audio thread: pull staged params if dirty (call at top of processBlock)
    void syncParams();

    // Audio thread: process one stereo block in-place.
    // Sums L+R to mono, applies EQ, then redistributes with pan law.
    void processBlock (float* L, float* R, int numSamples);

    // Safe to call before audio starts (no thread contention yet)
    void initParams (const TrackDSPParams& params);

private:
    double sampleRate_ = 44100.0;
    int    maxBlockSize_ = 0;

    // Staging (UI → audio thread)
    TrackDSPParams pendingParams_;
    TrackDSPParams activeParams_;
    std::atomic<bool> paramsDirty_ { false };
    juce::SpinLock pendingLock_;

    // Filters (stereo — one per channel)
    juce::dsp::IIR::Filter<float> filterL_, filterR_;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> panSmoother_;

    float lastCenterHz_   = -1.0f;
    float lastBandwidth_  = -1.0f;
    float lastEqGainDb_   = -1.0f;

    void rebuildFilter (float centerHz, float bandwidthOct, float gainDb);
};
