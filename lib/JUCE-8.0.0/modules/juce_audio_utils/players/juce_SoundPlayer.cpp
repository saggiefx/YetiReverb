/*
  ==============================================================================

   This file is part of the JUCE framework.
   Copyright (c) Raw Material Software Limited

   JUCE is an open source framework subject to commercial or open source
   licensing.

   By downloading, installing, or using the JUCE framework, or combining the
   JUCE framework with any other source code, object code, content or any other
   copyrightable work, you agree to the terms of the JUCE End User Licence
   Agreement, and all incorporated terms including the JUCE Privacy Policy and
   the JUCE Website Terms of Service, as applicable, which will bind you. If you
   do not agree to the terms of these agreements, we will not license the JUCE
   framework to you, and you must discontinue the installation or download
   process and cease use of the JUCE framework.

   JUCE End User Licence Agreement: https://juce.com/legal/juce-8-licence/
   JUCE Privacy Policy: https://juce.com/juce-privacy-policy
   JUCE Website Terms of Service: https://juce.com/juce-website-terms-of-service/

   Or:

   You may also use this code under the terms of the AGPLv3:
   https://www.gnu.org/licenses/agpl-3.0.en.html

   THE JUCE FRAMEWORK IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL
   WARRANTIES, WHETHER EXPRESSED OR IMPLIED, INCLUDING WARRANTY OF
   MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE, ARE DISCLAIMED.

  ==============================================================================
*/

namespace juce
{

// This is an AudioTransportSource which will own it's assigned source
struct AudioSourceOwningTransportSource final : public AudioTransportSource
{
    AudioSourceOwningTransportSource (PositionableAudioSource* s, double sr)  : source (s)
    {
        AudioTransportSource::setSource (s, 0, nullptr, sr);
    }

    ~AudioSourceOwningTransportSource()
    {
        setSource (nullptr);
    }

private:
    std::unique_ptr<PositionableAudioSource> source;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioSourceOwningTransportSource)
};

//==============================================================================
// An AudioSourcePlayer which will remove itself from the AudioDeviceManager's
// callback list once it finishes playing its source
struct AutoRemovingTransportSource final : public AudioTransportSource,
                                           private Timer
{
    AutoRemovingTransportSource (MixerAudioSource& mixerToUse, AudioTransportSource* ts, bool ownSource,
                                 int samplesPerBlock, double requiredSampleRate)
        : mixer (mixerToUse), transportSource (ts, ownSource)
    {
        jassert (ts != nullptr);

        setSource (transportSource);

        prepareToPlay (samplesPerBlock, requiredSampleRate);
        start();

        mixer.addInputSource (this, true);
        startTimerHz (10);
    }

    ~AutoRemovingTransportSource() override
    {
        setSource (nullptr);
    }

    void timerCallback() override
    {
        if (! transportSource->isPlaying())
            mixer.removeInputSource (this);
    }

private:
    MixerAudioSource& mixer;
    OptionalScopedPointer<AudioTransportSource> transportSource;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AutoRemovingTransportSource)
};

// An AudioSource which simply outputs a buffer
class AudioBufferSource final : public PositionableAudioSource
{
public:
    AudioBufferSource (AudioBuffer<float>* audioBuffer, bool ownBuffer, bool playOnAllChannels)
        : buffer (audioBuffer, ownBuffer),
          playAcrossAllChannels (playOnAllChannels)
    {}

    //==============================================================================
    void setNextReadPosition (int64 newPosition) override
    {
        jassert (newPosition >= 0);

        if (looping)
            newPosition = newPosition % static_cast<int64> (buffer->getNumSamples());

        position = jmin (buffer->getNumSamples(), static_cast<int> (newPosition));
    }

    int64 getNextReadPosition() const override      { return static_cast<int64> (position); }
    int64 getTotalLength() const override           { return static_cast<int64> (buffer->getNumSamples()); }

    bool isLooping() const override                 { return looping; }
    void setLooping (bool shouldLoop) override      { looping = shouldLoop; }

    //==============================================================================
    void prepareToPlay (int, double) override {}
    void releaseResources() override {}

    void getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill) override
    {
        bufferToFill.clearActiveBufferRegion();

        const int bufferSize = buffer->getNumSamples();
        const int samplesNeeded = bufferToFill.numSamples;
        const int samplesToCopy = jmin (bufferSize - position, samplesNeeded);

        if (samplesToCopy > 0)
        {
            int maxInChannels = buffer->getNumChannels();
            int maxOutChannels = bufferToFill.buffer->getNumChannels();

            if (! playAcrossAllChannels)
                maxOutChannels = jmin (maxOutChannels, maxInChannels);

            for (int i = 0; i < maxOutChannels; ++i)
                bufferToFill.buffer->copyFrom (i, bufferToFill.startSample, *buffer,
                                               i % maxInChannels, position, samplesToCopy);
        }

        position += samplesNeeded;

        if (looping)
            position %= bufferSize;
    }

