#pragma once
#include <JuceHeader.h>
#include "TrackState.h"

class SpatialMixerProcessor;

// Singleton that holds all track positions so every plugin instance
// can read and draw the full picture. Each instance gets a slot (0-7).
// All methods called from the UI (message) thread only.
class SharedMixerState : public juce::DeletedAtShutdown
{
public:
    JUCE_DECLARE_SINGLETON (SharedMixerState, false)

    // Each PluginProcessor calls this on construction; returns its slot index.
    // Returns -1 if all 8 slots are taken.
    int  registerProcessor   (SpatialMixerProcessor* proc, const TrackState& initialState);
    void unregisterProcessor (int slot);

    // Read snapshot of all states for canvas drawing (UI thread)
    std::array<TrackState, kMaxTracks> getAllStates() const;

    // Canvas calls these when the user drags any track.
    // Forwards the change to the owning processor (which pushes to its DSP engine)
    // and refreshes the cached state so all canvases repaint correctly.
    void setPosition (int slot, float normX, float normY);
    void setHeight   (int slot, float normHeight);
    void setLabel    (int slot, const juce::String& label);

    // Called by a processor to push its full state (e.g. after setStateInformation)
    void pushState (int slot, const TrackState& state);

    // Per-slot colours
    static juce::Colour trackColour (int slot);

private:
    SharedMixerState();

    struct SlotData
    {
        TrackState             state;
        SpatialMixerProcessor* processor = nullptr;
        bool                   occupied  = false;
    };

    std::array<SlotData, kMaxTracks> slots_;
    mutable juce::CriticalSection    lock_;
};
