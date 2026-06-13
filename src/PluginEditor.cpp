#include "PluginEditor.h"
#include "PluginProcessor.h"
class CaveyAudioProcessor; // forward-declare to match include order

CaveyAudioProcessorEditor::CaveyAudioProcessorEditor(CaveyAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    juce::Logger::writeToLog("Editor is starting...");
    setLookAndFeel(&lookAndFeel_);
    juce::LookAndFeel::setDefaultLookAndFeel(&lookAndFeel_);

    setSize(CaveyUI::INIT_SCREEN_WIDTH, CaveyUI::INIT_SCREEN_HEIGHT);
    setResizeLimits(
        CaveyUI::MIN_SCREEN_WIDTH,
        CaveyUI::MIN_SCREEN_HEIGHT,
        CaveyUI::MAX_SCREEN_WIDTH,
        CaveyUI::MAX_SCREEN_HEIGHT);
    setResizable(true, true);

    // Create the parameter area that first be a text box.
    mainLabel.setText(CaveyUI::MAIN_LABEL_TEXT, NotificationType::dontSendNotification);
    mainLabel.setEditable(false);
    mainLabel.setJustificationType(juce::Justification::centred);
    mainLabel.setMinimumHorizontalScale(1.0f);

    promptEditor.setTextToShowWhenEmpty(CaveyUI::PROMPT_PLACEHOLDER_TEXT, Colours::grey.withAlpha(.6f));
    promptEditor.onTextChange = [this] {
        updateGenerateButtonEnabledState();
    };

    aiSetupButton.setButtonText(CaveyUI::AI_SETUP_BUTTON_TEXT);
    aiSetupButton.setTooltip("Configure Ollama model");
    aiSetupButton.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    aiSetupButton.addListener(this);

    selectedModelIndicator.setJustificationType(juce::Justification::centred);
    selectedModelIndicator.setFont(selectedModelIndicator.getFont().withHeight(12.0f));
    selectedModelIndicator.setMinimumHorizontalScale(0.6f);
    selectedModelIndicator.setColour(juce::Label::textColourId, juce::Colours::grey);
    updateSelectedModelIndicator();

    generateButton.setButtonText(CaveyUI::GENERATE_BUTTON_TEXT);
    generateButton.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    generateButton.addListener(this);
    updateGenerateButtonEnabledState();

    addAndMakeVisible(&mainLabel);
    addAndMakeVisible(&promptEditor);
    addAndMakeVisible(&aiSetupButton);
    addAndMakeVisible(&selectedModelIndicator);
    addAndMakeVisible(&generateButton);
    addChildComponent(&loadingOverlay);
    loadingOverlay.setVisible(false);

    audioProcessor.addActionListener(this);
    const juce::String generatedParameterName = audioProcessor.getGeneratedParameterName();
    if (generatedParameterName.isNotEmpty()) {
        addParameterControl(generatedParameterName);
    }

    juce::Logger::writeToLog("Editor has been initialized.");
}