private:
    //==============================================================================
    OptionalScopedPointer<AudioBuffer<float>> buffer;
    int position = 0;
    bool looping = false, playAcrossAllChannels;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioBufferSource)
};

SoundPlayer::SoundPlayer()
    : sampleRate (44100.0), bufferSize (512)
{
    formatManager.registerBasicFormats();
    player.setSource (&mixer);
}

SoundPlayer::~SoundPlayer()
{
    mixer.removeAllInputs();
    player.setSource (nullptr);
}

void SoundPlayer::play (const File& file)
{
    if (file.existsAsFile())
        play (formatManager.createReaderFor (file), true);
}

void SoundPlayer::play (const void* resourceData, size_t resourceSize)
{
    if (resourceData != nullptr && resourceSize > 0)
    {
        auto mem = std::make_unique<MemoryInputStream> (resourceData, resourceSize, false);
        play (formatManager.createReaderFor (std::move (mem)), true);
    }
}

void SoundPlayer::play (AudioFormatReader* reader, bool deleteWhenFinished)
{
    if (reader != nullptr)
        play (new AudioFormatReaderSource (reader, deleteWhenFinished), true, reader->sampleRate);
}

void SoundPlayer::play (AudioBuffer<float>* buffer, bool deleteWhenFinished, bool playOnAllOutputChannels)
{
    if (buffer != nullptr)
        play (new AudioBufferSource (buffer, deleteWhenFinished, playOnAllOutputChannels), true);
}

void SoundPlayer::play (PositionableAudioSource* audioSource, bool deleteWhenFinished, double fileSampleRate)
{
    if (audioSource != nullptr)
    {
        AudioTransportSource* transport = dynamic_cast<AudioTransportSource*> (audioSource);

        if (transport == nullptr)
        {
            if (deleteWhenFinished)
            {
                transport = new AudioSourceOwningTransportSource (audioSource, fileSampleRate);
            }
            else
            {
                transport = new AudioTransportSource();
                transport->setSource (audioSource, 0, nullptr, fileSampleRate);
                deleteWhenFinished = true;
            }
        }

        transport->start();
        transport->prepareToPlay (bufferSize, sampleRate);

        new AutoRemovingTransportSource (mixer, transport, deleteWhenFinished, bufferSize, sampleRate);
    }
    else
    {
        if (deleteWhenFinished)
            delete audioSource;
    }
}

void SoundPlayer::playTestSound()
{
    auto soundLength = (int) sampleRate;
    double frequency = 440.0;
    float amplitude = 0.5f;

    auto phasePerSample = MathConstants<double>::twoPi / (sampleRate / frequency);

    auto* newSound = new AudioBuffer<float> (1, soundLength);

    for (int i = 0; i < soundLength; ++i)
        newSound->setSample (0, i, amplitude * (float) std::sin (i * phasePerSample));

    newSound->applyGainRamp (0, 0, soundLength / 10, 0.0f, 1.0f);
    newSound->applyGainRamp (0, soundLength - soundLength / 4, soundLength / 4, 1.0f, 0.0f);

    play (newSound, true, true);
}

//==============================================================================
void SoundPlayer::audioDeviceIOCallbackWithContext (const float* const* inputChannelData,
                                                    int numInputChannels,
                                                    float* const* outputChannelData,
                                                    int numOutputChannels,
                                                    int numSamples,
                                                    const AudioIODeviceCallbackContext& context)
{
    player.audioDeviceIOCallbackWithContext (inputChannelData, numInputChannels,
                                             outputChannelData, numOutputChannels,
                                             numSamples, context);
}

void SoundPlayer::audioDeviceAboutToStart (AudioIODevice* device)
{
    if (device != nullptr)
    {
        sampleRate = device->getCurrentSampleRate();
        bufferSize = device->getCurrentBufferSizeSamples();
    }

    player.audioDeviceAboutToStart (device);
}

void SoundPlayer::audioDeviceStopped()
{
    player.audioDeviceStopped();
}

void SoundPlayer::audioDeviceError (const String& errorMessage)
{
    player.audioDeviceError (errorMessage);
}

} // namespace juce
