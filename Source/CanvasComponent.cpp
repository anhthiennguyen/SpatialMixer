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
    float cx = s.normX * (float)getWidth();
    float cy = s.normY * (float)getHeight();
    float h  = juce::jmax(s.normHeight * (float)getHeight(), (float)kMinBoxPx);
    return { cx - kBoxW * 0.5f, cy - h * 0.5f, (float)kBoxW, h };
}

juce::Rectangle<float> CanvasComponent::mirrorBoxRect (const TrackState& s) const
{
    float mirrorNX = 1.0f - s.normX;
    float cx = mirrorNX * (float)getWidth();
    float cy = s.normY  * (float)getHeight();
    float h  = juce::jmax(s.normHeight * (float)getHeight(), (float)kMinBoxPx);
    return { cx - kBoxW * 0.5f, cy - h * 0.5f, (float)kBoxW, h };
}

CanvasComponent::DragMode CanvasComponent::hitZone (juce::Rectangle<float> rect,
                                                     juce::Point<float> pt) const
{
    if (!rect.contains(pt))                      return DragMode::None;
    if (pt.y <= rect.getY()      + kHandleZone)  return DragMode::ResizeTop;
    if (pt.y >= rect.getBottom() - kHandleZone)  return DragMode::ResizeBottom;
    return DragMode::Move;
}

int CanvasComponent::hitTest (const std::array<TrackState, kMaxTracks>& states,
                               juce::Point<float> pt, bool& isMirrorOut) const
{
    // Test top-most first; check both real and mirror box for each slot
    for (int i = kMaxTracks - 1; i >= 0; --i)
    {
        if (!states[i].active) continue;

        // Skip mirror test when centred or in Pan mode (no mirror exists)
        bool centred = std::abs(states[i].normX - 0.5f) < 0.01f;
        bool isPan   = (states[i].mode == TrackState::Mode::Pan);

        if (!centred && !isPan && mirrorBoxRect(states[i]).contains(pt))
        {
            isMirrorOut = true;
            return i;
        }
        if (boxRect(states[i]).contains(pt))
        {
            isMirrorOut = false;
            return i;
        }
    }
    return -1;
}

