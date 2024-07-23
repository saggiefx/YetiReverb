/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "RotaryImageDial.h"

//==============================================================================
/**
*/
class YetiReverbAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    YetiReverbAudioProcessorEditor (YetiReverbAudioProcessor&);
    ~YetiReverbAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    YetiReverbAudioProcessor& audioProcessor;

    ImageKnob mixKnob, dampKnob, sizeKnob, widthKnob, lowshelfKnob, highshelfKnob;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (YetiReverbAudioProcessorEditor)
};
