#pragma once

#include <JuceHeader.h>

class CaveyAudioProcessor;

class CaveyAudioProcessorEditor : public juce::AudioProcessorEditor {
public:
    explicit CaveyAudioProcessorEditor(CaveyAudioProcessor&);
    ~CaveyAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override {}

private:
    CaveyAudioProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CaveyAudioProcessorEditor)
};

