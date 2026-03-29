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
    SpatialMixerProcessor& proc_;

    enum class DragMode { None, Move, ResizeTop, ResizeBottom };
    DragMode dragMode_      = DragMode::None;
    int      draggedSlot_   = -1;
    bool     dragIsMirror_  = false;   // true when user grabbed the mirrored box
    int      hoveredSlot_   = -1;
    bool     hoverIsMirror_ = false;

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

    // Returns the rect for the primary (normX) box
    juce::Rectangle<float> boxRect       (const TrackState& s) const;
    // Returns the rect for the mirrored (1-normX) box
    juce::Rectangle<float> mirrorBoxRect (const TrackState& s) const;

    DragMode hitZone (juce::Rectangle<float> rect, juce::Point<float> pt) const;

    // Returns slot index or -1. Sets isMirrorOut to indicate which box was hit.
    int hitTest (const std::array<TrackState, kMaxTracks>& states,
                 juce::Point<float> pt, bool& isMirrorOut) const;

    void drawBox (juce::Graphics&, juce::Rectangle<float>,
                  const juce::String& label, juce::Colour col,
                  bool isDragged, bool isHovered, bool isOwned) const;

    void timerCallback() override { repaint(); }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CanvasComponent)
};
