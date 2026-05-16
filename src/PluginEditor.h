#pragma once

#include <JuceHeader.h>
#include <vector>
#include <iterator>
#include <utility>
#include "components/AiSetupComponent.h"
#include "components/CaveyLookAndFeel.h"
#include "components/Parameter.h"
#include "components/LoadingComponent.h"

namespace CaveyUI {
    static constexpr int MARGIN_EXTRA_SMALL = 10;
    static constexpr int MARGIN_SMALL = 20;
    static constexpr int PROMPT_WIDTH = 100;
    static constexpr int PROMPT_HEIGHT = 200;
    static constexpr int AI_SETUP_BUTTON_GAP = 6;
    static constexpr int AI_SETUP_BUTTON_HEIGHT = 28;
    static constexpr int SELECTED_MODEL_INDICATOR_HEIGHT = 44;
    static constexpr int MIN_GENERATE_BUTTON_WIDTH = PROMPT_WIDTH - 2 * MARGIN_EXTRA_SMALL;
    static constexpr int MIN_GENERATE_BUTTON_HEIGHT = AI_SETUP_BUTTON_HEIGHT;
    static constexpr int MIN_PROMPT_HEIGHT =
        2 * MARGIN_SMALL + AI_SETUP_BUTTON_HEIGHT + SELECTED_MODEL_INDICATOR_HEIGHT
        + AI_SETUP_BUTTON_GAP + MIN_GENERATE_BUTTON_HEIGHT;
    static constexpr int INIT_SCREEN_WIDTH = 600;
    static constexpr int INIT_SCREEN_HEIGHT = 400;
    static constexpr int MIN_PROMPT_EDITOR_WIDTH =
        (INIT_SCREEN_WIDTH - PROMPT_WIDTH - 2 * MARGIN_SMALL) / 2;
    static constexpr int MIN_SCREEN_WIDTH =
            MIN_PROMPT_EDITOR_WIDTH + PROMPT_WIDTH + 2 * MARGIN_SMALL;
    static constexpr int MIN_SCREEN_HEIGHT =
            (3 * INIT_SCREEN_HEIGHT) / 4;
    static constexpr int MAX_SCREEN_WIDTH = 32000;
    static constexpr int MAX_SCREEN_HEIGHT = 32000;
    static constexpr int MAX_PARAMETER_AMOUNT = 1;

    static constexpr size_t KNOB_INIT_Y_POS = 100L;
    static constexpr size_t KNOB_HEIGHT = 10;

    // Text
    static constexpr const char * MAIN_LABEL_TEXT = "Welcome to Cavey! Start with a prompt to generate a parameter.";
    static constexpr const char * PROMPT_PLACEHOLDER_TEXT = "Write your prompt here.";
    static constexpr const char * GENERATE_BUTTON_TEXT = "Generate";
    static constexpr const char * GENERATE_TOOLTIP_PARAMETER_EXISTS = "Only one parameter can be created.";
    static constexpr const char * GENERATE_TOOLTIP_EMPTY_PROMPT = "Enter a prompt to generate.";
    static constexpr const char * GENERATE_TOOLTIP_LOADING = "Generating parameter...";
    static constexpr const char * GENERATE_TOOLTIP_MODEL_REQUIRED = "Select an Ollama model in AI setup first.";
    static constexpr const char * AI_SETUP_BUTTON_TEXT = "Set up your AI";
    static constexpr const char * AI_PROVIDER_TEXT = "Ollama";
    static constexpr const char * NO_AI_MODEL_SELECTED_TEXT = "No model selected";
}

class CaveyAudioProcessor;

class CaveyAudioProcessorEditor : public juce::AudioProcessorEditor,
        public juce::Button::Listener,
        public juce::ActionListener,
        private juce::Slider::Listener
{
public:
    explicit CaveyAudioProcessorEditor(CaveyAudioProcessor&);
    ~CaveyAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void buttonClicked(Button * buttonRef) override;
    void openAiSetupDialog();
    void whenGenerateButtonClicked();
    void whenRemoveParameterButtonClicked(Parameter * parameterGroup);
    void addParameterControl(const juce::String& parameterName);
    void renderParameterKnobs() const noexcept;

    void sliderValueChanged(juce::Slider * slider) override;

    void actionListenerCallback(const juce::String& message) override;

    void setLoading(bool desiredLoadingState);
    void updateGenerateButtonEnabledState();
    void updateSelectedModelIndicator();

    std::optional<Parameter *> getParameterGroup(Button * buttonRef);
    CaveyAudioProcessor& audioProcessor;
    CaveyLookAndFeel lookAndFeel_;
    Label mainLabel;
    TextEditor promptEditor;
    TextButton aiSetupButton;
    Label selectedModelIndicator;
    TextButton generateButton;
    TooltipWindow tooltipWindow;

    // Keeping as a list for future extensibility
    std::vector<Parameter *> parameterKnobs = {};
    LoadingComponent loadingOverlay;
    bool isLoading = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CaveyAudioProcessorEditor)
};
