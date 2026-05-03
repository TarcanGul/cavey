#include "PluginEditor.h"
#include "PluginProcessor.h"
class CaveyAudioProcessor; // forward-declare to match include order

CaveyAudioProcessorEditor::CaveyAudioProcessorEditor(CaveyAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    juce::Logger::writeToLog("Editor is starting...");

    setSize(CaveyUI::INIT_SCREEN_WIDTH, CaveyUI::INIT_SCREEN_HEIGHT);
    setResizable(true, true);

    // Create the parameter area that first be a text box.
    mainLabel.setText(CaveyUI::MAIN_LABEL_TEXT, NotificationType::dontSendNotification);
    mainLabel.setEditable(false);

    promptEditor.setTextToShowWhenEmpty(CaveyUI::PROMPT_PLACEHOLDER_TEXT, Colours::grey.withAlpha(.6f));
    promptEditor.addListener(this);

    generateButton.setButtonText(CaveyUI::GENERATE_BUTTON_TEXT);
    generateButton.addListener(this);
    updateGenerateButtonEnabledState();

    addAndMakeVisible(&mainLabel);
    addAndMakeVisible(&promptEditor);
    addAndMakeVisible(&generateButton);
    addChildComponent(&loadingOverlay);
    loadingOverlay.setVisible(false);

    audioProcessor.addActionListener(this);

    juce::Logger::writeToLog("Editor has been initialized.");
}

CaveyAudioProcessorEditor::~CaveyAudioProcessorEditor() {
    generateButton.removeListener(this);
    promptEditor.removeListener(this);
    for (auto parameter : parameterKnobs) {
        delete parameter;
    }
    audioProcessor.removeActionListener(this);
}

void CaveyAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void CaveyAudioProcessorEditor::resized() {
    auto screen = getLocalBounds();
    auto promptBounds = screen.removeFromBottom(CaveyUI::PROMPT_HEIGHT);
    auto buttonBounds = promptBounds.removeFromRight(CaveyUI::PROMPT_WIDTH);

    if (parameterKnobs.empty()) {
        mainLabel.setVisible(true);
        mainLabel.setBounds(screen.withSizeKeepingCentre(juce::GlyphArrangement::getStringWidthInt(mainLabel.getFont(), CaveyUI::MAIN_LABEL_TEXT), 20));
    } else {
        renderParameterKnobs();
    }

    // Divide the screen to four areas (header - main area - text area - footer)
    promptEditor.setBounds(promptBounds.reduced(CaveyUI::MARGIN_SMALL));
    generateButton.setBounds(buttonBounds.reduced(CaveyUI::MARGIN_EXTRA_SMALL, CaveyUI::MARGIN_SMALL));
    loadingOverlay.setBounds(screen);
}

void CaveyAudioProcessorEditor::buttonClicked(juce::Button *buttonRef) {
    juce::Logger::writeToLog("Button is clicked");
    if (buttonRef == &generateButton) {
        whenGenerateButtonClicked();
        return;
    }

    std::optional<Parameter *> maybeParameterTuple = getParameterGroup(buttonRef);
    if (maybeParameterTuple.has_value()) {
        whenRemoveParameterButtonClicked(maybeParameterTuple.value());
    }
}

void CaveyAudioProcessorEditor::whenGenerateButtonClicked() {
    if (parameterKnobs.size() >= CaveyUI::MAX_PARAMETER_AMOUNT) {
        // Error toast maybe
        return;
    }

    setLoading(true);
    updateGenerateButtonEnabledState();

    const juce::String prompt = promptEditor.getText();

    std::thread([this, p = prompt] {
        audioProcessor.addCaveyParameter(p);
        juce::Logger::writeToLog("Prompt sent as action message");
    }).detach();
}

void CaveyAudioProcessorEditor::sliderValueChanged(juce::Slider *slider) {
    audioProcessor.setBackendParameterValue(slider->getName(), static_cast<float>(slider->getValue()));
}

