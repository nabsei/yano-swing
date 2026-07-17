#pragma once
#include "YanoSwingLookAndFeel.h"
#include "YanoSwingProcessor.h"

class YanoSwingEditor : public juce::AudioProcessorEditor
{
public:
    explicit YanoSwingEditor(YanoSwingProcessor&);
    ~YanoSwingEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    YanoSwingProcessor& processorRef;
    YanoSwingLookAndFeel lookAndFeel;

    juce::Slider amountSlider;
    juce::Label amountValueLabel;
    juce::Label titleLabel;
    juce::Label subtitleLabel;
    juce::Label footerLabel;
    juce::Label brandLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> amountAttachment;

    void updateValueLabel();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(YanoSwingEditor)
};