//==============================================================================
void CanvasComponent::drawBox (juce::Graphics& g,
                                juce::Rectangle<float> rect,
                                const juce::String& label,
                                juce::Colour col,
                                bool isDragged, bool isHovered, bool isOwned,
                                int priority) const
{
    // Shadow
    g.setColour(juce::Colours::black.withAlpha(0.40f));
    g.fillRoundedRectangle(rect.translated(3, 3), 7.0f);

    // Fill
    float alpha = isOwned ? (isDragged ? 1.0f : 0.88f) : 0.72f;
    g.setColour(col.withAlpha(alpha));
    g.fillRoundedRectangle(rect, 7.0f);

    // Resize handle tints
    g.setColour(juce::Colours::white.withAlpha(0.15f));
    g.fillRoundedRectangle(rect.withHeight((float)kHandleZone), 7.0f);
    g.fillRoundedRectangle(rect.withY(rect.getBottom() - kHandleZone)
                               .withHeight((float)kHandleZone), 7.0f);

    // White highlight ring for owned track
    if (isOwned)
    {
        g.setColour(juce::Colours::white.withAlpha(0.45f));
        g.drawRoundedRectangle(rect.expanded(1.5f), 8.0f, 1.0f);
    }

    // Border
    g.setColour(isDragged ? juce::Colours::white
                          : (isHovered ? col.brighter(0.9f) : col.brighter(0.4f)));
    g.drawRoundedRectangle(rect, 7.0f, isDragged ? 2.5f : 1.5f);

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
        g.setColour(juce::Colours::white);
        g.drawText(label, textRect.toNearestInt(), juce::Justification::centred, true);
        g.restoreState();
    }

    // Priority badge — only shown when track has been yielded (priority > 0)
    if (priority > 0)
    {
        juce::Rectangle<float> badge (rect.getRight() - 14.0f, rect.getY() - 1.0f, 14.0f, 14.0f);
        g.setColour(juce::Colours::black.withAlpha(0.8f));
        g.fillEllipse(badge);
        // Colour shifts: priority 1 = amber, 2+ = red
        g.setColour(priority == 1 ? juce::Colour(0xffF9A825) : juce::Colour(0xffE53935));
        g.drawEllipse(badge, 1.0f);
        g.setFont(juce::Font(juce::FontOptions().withHeight(8.5f).withStyle("Bold")));
        g.setColour(juce::Colours::white);
        g.drawText(juce::String(priority), badge.toNearestInt(), juce::Justification::centred);
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

    auto states  = SharedMixerState::getInstance()->getAllStates();
    int  ownSlot = proc_.getSlotIndex();

    // Draw all boxes (real + mirror) — both fully solid and identical
    for (int i = 0; i < kMaxTracks; ++i)
    {
        if (!states[i].active) continue;

        bool isOwned   = (i == ownSlot);
        bool isDraggedReal   = (i == draggedSlot_ && !dragIsMirror_);
        bool isDraggedMirror = (i == draggedSlot_ &&  dragIsMirror_);
        bool isHoveredReal   = (i == hoveredSlot_ && !hoverIsMirror_);
        bool isHoveredMirror = (i == hoveredSlot_ &&  hoverIsMirror_);

        juce::Colour col = SharedMixerState::trackColour(i);

        int pri = states[i].priority;

        // Primary box
        drawBox(g, boxRect(states[i]), states[i].label, col,
                isDraggedReal, isHoveredReal, isOwned, pri);

        // Mirror box — skip if centred or track is in Pan mode (intentional one-side)
        bool centred = std::abs(states[i].normX - 0.5f) < 0.01f;
        bool isPan   = (states[i].mode == TrackState::Mode::Pan);
        if (!centred && !isPan)
            drawBox(g, mirrorBoxRect(states[i]), states[i].label, col,
                    isDraggedMirror, isHoveredMirror, isOwned, pri);
    }

    // Status bar
    g.setColour(juce::Colours::black.withAlpha(0.55f));
    g.fillRect(0, h - 18, w, 18);
    g.setColour(juce::Colours::white.withAlpha(0.45f));
    g.setFont(juce::Font(juce::FontOptions().withHeight(10.0f)));

    const auto& ts = proc_.getTrackState();
    bool isPanMode = (ts.mode == TrackState::Mode::Pan);
    juce::String modeStr = isPanMode ? "Pan" : "Stereo";
    float width = std::abs(ts.normX * 2.0f - 1.0f);
    juce::String widthStr = isPanMode
        ? (ts.normX < 0.5f ? "L " : "R ") + juce::String((int)(width * 100)) + "%"
        : juce::String((int)(width * 100)) + "% wide";
    juce::String bandName = kBands[juce::jlimit(0, kNumBands - 1,
        (int)std::round(ts.getFractionalBand()))].name;

    int ownSlotDisplay = ownSlot >= 0 ? ownSlot + 1 : 0;
    g.drawText("Slot " + juce::String(ownSlotDisplay) + ": "
               + ts.label
               + "   [" + modeStr + "]"
               + "   " + widthStr
               + "   Band: " + bandName
               + "   |  drag  |  edges: resize  |  dbl-click: rename  |  right-click: options",
               6, h - 17, w - 12, 16, juce::Justification::centredLeft, false);
}

void CanvasComponent::resized() {}

