#include "CanvasComponent.h"
#include "PluginProcessor.h"
#include "SharedMixerState.h"

static juce::Colour bandColour (int band)
{
    static const juce::Colour colours[kNumBands] = {
        juce::Colour(0xff1a1040),
        juce::Colour(0xff0d2e50),
        juce::Colour(0xff0a3328),
        juce::Colour(0xff3d2800),
        juce::Colour(0xff3d1200),
    };
    return colours[juce::jlimit(0, kNumBands - 1, band)];
}

//==============================================================================
CanvasComponent::CanvasComponent (SpatialMixerProcessor& proc)
    : proc_(proc)
{
    setSize(640, 500);
    startTimerHz(60);
}

CanvasComponent::~CanvasComponent() { stopTimer(); }

//==============================================================================
juce::Rectangle<float> CanvasComponent::boxRect (const TrackState& s) const
{
    float cx = s.normX      * (float)getWidth();
    float cy = s.normY      * (float)getHeight();
    float h  = juce::jmax(s.normHeight * (float)getHeight(), (float)kMinBoxPx);
    return { cx - kBoxW * 0.5f, cy - h * 0.5f, (float)kBoxW, h };
}

CanvasComponent::DragMode CanvasComponent::hitZone (const TrackState& s,
                                                     juce::Point<float> pt) const
{
    auto r = boxRect(s);
    if (!r.contains(pt))                     return DragMode::None;
    if (pt.y <= r.getY()      + kHandleZone) return DragMode::ResizeTop;
    if (pt.y >= r.getBottom() - kHandleZone) return DragMode::ResizeBottom;
    return DragMode::Move;
}

int CanvasComponent::hitTest (const std::array<TrackState, kMaxTracks>& states,
                               juce::Point<float> pt) const
{
    // Test top-most (last drawn) first
    for (int i = kMaxTracks - 1; i >= 0; --i)
        if (states[i].active && boxRect(states[i]).contains(pt))
            return i;
    return -1;
}

//==============================================================================
void CanvasComponent::drawBox (juce::Graphics& g,
                                juce::Rectangle<float> rect,
                                const juce::String& label,
                                juce::Colour col,
                                bool isDragged, bool isHovered,
                                bool isMirror, bool isOwned) const
{
    if (isMirror)
    {
        g.setColour(col.withAlpha(0.20f));
        g.fillRoundedRectangle(rect, 7.0f);

        juce::Path border;
        border.addRoundedRectangle(rect, 7.0f);
        const float dash[] = { 5.0f, 4.0f };
        juce::Path dashed;
        juce::PathStrokeType(1.5f).createDashedStroke(dashed, border, dash, 2);
        g.setColour(col.withAlpha(0.45f));
        g.fillPath(dashed);
    }
    else
    {
        // Shadow
        g.setColour(juce::Colours::black.withAlpha(0.40f));
        g.fillRoundedRectangle(rect.translated(3, 3), 7.0f);

        // Fill — owned track slightly brighter
        float alpha = isOwned ? (isDragged ? 1.0f : 0.88f) : 0.68f;
        g.setColour(col.withAlpha(alpha));
        g.fillRoundedRectangle(rect, 7.0f);

        // Handle tints
        g.setColour(juce::Colours::white.withAlpha(0.15f));
        g.fillRoundedRectangle(rect.withHeight((float)kHandleZone), 7.0f);
        g.fillRoundedRectangle(rect.withY(rect.getBottom() - kHandleZone)
                                   .withHeight((float)kHandleZone), 7.0f);

        // Border — owned track gets a white highlight ring
        if (isOwned)
        {
            g.setColour(juce::Colours::white.withAlpha(0.5f));
            g.drawRoundedRectangle(rect.expanded(1.5f), 8.0f, 1.0f);
        }
        g.setColour(isDragged ? juce::Colours::white
                              : (isHovered ? col.brighter(0.9f) : col.brighter(0.4f)));
        g.drawRoundedRectangle(rect, 7.0f, isDragged ? 2.5f : 1.5f);
    }

    // Vertical label
    if (rect.getHeight() >= 28.0f)
    {
        g.saveState();
        g.addTransform(juce::AffineTransform::rotation(
            -juce::MathConstants<float>::halfPi,
            rect.getCentreX(), rect.getCentreY()));

        juce::Rectangle<float> textRect (
            rect.getCentreX() - rect.getHeight() * 0.5f,
            rect.getCentreY() - rect.getWidth()  * 0.5f,
            rect.getHeight(), rect.getWidth());

        float fontSize = juce::jlimit(10.0f, 15.0f, rect.getHeight() * 0.14f);
        g.setFont(juce::Font(juce::FontOptions().withHeight(fontSize).withStyle("Bold")));
        g.setColour(isMirror ? col.brighter(0.4f).withAlpha(0.55f) : juce::Colours::white);
        g.drawText(label, textRect.toNearestInt(), juce::Justification::centred, true);
        g.restoreState();
    }
}

