#pragma once

#include <JuceHeader.h>

class ImageKnobLook : public LookAndFeel_V4
{
private:
    String imgName;
    int frameWidth = 0, frameHeight = 0;
    Image dialImg;

public:
    ImageKnobLook() = default;

    void setImage(const String& assetName, int asset_w, int asset_h)
    {
        imgName = assetName;
        frameWidth = asset_w;
        frameHeight = asset_h;
    }

    void drawRotarySlider(Graphics& g, int x, int y, int width, int height,
        float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle,
        Slider& slider) override
    {
        int frameIndex = static_cast<int>(sliderPosProportional * 90) % 91;
        String assetFullName = imgName + "_" + String(frameIndex) + "_png";
        int size = 0;
        const char* data = BinaryData::getNamedResource(assetFullName.toUTF8(), size);

        if (data != nullptr) {
            dialImg = ImageCache::getFromMemory(data, size);
            g.drawImage(dialImg, x, y, width, height, 0, 0, frameWidth, frameHeight);
        }
    }
};


class ImageKnob : public juce::Slider
{
public:
    ImageKnob()
    {
        setLookAndFeel(&lookAndFeel);
        setSliderStyle(juce::Slider::RotaryVerticalDrag);
        setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    }

    ~ImageKnob()
    {
        setLookAndFeel(nullptr);
    }

    void mouseEnter(const MouseEvent& event) override
    {
        setMouseCursor(MouseCursor::DraggingHandCursor);
        Slider::mouseEnter(event);
    }

    void setAudioParameter(AudioProcessorValueTreeState& apvts, const String& parameterID)
    {
        sliderAttachment.reset();
        sliderAttachment = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(apvts, parameterID, *this);
    }

    void setImage(const juce::String& imageName, int frameWidth, int frameHeight)
    {
        lookAndFeel.setImage(imageName, frameWidth, frameHeight);
        repaint();
    }

private:
    ImageKnobLook lookAndFeel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sliderAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ImageKnob)
};
