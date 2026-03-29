#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "SharedMixerState.h"

SpatialMixerProcessor::SpatialMixerProcessor()
    : AudioProcessor (BusesProperties()
                      .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
    state_.computeDSP();
    // Register with the shared singleton so all canvases can see this track
    slotIndex_ = SharedMixerState::getInstance()->registerProcessor(this, state_);
    // Default label shows the slot number until user renames it
    if (slotIndex_ >= 0)
        state_.label = "Track " + juce::String(slotIndex_ + 1);
}

SpatialMixerProcessor::~SpatialMixerProcessor()
{
    SharedMixerState::getInstance()->unregisterProcessor(slotIndex_);
}

//==============================================================================
void SpatialMixerProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    dsp_.prepare(sampleRate, samplesPerBlock);
    state_.computeDSP();
    dsp_.initParams(state_.dsp);
}

void SpatialMixerProcessor::releaseResources() { dsp_.reset(); }

void SpatialMixerProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                           juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    dsp_.syncParams();

    float* L = buffer.getWritePointer(0);
    float* R = buffer.getWritePointer(buffer.getNumChannels() > 1 ? 1 : 0);
    dsp_.processBlock(L, R, buffer.getNumSamples());
}

//==============================================================================
void SpatialMixerProcessor::setTrackPosition (float normX, float normY)
{
    state_.normX = normX;
    state_.normY = normY;
    // SharedMixerState::setPosition already updated the shared copy and will
    // call resolveAndPush → applyResolvedDSP. Just keep local state in sync.
}

void SpatialMixerProcessor::setTrackHeight (float normHeight)
{
    state_.normHeight = juce::jlimit(0.05f, 0.95f, normHeight);
    // Same — SharedMixerState::setHeight drives the DSP update.
}

void SpatialMixerProcessor::setTrackLabel (const juce::String& label)
{
    state_.label = label;
    SharedMixerState::getInstance()->pushState(slotIndex_, state_);
}

void SpatialMixerProcessor::setTrackPriority (int priority)
{
    state_.priority = juce::jmax(0, priority);
    SharedMixerState::getInstance()->setPriority(slotIndex_, state_.priority);
}

void SpatialMixerProcessor::setTrackMode (TrackState::Mode mode)
{
    state_.mode = mode;
    pushParams();
}

void SpatialMixerProcessor::pushParams()
{
    state_.computeDSP();
    // Don't push to DSP engine here — SharedMixerState::resolveAndPush()
    // will do it after applying cross-track overlap corrections.
    SharedMixerState::getInstance()->pushState(slotIndex_, state_);
}

void SpatialMixerProcessor::applyResolvedDSP (const TrackDSPParams& params)
{
    state_.dsp = params;
    dsp_.updateParams(params);
}

//==============================================================================
void SpatialMixerProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::XmlElement xml("SpatialMixerState");
    xml.setAttribute("x",        state_.normX);
    xml.setAttribute("y",        state_.normY);
    xml.setAttribute("height",   state_.normHeight);
    xml.setAttribute("label",    state_.label);
    xml.setAttribute("priority", state_.priority);
    xml.setAttribute("isPanMode", state_.mode == TrackState::Mode::Pan ? 1 : 0);
    copyXmlToBinary(xml, destData);
}

void SpatialMixerProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary(data, sizeInBytes);
    if (!xml || !xml->hasTagName("SpatialMixerState")) return;
    state_.normX      = (float) xml->getDoubleAttribute("x",        0.5);
    state_.normY      = (float) xml->getDoubleAttribute("y",        0.5);
    state_.normHeight = (float) xml->getDoubleAttribute("height",   0.25);
    state_.label      =         xml->getStringAttribute("label",    state_.label);
    state_.priority   =         xml->getIntAttribute   ("priority", 0);
    state_.mode       = xml->getIntAttribute("isPanMode", 0) ? TrackState::Mode::Pan
                                                             : TrackState::Mode::Stereo;
    pushParams();
}

//==============================================================================
juce::AudioProcessorEditor* SpatialMixerProcessor::createEditor()
{
    return new SpatialMixerEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SpatialMixerProcessor();
}