//==============================================================================
void CanvasComponent::mouseDown (const juce::MouseEvent& e)
{
    if (labelEditor_) { commitEdit(); return; }

    auto  states   = SharedMixerState::getInstance()->getAllStates();
    bool  isMirror = false;
    int   idx      = hitTest(states, e.position, isMirror);

    // Right-click → priority context menu
    if (e.mods.isRightButtonDown())
    {
        if (idx < 0) return;
        int currentPriority = states[idx].priority;

        bool isCurrentlyPan = (states[idx].mode == TrackState::Mode::Pan);

        juce::PopupMenu menu;
        menu.addSectionHeader(states[idx].label);
        menu.addItem(1, "Bring Forward",  currentPriority > 0);
        menu.addItem(2, "Send Back");
        if (currentPriority > 0)
            menu.addItem(3, "Reset Priority");
        menu.addSeparator();
        menu.addItem(4, isCurrentlyPan ? "Switch to Stereo Widening" : "Switch to Pan Mode");

        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this),
            [this, idx, currentPriority, isCurrentlyPan] (int result)
            {
                if (result == 1)
                    SharedMixerState::getInstance()->setPriority(idx, currentPriority - 1);
                else if (result == 2)
                    SharedMixerState::getInstance()->setPriority(idx, currentPriority + 1);
                else if (result == 3)
                    SharedMixerState::getInstance()->setPriority(idx, 0);
                else if (result == 4)
                    SharedMixerState::getInstance()->setMode(
                        idx, isCurrentlyPan ? TrackState::Mode::Stereo : TrackState::Mode::Pan);
            });
        return;
    }

    if (idx < 0) return;

    draggedSlot_  = idx;
    dragIsMirror_ = isMirror;
    dragStartMouse_ = e.position;

    // Use whichever box was grabbed to seed drag start coords
    auto r = isMirror ? mirrorBoxRect(states[idx]) : boxRect(states[idx]);
    dragMode_         = hitZone(r, e.position);
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

    // When dragging the mirror box, X movement is inverted so both boxes
    // track the mouse naturally from whichever side was grabbed.
    float xSign = dragIsMirror_ ? -1.0f : 1.0f;

    if (dragMode_ == DragMode::Move)
    {
        float rawCX = dragStartCentreX_ + delta.x * xSign;
        float nx = juce::jlimit(kBoxW * 0.5f / W, 1.0f - kBoxW * 0.5f / W, rawCX / W);
        // In Pan mode the single box can go anywhere; in Stereo mode keep primary
        // box on the left half so the mirror stays on the right half.
        bool isPan = (shared->getAllStates()[draggedSlot_].mode == TrackState::Mode::Pan);
        if (!dragIsMirror_ && !isPan)
            nx = juce::jlimit(0.0f, 0.5f, nx);
        float ny = juce::jlimit(0.0f, 1.0f, (dragStartCentreY_ + delta.y) / H);
        shared->setPosition(draggedSlot_, nx, ny);
    }
    else if (dragMode_ == DragMode::ResizeTop)
    {
        float newTop = juce::jlimit(0.0f, dragStartBottomY_ - kMinBoxPx,
                                    dragStartTopY_ + delta.y);
        float nx = shared->getAllStates()[draggedSlot_].normX;
        shared->setPosition(draggedSlot_, nx, (newTop + dragStartBottomY_) * 0.5f / H);
        shared->setHeight  (draggedSlot_, (dragStartBottomY_ - newTop) / H);
    }
    else if (dragMode_ == DragMode::ResizeBottom)
    {
        float newBot = juce::jlimit(dragStartTopY_ + kMinBoxPx, H,
                                    dragStartBottomY_ + delta.y);
        float nx = shared->getAllStates()[draggedSlot_].normX;
        shared->setPosition(draggedSlot_, nx, (dragStartTopY_ + newBot) * 0.5f / H);
        shared->setHeight  (draggedSlot_, (newBot - dragStartTopY_) / H);
    }
}

void CanvasComponent::mouseUp (const juce::MouseEvent&)
{
    draggedSlot_  = -1;
    dragIsMirror_ = false;
    dragMode_     = DragMode::None;
}

void CanvasComponent::mouseMove (const juce::MouseEvent& e)
{
    auto  states = SharedMixerState::getInstance()->getAllStates();
    bool  isMirror = false;
    int   slot  = hitTest(states, e.position, isMirror);

    bool changed = (slot != hoveredSlot_ || isMirror != hoverIsMirror_);
    hoveredSlot_   = slot;
    hoverIsMirror_ = isMirror;
    if (changed) repaint();

    if (slot >= 0)
    {
        auto r    = isMirror ? mirrorBoxRect(states[slot]) : boxRect(states[slot]);
        auto zone = hitZone(r, e.position);
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
    auto  states = SharedMixerState::getInstance()->getAllStates();
    bool  isMirror = false;
    int   idx = hitTest(states, e.position, isMirror);
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
