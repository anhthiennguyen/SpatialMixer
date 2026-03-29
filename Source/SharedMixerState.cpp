#include "SharedMixerState.h"
#include "PluginProcessor.h"

JUCE_IMPLEMENT_SINGLETON (SharedMixerState)

SharedMixerState::SharedMixerState() {}

//==============================================================================
int SharedMixerState::registerProcessor (SpatialMixerProcessor* proc,
                                          const TrackState& initialState)
{
    juce::ScopedLock lock(lock_);
    for (int i = 0; i < kMaxTracks; ++i)
    {
        if (!slots_[i].occupied)
        {
            slots_[i].processor = proc;
            slots_[i].state     = initialState;
            slots_[i].state.active = true;
            slots_[i].occupied  = true;
            return i;
        }
    }
    return -1;  // no free slot
}

void SharedMixerState::unregisterProcessor (int slot)
{
    if (slot < 0 || slot >= kMaxTracks) return;
    juce::ScopedLock lock(lock_);
    slots_[slot].processor       = nullptr;
    slots_[slot].state.active    = false;
    slots_[slot].occupied        = false;
}

//==============================================================================
std::array<TrackState, kMaxTracks> SharedMixerState::getAllStates() const
{
    juce::ScopedLock lock(lock_);
    std::array<TrackState, kMaxTracks> out;
    for (int i = 0; i < kMaxTracks; ++i)
        out[i] = slots_[i].state;
    return out;
}

//==============================================================================
void SharedMixerState::setPosition (int slot, float normX, float normY)
{
    if (slot < 0 || slot >= kMaxTracks) return;
    SpatialMixerProcessor* proc = nullptr;
    {
        juce::ScopedLock lock(lock_);
        if (!slots_[slot].occupied) return;
        slots_[slot].state.normX = normX;
        slots_[slot].state.normY = normY;
        proc = slots_[slot].processor;
    }
    if (proc) proc->setTrackPosition(normX, normY);
}

void SharedMixerState::setHeight (int slot, float normHeight)
{
    if (slot < 0 || slot >= kMaxTracks) return;
    SpatialMixerProcessor* proc = nullptr;
    {
        juce::ScopedLock lock(lock_);
        if (!slots_[slot].occupied) return;
        slots_[slot].state.normHeight = normHeight;
        proc = slots_[slot].processor;
    }
    if (proc) proc->setTrackHeight(normHeight);
}

void SharedMixerState::setLabel (int slot, const juce::String& label)
{
    if (slot < 0 || slot >= kMaxTracks) return;
    SpatialMixerProcessor* proc = nullptr;
    {
        juce::ScopedLock lock(lock_);
        if (!slots_[slot].occupied) return;
        slots_[slot].state.label = label;
        proc = slots_[slot].processor;
    }
    if (proc) proc->setTrackLabel(label);
}

void SharedMixerState::pushState (int slot, const TrackState& state)
{
    if (slot < 0 || slot >= kMaxTracks) return;
    juce::ScopedLock lock(lock_);
    if (!slots_[slot].occupied) return;
    auto proc = slots_[slot].state.active;  // preserve active flag
    slots_[slot].state        = state;
    slots_[slot].state.active = proc;
}

//==============================================================================
juce::Colour SharedMixerState::trackColour (int slot)
{
    static const juce::Colour colours[kMaxTracks] = {
        juce::Colour(0xffE53935),  // red
        juce::Colour(0xff1E88E5),  // blue
        juce::Colour(0xff43A047),  // green
        juce::Colour(0xffFB8C00),  // orange
        juce::Colour(0xff8E24AA),  // purple
        juce::Colour(0xff00ACC1),  // cyan
        juce::Colour(0xffF9A825),  // amber
        juce::Colour(0xffE64A19),  // deep orange
    };
    return colours[juce::jlimit(0, kMaxTracks - 1, slot)];
}
