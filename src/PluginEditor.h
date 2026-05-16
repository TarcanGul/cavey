#pragma once

#include <JuceHeader.h>
#include <vector>
#include <iterator>
#include <utility>
#include "components/Parameter.h"
#include "components/LoadingComponent.h"
#include "components/AiSetupComponent.h"

namespace CaveyUI {
    static constexpr int MARGIN_EXTRA_SMALL = 10;
    static constexpr int MARGIN_SMALL = 20;
    static constexpr int PROMPT_WIDTH = 130;
    static constexpr int PROMPT_HEIGHT = 200;
    static constexpr int INIT_SCREEN_WIDTH = 600;
    static constexpr int INIT_SCREEN_HEIGHT = 400;
    static constexpr int MAX_PARAMETER_AMOUNT = 1;

    static constexpr size_t KNOB_INIT_Y_POS = 100L;
    static constexpr size_t KNOB_HEIGHT = 10;

    // Text
    static constexpr const char * MAIN_LABEL_TEXT = "Welcome to Cavey! Start with a prompt to generate a parameter.";
    static constexpr const char * PROMPT_PLACEHOLDER_TEXT = "Write your prompt here.";
    static constexpr const char * GENERATE_BUTTON_TEXT = "Generate";
    static constexpr const char * SETUP_BUTTON_TEXT = "Set up your AI";
    static constexpr const char * GENERATE_TOOLTIP_PARAMETER_EXISTS = "Only one parameter can be created.";
    static constexpr const char * GENERATE_TOOLTIP_EMPTY_PROMPT = "Enter a prompt to generate.";
    static constexpr const char * GENERATE_TOOLTIP_LOADING = "Generating parameter...";
    static constexpr const char * GENERATE_TOOLTIP_SETUP_REQUIRED = "Set up your AI before generating.";
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
    void whenGenerateButtonClicked();
    void whenSetupButtonClicked();
    void whenRemoveParameterButtonClicked(Parameter * parameterGroup);
    void addParameterControl(const juce::String& parameterName);
    void renderParameterKnobs() const noexcept;
    void showErrorToast(const juce::String& message);

    void sliderValueChanged(juce::Slider * slider) override;

    void actionListenerCallback(const juce::String& message) override;

    void setLoading(bool desiredLoadingState);
    void updateGenerateButtonEnabledState();
    void updateMainProviderLabel();

    std::optional<Parameter *> getParameterGroup(Button * buttonRef);
    CaveyAudioProcessor& audioProcessor;
    Label mainLabel;
    TextEditor promptEditor;
    TextButton generateButton;
    TextButton setupButton;
    Label mainProviderLabel;
    TooltipWindow tooltipWindow;
    Label errorToast;

    // Keeping as a list for future extensibility
    std::vector<Parameter *> parameterKnobs = {};
    LoadingComponent loadingOverlay;
    bool isLoading = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CaveyAudioProcessorEditor)
};
