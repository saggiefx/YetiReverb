/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

namespace ParamIDs
{

    inline constexpr auto size{ "size" };
    inline constexpr auto damp{ "damp" };
    inline constexpr auto width{ "width" };
    inline constexpr auto mix{ "mix" };
    inline constexpr auto lowshelf{ "lowshelf" };
    inline constexpr auto highshelf{ "highshelf" };

} // namespace ParamIDs

static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    auto percentFormat = [](float value, int /*maximumStringLength*/)
    {
        value *= 100; // Multiply by 100
        return juce::String(value, 2) + " %"; // Format as a percentage with 2 decimal places
    };

    auto hzFormat = [](float value, int /*maximumStringLength*/)
    {
        return juce::String(value, 1) + " Hz"; // Format value as Hz
    };


    juce::NormalisableRange<float> range(0.0f, 1.0f, 0.01f, 1.0f);
    juce::NormalisableRange<float> lowshelfRange(20.0f, 1000.0f, 1.0f, 0.5f);
    juce::NormalisableRange<float> highshelfRange(1000.0f, 20000.0f, 1.0f, 0.5f);

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::size,
        "Size",
        range,
        0.5f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        percentFormat,
        nullptr
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::damp,
        "Damp",
        range,
        0.5f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        percentFormat,
        nullptr
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::width,
        "Width",
        range,
        0.5f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        percentFormat,
        nullptr
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::mix,
        "Mix",
        range,
        0.5f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        percentFormat,
        nullptr
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::lowshelf,
        "Lowshelf Freq",
        lowshelfRange,
        20.0f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        hzFormat,
        nullptr
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::highshelf,
        "Highshelf Freq",
        highshelfRange,
        20000.0f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        hzFormat,
        nullptr
    ));

    return layout;
}

//==============================================================================
/**
*/
class YetiReverbAudioProcessor  : public juce::AudioProcessor
                            #if JucePlugin_Enable_ARA
                             , public juce::AudioProcessorARAExtension
                            #endif
{
public:
    //==============================================================================
    YetiReverbAudioProcessor();
    ~YetiReverbAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

private:

    std::atomic<float>* sizeParam { nullptr };
    std::atomic<float>* dampParam { nullptr };
    std::atomic<float>* widthParam { nullptr };
    std::atomic<float>* mixParam   { nullptr };
    std::atomic<float>* lowShelfFreqParam {nullptr};
    std::atomic<float>* highShelfFreqParam{nullptr};

    void updateReverbParams();
    void updateFilterCoefficients();

    juce::dsp::IIR::Filter<float> leftLowShelfFilter;
    juce::dsp::IIR::Filter<float> rightLowShelfFilter;
    juce::dsp::IIR::Filter<float> leftHighShelfFilter;
    juce::dsp::IIR::Filter<float> rightHighShelfFilter;

    juce::dsp::Reverb::Parameters params;
    juce::dsp::Reverb reverb;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (YetiReverbAudioProcessor)
};