//==============================================================================
void CanvasComponent::paint (juce::Graphics& g)
{
    const int w = getWidth();
    const int h = getHeight();

    // Band stripes
    for (int b = 0; b < kNumBands; ++b)
    {
        int   db = kNumBands - 1 - b;
        float y0 = (float)(b * h) / kNumBands;
        float y1 = (float)((b + 1) * h) / kNumBands;
        g.setColour(bandColour(db).withAlpha(0.88f));
        g.fillRect(juce::Rectangle<float>(0, y0, (float)w, y1 - y0));
        g.setColour(juce::Colours::white.withAlpha(0.38f));
        g.setFont(juce::Font(juce::FontOptions().withHeight(10.0f).withStyle("Italic")));
        g.drawText(kBands[db].name, w - 58, (int)y0, 56, (int)(y1 - y0),
                   juce::Justification::centredRight, false);
        if (b > 0)
        {
            g.setColour(juce::Colours::white.withAlpha(0.10f));
            g.drawHorizontalLine((int)y0, 0.0f, (float)w);
        }
    }

    // Centre line
    g.setColour(juce::Colours::white.withAlpha(0.22f));
    g.drawVerticalLine(w / 2, 0.0f, (float)(h - 18));

    // Zone labels
    g.setColour(juce::Colours::white.withAlpha(0.35f));
    g.setFont(juce::Font(juce::FontOptions().withHeight(11.0f).withStyle("Bold")));
    g.drawText("LEFT",  4,        4, w / 2 - 8, 14, juce::Justification::centredLeft);
    g.drawText("MONO",  w/2 - 24, 4, 48,        14, juce::Justification::centred);
    g.drawText("RIGHT", w/2 + 4,  4, w / 2 - 8, 14, juce::Justification::centredRight);

    // Fetch all states
    auto states  = SharedMixerState::getInstance()->getAllStates();
    int  ownSlot = proc_.getSlotIndex();

    // Draw mirror ghosts first (all tracks)
    for (int i = 0; i < kMaxTracks; ++i)
    {
        if (!states[i].active) continue;
        float mirrorNX = 1.0f - states[i].normX;
        if (std::abs(mirrorNX - states[i].normX) < 0.02f) continue;

        float mirrorCX = mirrorNX * (float)w;
        float boxH     = juce::jmax(states[i].normHeight * (float)h, (float)kMinBoxPx);
        float boxY     = states[i].normY * (float)h - boxH * 0.5f;
        juce::Rectangle<float> mr (mirrorCX - kBoxW * 0.5f, boxY, (float)kBoxW, boxH);
        drawBox(g, mr, states[i].label, SharedMixerState::trackColour(i),
                false, false, true, false);
    }

    // Draw real boxes (all tracks)
    for (int i = 0; i < kMaxTracks; ++i)
    {
        if (!states[i].active) continue;
        bool isOwned   = (i == ownSlot);
        bool isDragged = (i == draggedSlot_);
        bool isHovered = (i == hoveredSlot_);
        drawBox(g, boxRect(states[i]), states[i].label,
                SharedMixerState::trackColour(i),
                isDragged, isHovered, false, isOwned);
    }

    // Status bar
    g.setColour(juce::Colours::black.withAlpha(0.55f));
    g.fillRect(0, h - 18, w, 18);
    g.setColour(juce::Colours::white.withAlpha(0.45f));
    g.setFont(juce::Font(juce::FontOptions().withHeight(10.0f)));

    juce::String status = "This track: " + proc_.getTrackState().label;
    if (ownSlot >= 0)
        status += "  (slot " + juce::String(ownSlot + 1) + ")";
    status += "   |   drag any box  |  drag edges to resize  |  double-click to rename";
    g.drawText(status, 6, h - 17, w - 12, 16, juce::Justification::centredLeft, false);
}

void CanvasComponent::resized() {}

//==============================================================================
void CanvasComponent::mouseDown (const juce::MouseEvent& e)
{
    if (labelEditor_) { commitEdit(); return; }

    auto states = SharedMixerState::getInstance()->getAllStates();
    int  idx    = hitTest(states, e.position);
    if (idx < 0) return;

    draggedSlot_ = idx;
    dragMode_    = hitZone(states[idx], e.position);
    dragStartMouse_ = e.position;

    auto r = boxRect(states[idx]);
    dragStartCentreX_ = r.getCentreX();
    dragStartCentreY_ = r.getCentreY();
    dragStartTopY_    = r.getY();
    dragStartBottomY_ = r.getBottom();
}

