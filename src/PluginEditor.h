#pragma once

#include <JuceHeader.h>

static constexpr const int MARGIN_EXTRA_SMALL = 10;
static constexpr const int MARGIN_SMALL = 20;
static constexpr const int PROMPT_WIDTH = 100;
static constexpr const int PROMPT_HEIGHT = 200;
static constexpr int INIT_SCREEN_WIDTH = 600;
static constexpr int INIT_SCREEN_HEIGHT = 600;

// Text
static constexpr const char *const MAIN_LABEL_TEXT = "Welcome to Cavey! Start with a prompt to generate a parameter.";
static constexpr const char *const PROMPT_PLACEHOLDER_TEXT = "Write your prompt here.";
static constexpr const char *const GENERATE_BUTTON_TEXT = "Generate";

class CaveyAudioProcessor;

class CaveyAudioProcessorEditor : public juce::AudioProcessorEditor {
public:
    explicit CaveyAudioProcessorEditor(CaveyAudioProcessor&);
    ~CaveyAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    CaveyAudioProcessor& audioProcessor;
    Label mainLabel;
    TextEditor promptEditor;
    TextButton generateButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CaveyAudioProcessorEditor)
};

