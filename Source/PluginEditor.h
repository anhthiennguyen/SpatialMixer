#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "CanvasComponent.h"
#include "SpectrumComponent.h"

class SpatialMixerEditor : public juce::AudioProcessorEditor
{
public:
    explicit SpatialMixerEditor (SpatialMixerProcessor&);
    ~SpatialMixerEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    SpatialMixerProcessor& proc_;
    CanvasComponent    canvas_;
    SpectrumComponent  spectrum_;

    // Track label editor: double-click a box to rename it
    std::unique_ptr<juce::TextEditor> labelEditor_;
    int editingTrack_ = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpatialMixerEditor)
};