CaveyAudioProcessorEditor::~CaveyAudioProcessorEditor() {
    setLookAndFeel(nullptr);
    juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
    aiSetupButton.removeListener(this);
    generateButton.removeListener(this);
    promptEditor.onTextChange = nullptr;
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
    const int promptHeight = juce::jlimit(
        CaveyUI::MIN_PROMPT_HEIGHT,
        CaveyUI::PROMPT_HEIGHT,
        getHeight() / 2);
    auto promptBounds = screen.removeFromBottom(promptHeight);
    auto buttonBounds = promptBounds.removeFromRight(CaveyUI::PROMPT_WIDTH);

    if (parameterKnobs.empty()) {
        mainLabel.setVisible(true);
        mainLabel.setBounds(screen.reduced(CaveyUI::MARGIN_SMALL));
    } else {
        renderParameterKnobs();
    }

    // Divide the bottom prompt area between the text editor and prompt controls.
    promptEditor.setBounds(promptBounds.reduced(CaveyUI::MARGIN_SMALL));
    auto promptButtonBounds = buttonBounds.reduced(CaveyUI::MARGIN_EXTRA_SMALL, CaveyUI::MARGIN_SMALL);

    aiSetupButton.setBounds(promptButtonBounds.removeFromTop(CaveyUI::AI_SETUP_BUTTON_HEIGHT));
    selectedModelIndicator.setBounds(
        promptButtonBounds.removeFromTop(CaveyUI::SELECTED_MODEL_INDICATOR_HEIGHT));
    promptButtonBounds.removeFromTop(CaveyUI::AI_SETUP_BUTTON_GAP);
    generateButton.setBounds(
        promptButtonBounds.withSizeKeepingCentre(
            juce::jmax(promptButtonBounds.getWidth(), CaveyUI::MIN_GENERATE_BUTTON_WIDTH),
            juce::jmax(promptButtonBounds.getHeight(), CaveyUI::MIN_GENERATE_BUTTON_HEIGHT)));
    loadingOverlay.setBounds(screen);
}

void CaveyAudioProcessorEditor::buttonClicked(juce::Button *buttonRef) {
    juce::Logger::writeToLog("Button is clicked");
    if (buttonRef == &generateButton) {
        whenGenerateButtonClicked();
        return;
    }
    if (buttonRef == &aiSetupButton) {
        openAiSetupDialog();
        return;
    }

    std::optional<Parameter *> maybeParameterTuple = getParameterGroup(buttonRef);
    if (maybeParameterTuple.has_value()) {
        whenRemoveParameterButtonClicked(maybeParameterTuple.value());
    }
}

void CaveyAudioProcessorEditor::openAiSetupDialog() {
    juce::DialogWindow::LaunchOptions options;
    options.dialogTitle = "AI setup";
    options.content.setOwned(new AiSetupComponent(audioProcessor, [this] {
        updateSelectedModelIndicator();
        updateGenerateButtonEnabledState();
    }));
    options.componentToCentreAround = this;
    options.dialogBackgroundColour = getLookAndFeel().findColour(
        juce::ResizableWindow::backgroundColourId);
    options.escapeKeyTriggersCloseButton = true;
    options.useNativeTitleBar = true;
    options.resizable = false;
    options.launchAsync();
}

void CaveyAudioProcessorEditor::whenGenerateButtonClicked() {
    if (parameterKnobs.size() >= CaveyUI::MAX_PARAMETER_AMOUNT) {
        // Error toast maybe
        return;
    }

    setLoading(true);
    updateGenerateButtonEnabledState();

    const juce::String prompt = promptEditor.getText();
    juce::Component::SafePointer<CaveyAudioProcessorEditor> safeThis(this);
    CaveyAudioProcessor* processor = &audioProcessor;

    std::thread([safeThis, processor, p = prompt] {
        GenerateParameterResult result;

        try {
            result = processor->addCaveyParameter(p);
        } catch (const std::exception& exception) {
            result = {
                .success = false,
                .parameterName = {},
                .errorMessage = exception.what()
            };
            juce::Logger::writeToLog("Generation worker failed: " + result.errorMessage);
        } catch (...) {
            result = {
                .success = false,
                .parameterName = {},
                .errorMessage = "Unknown generation error."
            };
            juce::Logger::writeToLog("Generation worker failed with an unknown error.");
        }

        if (result.success) {
            juce::Logger::writeToLog("Prompt sent as action message");
        }

        juce::MessageManager::callAsync([safeThis, result] {
            if (safeThis == nullptr) {
                return;
            }

            safeThis->setLoading(false);

            if (!result.success) {
                const juce::String errorMessage = result.errorMessage.isNotEmpty()
                    ? result.errorMessage
                    : "Generation failed.";
                safeThis->mainLabel.setText(
                    "Generation failed: " + errorMessage,
                    juce::NotificationType::dontSendNotification);
                safeThis->mainLabel.setVisible(true);
            }

            safeThis->updateGenerateButtonEnabledState();
        });
    }).detach();
}

