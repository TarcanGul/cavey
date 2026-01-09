#pragma once

#include <JuceHeader.h>
#include <vector>
#include <iterator>
#include <utility>
#include "components/Parameter.h"

static constexpr const int MARGIN_EXTRA_SMALL = 10;
static constexpr const int MARGIN_SMALL = 20;
static constexpr const int PROMPT_WIDTH = 100;
static constexpr const int PROMPT_HEIGHT = 200;
static constexpr int INIT_SCREEN_WIDTH = 600;
static constexpr int INIT_SCREEN_HEIGHT = 600;
static constexpr int MAX_PARAMETER_AMOUNT = 3;

// Text
static constexpr const char *const MAIN_LABEL_TEXT = "Welcome to Cavey! Start with a prompt to generate a parameter.";
static constexpr const char *const PROMPT_PLACEHOLDER_TEXT = "Write your prompt here.";
static constexpr const char *const GENERATE_BUTTON_TEXT = "Generate";

static constexpr const long KNOB_INIT_Y_POS = 100L;

static constexpr const long KNOB_HEIGHT = 10;

class CaveyAudioProcessor;

class CaveyAudioProcessorEditor : public juce::AudioProcessorEditor, public juce::Button::Listener, private juce::Slider::Listener  {
public:
    explicit CaveyAudioProcessorEditor(CaveyAudioProcessor&);
    ~CaveyAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void buttonClicked(Button * buttonRef) override;
    void whenGenerateButtonClicked();
    void whenRemoveParameterButtonClicked(Parameter * parameterGroup);
    void renderParameterKnobs() const noexcept;

    void sliderValueChanged(juce::Slider * slider) override;

    std::optional<Parameter *> getParameterGroup(Button * buttonRef);
    CaveyAudioProcessor& audioProcessor;
    Label mainLabel;
    TextEditor promptEditor;
    TextButton generateButton;
    std::vector<Parameter *> parameterKnobs = {};

    inline void parameterAdded();
    inline void parameterRemoved();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CaveyAudioProcessorEditor)
};

