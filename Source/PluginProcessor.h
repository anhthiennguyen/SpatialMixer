#pragma once
#include <JuceHeader.h>
#include "TrackState.h"
#include "DSPEngine.h"

class SpatialMixerProcessor : public juce::AudioProcessor
{
public:
    SpatialMixerProcessor();
    ~SpatialMixerProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "SpatialMixer"; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Default"; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // Called from UI thread (by SharedMixerState or canvas directly)
    void setTrackPosition (float normX, float normY);
    void setTrackHeight   (float normHeight);
    void setTrackLabel    (const juce::String& label);
    void setTrackPriority (int priority);
    void setTrackMode     (TrackState::Mode mode);

    // Called by SharedMixerState after overlap resolution —
    // bypasses recompute and pushes final corrected params straight to DSP engine.
    void applyResolvedDSP (const TrackDSPParams& params);

    const TrackState& getTrackState() const { return state_; }
    int getSlotIndex() const { return slotIndex_; }
    double getSampleRate() const { return dsp_.getSampleRate(); }
    SpectrumAnalyser& getSpectrumAnalyser() { return dsp_.getSpectrumAnalyser(); }
    StereoScope&      getStereoScope()      { return dsp_.getStereoScope(); }

private:
    TrackState state_;
    DSPEngine  dsp_;
    int        slotIndex_ = -1;  // index in SharedMixerState

    void pushParams();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpatialMixerProcessor)
};
