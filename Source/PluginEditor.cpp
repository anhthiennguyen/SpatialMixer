#include "PluginEditor.h"

static constexpr int kSpectrumHeight = 140;

SpatialMixerEditor::SpatialMixerEditor (SpatialMixerProcessor& p)
    : AudioProcessorEditor(&p), proc_(p), canvas_(p), spectrum_(p)
{
    addAndMakeVisible(canvas_);
    addAndMakeVisible(spectrum_);
    setSize(620, 480 + kSpectrumHeight);
    setResizable(true, true);
    setResizeLimits(480, 360 + kSpectrumHeight, 1280, 900 + kSpectrumHeight);
}

SpatialMixerEditor::~SpatialMixerEditor() {}

void SpatialMixerEditor::paint (juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1a1a2e));

    g.setColour(juce::Colour(0xff16213e));
    g.fillRect(0, 0, getWidth(), 32);

    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(juce::FontOptions().withHeight(15.0f).withStyle("Bold")));
    g.drawText("SPATIAL MIXER", 12, 0, 200, 32, juce::Justification::centredLeft);

    g.setColour(juce::Colours::white.withAlpha(0.3f));
    g.setFont(juce::Font(juce::FontOptions().withHeight(10.0f)));
    g.drawText("X = Pan   |   Y = Frequency Band   |   Height = Bandwidth",
               220, 0, getWidth() - 230, 32, juce::Justification::centredLeft);
}

void SpatialMixerEditor::resized()
{
    int specH  = kSpectrumHeight;
    int canvasH = getHeight() - 32 - specH;
    canvas_.setBounds  (0, 32,          getWidth(), canvasH);
    spectrum_.setBounds(0, 32 + canvasH, getWidth(), specH);
}