void CaveyAudioProcessorEditor::textEditorTextChanged(TextEditor& editor) {
    if (&editor != &promptEditor) {
        return;
    }

    updateGenerateButtonEnabledState();
}

void CaveyAudioProcessorEditor::whenRemoveParameterButtonClicked(Parameter * parameterGroup) {
    Button& removeButton = parameterGroup->getRemoveButton();
    const auto newListHead = std::remove_if(
            parameterKnobs.begin(),
            parameterKnobs.end(),
            [&](Parameter * parameterKnob) {
                return parameterKnob == parameterGroup;
            }
        );
    parameterKnobs.erase(newListHead, parameterKnobs.end());
    removeButton.removeListener(this);
    delete parameterGroup;
    parameterRemoved();
}

inline void CaveyAudioProcessorEditor::parameterAdded() {
    juce::Logger::writeToLog("Parameter added");
    renderParameterKnobs();
    mainLabel.setVisible(false);
    updateGenerateButtonEnabledState();
}

inline void CaveyAudioProcessorEditor::parameterRemoved() {
    renderParameterKnobs();
    if (parameterKnobs.empty()) {
        mainLabel.setVisible(true);
    }
    updateGenerateButtonEnabledState();
}

void CaveyAudioProcessorEditor::renderParameterKnobs() const noexcept {
    auto screen = getLocalBounds();
    for (size_t i = 0; i < parameterKnobs.size(); ++i) {
        auto parameter = parameterKnobs[i];
        parameter->setBounds(screen.removeFromTop(CaveyUI::KNOB_INIT_Y_POS + CaveyUI::KNOB_HEIGHT * i));
    }
}

void CaveyAudioProcessorEditor::actionListenerCallback(const juce::String &message) {
    if (message.isEmpty()) {
        juce::Logger::writeToLog("Empty message received, noop");
        return;
    }

    // Right now it is hardcoded, only processor will send parameter name, can be changed later
    const juce::String parameterName {message};

    auto * parameter = new Parameter(parameterName, audioProcessor.getValueTree());
    parameter->setLabel(parameterName);
    parameter->setRemoveButtonListener(this);
    parameter->getSlider().addListener(this);

    juce::Logger::writeToLog("Parameter is added via the callback");

    juce::MessageManager::callAsync([this, parameter] {
        setLoading(false);
        addAndMakeVisible(parameter);
        parameterKnobs.emplace_back(parameter);
        parameterAdded();
    });
}

std::optional<Parameter *> CaveyAudioProcessorEditor::getParameterGroup(Button* buttonRef) {
    for (auto parameter : parameterKnobs) {
        Button& removeButton = parameter->getRemoveButton();
        if (std::addressof(removeButton) == buttonRef)
            return parameter;
    }

    return {};
}

void CaveyAudioProcessorEditor::setLoading(bool desiredLoadingState) {
    if (isLoading == desiredLoadingState) return;
    isLoading = desiredLoadingState;
    loadingOverlay.setVisible(isLoading);
    mainLabel.setVisible(false);
    if (isLoading) loadingOverlay.toFront(true);
    repaint();
}

void CaveyAudioProcessorEditor::updateGenerateButtonEnabledState() {
    const juce::String promptText = promptEditor.getText();
    const bool hasPrompt = promptText.trim().isNotEmpty();
    const bool hasGeneratedParameter = audioProcessor.hasGeneratedParameter();
    const bool shouldEnableGenerateButton = hasPrompt && !isLoading && !hasGeneratedParameter;

    generateButton.setEnabled(shouldEnableGenerateButton);

    if (shouldEnableGenerateButton) {
        generateButton.setTooltip({});
    } else if (hasGeneratedParameter) {
        generateButton.setTooltip(CaveyUI::GENERATE_TOOLTIP_PARAMETER_EXISTS);
    } else if (!hasPrompt) {
        generateButton.setTooltip(CaveyUI::GENERATE_TOOLTIP_EMPTY_PROMPT);
    } else {
        generateButton.setTooltip(CaveyUI::GENERATE_TOOLTIP_LOADING);
    }
}
