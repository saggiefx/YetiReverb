#pragma once

#include <JuceHeader.h>

class ImageKnobLook : public LookAndFeel_V4
{
private:
    String imageName;
    int frameWidth = 0, frameHeight = 0;
    Image dialImg;

public:
    ImageKnobLook() = default;

    void setImage(const String& assetName, int asset_w, int asset_h)
    {
        imageName = assetName;
        frameWidth = asset_w;
        frameHeight = asset_h;
    }

    void drawRotarySlider(Graphics& g, int x, int y, int width, int height,
        float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle,
        Slider& slider) override
    {
        float centerX = x + width * 0.5f;
        float centerY = y + height * 0.5f;
        float radius = jmin(width, height) * 0.4f;

        float newWidth = radius * 2;
        float newHeight = radius * 2;
        float newX = centerX - radius;
        float newY = centerY - radius;

        ColourGradient backgroundGradient(Colours::grey, newX, newY, Colours::lightgrey, newX + newWidth, newY + newHeight, false);
        backgroundGradient.addColour(0.5, Colours::white.withAlpha(0.6f));
        g.setGradientFill(backgroundGradient);
        g.fillEllipse(newX, newY, newWidth, newHeight);

        ColourGradient highlightGradient(Colours::white.withAlpha(0.3f), newX, newY, Colours::transparentWhite, newX + newWidth, newY + newHeight, false);
        g.setGradientFill(highlightGradient);
        g.fillEllipse(newX, newY, newWidth, newHeight);

        g.setColour(Colours::darkgrey);
        g.drawEllipse(newX, newY, newWidth, newHeight, 1.0f);

        ColourGradient shadowGradient(Colours::black.withAlpha(0.1f), newX, newY, Colours::transparentBlack, newX + newWidth, newY + newHeight, false);
        g.setGradientFill(shadowGradient);
        g.fillEllipse(newX, newY, newWidth, newHeight);

        float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
        float indicatorLength = radius * 0.7f;
        float indicatorX = centerX + indicatorLength * std::cos(angle);
        float indicatorY = centerY + indicatorLength * std::sin(angle);

        g.setColour(Colours::black);
        g.drawLine(centerX, centerY, indicatorX, indicatorY, 2.0f);

        ColourGradient capGradient(Colours::darkgrey, centerX - 1, centerY - 1, Colours::lightgrey, centerX + 1, centerY + 1, true);
        g.setGradientFill(capGradient);
        g.fillEllipse(centerX - 5.0f, centerY - 5.0f, 10.0f, 10.0f);

        g.setColour(Colours::darkgrey);
        for (int i = 0; i < 10; ++i) {
            float tickAngle = rotaryStartAngle + i * (rotaryEndAngle - rotaryStartAngle) / 9.0f;
            float tickStartX = centerX + radius * 0.9f * std::cos(tickAngle);
            float tickStartY = centerY + radius * 0.9f * std::sin(tickAngle);
            float tickEndX = centerX + radius * 0.95f * std::cos(tickAngle);
            float tickEndY = centerY + radius * 0.95f * std::sin(tickAngle);
            g.drawLine(tickStartX, tickStartY, tickEndX, tickEndY, 1.0f);
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