void CanvasComponent::mouseDrag (const juce::MouseEvent& e)
{
    if (draggedSlot_ < 0 || dragMode_ == DragMode::None) return;

    auto* shared = SharedMixerState::getInstance();
    const float W = (float)getWidth();
    const float H = (float)getHeight();
    auto delta = e.position - dragStartMouse_;

    if (dragMode_ == DragMode::Move)
    {
        float nx = juce::jlimit(kBoxW * 0.5f / W, 1.0f - kBoxW * 0.5f / W,
                                (dragStartCentreX_ + delta.x) / W);
        float ny = juce::jlimit(0.0f, 1.0f, (dragStartCentreY_ + delta.y) / H);
        shared->setPosition(draggedSlot_, nx, ny);
    }
    else if (dragMode_ == DragMode::ResizeTop)
    {
        float newTop = juce::jlimit(0.0f, dragStartBottomY_ - kMinBoxPx,
                                    dragStartTopY_ + delta.y);
        shared->setPosition(draggedSlot_,
            SharedMixerState::getInstance()->getAllStates()[draggedSlot_].normX,
            (newTop + dragStartBottomY_) * 0.5f / H);
        shared->setHeight(draggedSlot_, (dragStartBottomY_ - newTop) / H);
    }
    else if (dragMode_ == DragMode::ResizeBottom)
    {
        float newBot = juce::jlimit(dragStartTopY_ + kMinBoxPx, H,
                                    dragStartBottomY_ + delta.y);
        shared->setPosition(draggedSlot_,
            SharedMixerState::getInstance()->getAllStates()[draggedSlot_].normX,
            (dragStartTopY_ + newBot) * 0.5f / H);
        shared->setHeight(draggedSlot_, (newBot - dragStartTopY_) / H);
    }
}

void CanvasComponent::mouseUp (const juce::MouseEvent&)
{
    draggedSlot_ = -1;
    dragMode_    = DragMode::None;
}

void CanvasComponent::mouseMove (const juce::MouseEvent& e)
{
    auto states = SharedMixerState::getInstance()->getAllStates();
    int  prev   = hoveredSlot_;
    hoveredSlot_ = hitTest(states, e.position);
    if (hoveredSlot_ != prev) repaint();

    if (hoveredSlot_ >= 0)
    {
        auto zone = hitZone(states[hoveredSlot_], e.position);
        if (zone == DragMode::ResizeTop || zone == DragMode::ResizeBottom)
            setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
        else
            setMouseCursor(juce::MouseCursor::DraggingHandCursor);
    }
    else
    {
        setMouseCursor(juce::MouseCursor::NormalCursor);
    }
}

void CanvasComponent::mouseDoubleClick (const juce::MouseEvent& e)
{
    auto states = SharedMixerState::getInstance()->getAllStates();
    int  idx    = hitTest(states, e.position);
    if (idx >= 0) startEditing(idx);
}

//==============================================================================
void CanvasComponent::startEditing (int slot)
{
    commitEdit();
    editingSlot_ = slot;

    auto states = SharedMixerState::getInstance()->getAllStates();
    auto rect   = boxRect(states[slot]).toNearestInt();
    int  edW    = juce::jmax(130, rect.getWidth() + 50);
    int  edH    = 28;

    labelEditor_ = std::make_unique<juce::TextEditor>();
    labelEditor_->setBounds(rect.getCentreX() - edW / 2, rect.getCentreY() - edH / 2, edW, edH);
    labelEditor_->setText(states[slot].label, false);
    labelEditor_->selectAll();
    labelEditor_->setFont(juce::Font(juce::FontOptions().withHeight(14.0f)));
    labelEditor_->setColour(juce::TextEditor::backgroundColourId, juce::Colours::black.withAlpha(0.9f));
    labelEditor_->setColour(juce::TextEditor::textColourId,       juce::Colours::white);
    labelEditor_->setColour(juce::TextEditor::outlineColourId,    SharedMixerState::trackColour(slot));
    labelEditor_->onReturnKey = [this] { commitEdit(); };
    labelEditor_->onEscapeKey = [this] { labelEditor_.reset(); editingSlot_ = -1; };
    labelEditor_->onFocusLost = [this] { commitEdit(); };

    addAndMakeVisible(*labelEditor_);
    labelEditor_->grabKeyboardFocus();
}

void CanvasComponent::commitEdit()
{
    if (!labelEditor_ || editingSlot_ < 0) return;
    juce::String txt = labelEditor_->getText().trim();
    if (txt.isNotEmpty())
        SharedMixerState::getInstance()->setLabel(editingSlot_, txt);
    labelEditor_.reset();
    editingSlot_ = -1;
    repaint();
}