void CaveyAudioProcessorEditor::sliderValueChanged(juce::Slider *slider) {
    audioProcessor.setBackendParameterValue(slider->getName(), static_cast<float>(slider->getValue()));
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
    audioProcessor.clearGeneratedParameter();
    renderParameterKnobs();
    if (parameterKnobs.empty()) {
        mainLabel.setVisible(true);
    }
    updateGenerateButtonEnabledState();
}

void CaveyAudioProcessorEditor::addParameterControl(const juce::String& parameterName) {
    auto * parameter = new Parameter(parameterName, audioProcessor.getValueTree());
    parameter->setLabel(parameterName);
    parameter->setRemoveButtonListener(this);
    parameter->getSlider().addListener(this);

    addAndMakeVisible(parameter);
    parameterKnobs.emplace_back(parameter);
    Logger::writeToLog("Parameter added");
    renderParameterKnobs();
    mainLabel.setVisible(false);
    updateGenerateButtonEnabledState();
}

void CaveyAudioProcessorEditor::renderParameterKnobs() const noexcept {
    auto screen = getLocalBounds();
    for (size_t i = 0; i < parameterKnobs.size(); ++i) {
        auto parameter = parameterKnobs[i];
        auto parameterBounds = screen.removeFromTop(CaveyUI::KNOB_INIT_Y_POS + CaveyUI::KNOB_HEIGHT * i);
        parameterBounds.removeFromLeft(CaveyUI::MARGIN_SMALL);
        parameter->setBounds(parameterBounds);
    }
}

void CaveyAudioProcessorEditor::actionListenerCallback(const juce::String &message) {
    if (message.isEmpty()) {
        juce::Logger::writeToLog("Empty message received, noop");
        return;
    }

    // Right now it is hardcoded, only processor will send parameter name, can be changed later
    const juce::String parameterName {message};

    juce::Logger::writeToLog("Parameter is added via the callback");

    juce::Component::SafePointer<CaveyAudioProcessorEditor> safeThis(this);
    juce::MessageManager::callAsync([safeThis, parameterName] {
        if (safeThis == nullptr) {
            return;
        }

        safeThis->setLoading(false);
        safeThis->addParameterControl(parameterName);
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
    const bool hasSelectedModel = audioProcessor.hasSelectedOllamaModel();
    const bool shouldEnableGenerateButton =
        hasPrompt && !isLoading && !hasGeneratedParameter && hasSelectedModel;

    generateButton.setEnabled(shouldEnableGenerateButton);

    if (shouldEnableGenerateButton) {
        generateButton.setTooltip({});
    } else if (hasGeneratedParameter) {
        generateButton.setTooltip(CaveyUI::GENERATE_TOOLTIP_PARAMETER_EXISTS);
    } else if (!hasSelectedModel) {
        generateButton.setTooltip(CaveyUI::GENERATE_TOOLTIP_MODEL_REQUIRED);
    } else if (!hasPrompt) {
        generateButton.setTooltip(CaveyUI::GENERATE_TOOLTIP_EMPTY_PROMPT);
    } else {
        generateButton.setTooltip(CaveyUI::GENERATE_TOOLTIP_LOADING);
    }
}

void CaveyAudioProcessorEditor::updateSelectedModelIndicator() {
    const juce::String selectedModel = audioProcessor.getSelectedOllamaModel();
    const juce::String modelText = selectedModel.isNotEmpty()
        ? selectedModel
        : CaveyUI::NO_AI_MODEL_SELECTED_TEXT;

    selectedModelIndicator.setText(
        juce::String(CaveyUI::AI_PROVIDER_TEXT) + "\n" + modelText,
        juce::NotificationType::dontSendNotification);
}
