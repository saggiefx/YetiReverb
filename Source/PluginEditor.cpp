/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
YetiReverbAudioProcessorEditor::YetiReverbAudioProcessorEditor (YetiReverbAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (644, 210);

    addAndMakeVisible(mixKnob);
    //addAndMakeVisible(dampKnob);
    //addAndMakeVisible(sizeKnob);
    //addAndMakeVisible(widthKnob);
    //addAndMakeVisible(lowshelfKnob);
    //addAndMakeVisible(highshelfKnob);

    mixKnob.setImage("xknob", 370, 370);

    mixKnob.setAudioParameter(audioProcessor.apvts, ParamIDs::mix);
    //sizeKnob.setAudioParameter(audioProcessor.apvts, ParamIDs::size);
    //dampKnob.setAudioParameter(audioProcessor.apvts, ParamIDs::damp);
    //widthKnob.setAudioParameter(audioProcessor.apvts, ParamIDs::width);
    //lowshelfKnob.setAudioParameter(audioProcessor.apvts, ParamIDs::lowshelf);
    //highshelfKnob.setAudioParameter(audioProcessor.apvts, ParamIDs::highshelf);
}

YetiReverbAudioProcessorEditor::~YetiReverbAudioProcessorEditor()
{
}

//==============================================================================
void YetiReverbAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.drawImage(ImageCache::getFromMemory(BinaryData::Base_png, BinaryData::Base_pngSize),getLocalBounds().toFloat());
}

void YetiReverbAudioProcessorEditor::resized()
{

    int knobSize = 71;


    // Set bounds for each knob
    sizeKnob.setBounds(16, 72, knobSize, knobSize);
    widthKnob.setBounds(sizeKnob.getRight() + 22, sizeKnob.getY(), knobSize, knobSize);
    dampKnob.setBounds(widthKnob.getRight() + 5, sizeKnob.getY(), knobSize, knobSize);
    lowshelfKnob.setBounds(dampKnob.getRight() + 24, 73, knobSize, knobSize);
    highshelfKnob.setBounds(lowshelfKnob.getRight() + 11, 73, knobSize, knobSize);
    mixKnob.setBounds(444, 12, 183, 183);
}
