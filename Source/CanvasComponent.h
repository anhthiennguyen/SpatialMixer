#pragma once
#include <JuceHeader.h>
#include "TrackState.h"

class SpatialMixerProcessor;

class CanvasComponent : public juce::Component,
                        private juce::Timer
{
public:
    explicit CanvasComponent (SpatialMixerProcessor& proc);
    ~CanvasComponent() override;

    void paint            (juce::Graphics&) override;
    void resized          () override;
    void mouseDown        (const juce::MouseEvent&) override;
    void mouseDrag        (const juce::MouseEvent&) override;
    void mouseUp          (const juce::MouseEvent&) override;
    void mouseMove        (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

private:
    SpatialMixerProcessor& proc_;  // the instance this window belongs to

    enum class DragMode { None, Move, ResizeTop, ResizeBottom };
    DragMode dragMode_    = DragMode::None;
    int      draggedSlot_ = -1;
    int      hoveredSlot_ = -1;

    float dragStartCentreX_ = 0.0f;
    float dragStartCentreY_ = 0.0f;
    float dragStartTopY_    = 0.0f;
    float dragStartBottomY_ = 0.0f;
    juce::Point<float> dragStartMouse_;

    std::unique_ptr<juce::TextEditor> labelEditor_;
    int editingSlot_ = -1;
    void startEditing (int slot);
    void commitEdit();

    static constexpr int kBoxW       = 58;
    static constexpr int kHandleZone = 10;
    static constexpr int kMinBoxPx   = 30;

    juce::Rectangle<float> boxRect  (const TrackState& s) const;
    DragMode                hitZone (const TrackState& s, juce::Point<float> pt) const;
    int                     hitTest (const std::array<TrackState, kMaxTracks>& states,
                                     juce::Point<float> pt) const;

    void drawBox (juce::Graphics&, juce::Rectangle<float>,
                  const juce::String& label, juce::Colour,
                  bool isDragged, bool isHovered, bool isMirror, bool isOwned) const;

    void timerCallback() override { repaint(); }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CanvasComponent)
};
