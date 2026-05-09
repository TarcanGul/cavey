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
    promptEditor.onTextChange = [this] {
        updateGenerateButtonEnabledState();
    };

    generateButton.setButtonText(CaveyUI::GENERATE_BUTTON_TEXT);
    generateButton.addListener(this);

    setupButton.setButtonText(CaveyUI::SETUP_BUTTON_TEXT);
    setupButton.addListener(this);

    errorToast.setJustificationType(juce::Justification::centred);
    errorToast.setColour(juce::Label::backgroundColourId,
                         juce::Colours::darkred.withAlpha(0.9f));
    errorToast.setColour(juce::Label::textColourId, juce::Colours::white);
    errorToast.setVisible(false);

    updateGenerateButtonEnabledState();

    addAndMakeVisible(&mainLabel);
    addAndMakeVisible(&promptEditor);
    addAndMakeVisible(&generateButton);
    addAndMakeVisible(&setupButton);
    addChildComponent(&errorToast);
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
    generateButton.removeListener(this);
    setupButton.removeListener(this);
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
    auto reducedButtonBounds = buttonBounds.reduced(CaveyUI::MARGIN_EXTRA_SMALL,
                                                    CaveyUI::MARGIN_SMALL);
    setupButton.setBounds(reducedButtonBounds.removeFromTop(
            reducedButtonBounds.getHeight() / 2).reduced(0, 2));
    generateButton.setBounds(reducedButtonBounds.reduced(0, 2));
    errorToast.setBounds(getLocalBounds().withSizeKeepingCentre(360, 34));
    loadingOverlay.setBounds(screen);
}

void CaveyAudioProcessorEditor::buttonClicked(juce::Button *buttonRef) {
    juce::Logger::writeToLog("Button is clicked");
    if (buttonRef == &generateButton) {
        whenGenerateButtonClicked();
        return;
    }
    if (buttonRef == &setupButton) {
        whenSetupButtonClicked();
        return;
    }

    std::optional<Parameter *> maybeParameterTuple = getParameterGroup(buttonRef);
    if (maybeParameterTuple.has_value()) {
        whenRemoveParameterButtonClicked(maybeParameterTuple.value());
    }
}

void CaveyAudioProcessorEditor::whenGenerateButtonClicked() {
    if (parameterKnobs.size() >= CaveyUI::MAX_PARAMETER_AMOUNT) {
        return;
    }

    setLoading(true);
    updateGenerateButtonEnabledState();

    const juce::String prompt = promptEditor.getText();
    const juce::Component::SafePointer<CaveyAudioProcessorEditor> safeThis(this);

    std::thread([safeThis, p = prompt] {
        try {
            if (safeThis == nullptr) {
                return;
            }

            safeThis->audioProcessor.addCaveyParameter(p);
            juce::Logger::writeToLog("Prompt sent as action message");
        } catch (const std::exception& exception) {
            juce::MessageManager::callAsync([safeThis,
                                             message = juce::String(exception.what())] {
                if (safeThis == nullptr) {
                    return;
                }

                safeThis->setLoading(false);
                safeThis->showErrorToast(message);
                safeThis->updateGenerateButtonEnabledState();
            });
        }
    }).detach();
}

void CaveyAudioProcessorEditor::whenSetupButtonClicked() {
    const juce::Component::SafePointer<CaveyAudioProcessorEditor> safeThis(this);
    juce::DialogWindow::LaunchOptions options;
    options.dialogTitle = "Set up your AI";
    options.dialogBackgroundColour = getLookAndFeel().findColour(
            juce::ResizableWindow::backgroundColourId);
    options.escapeKeyTriggersCloseButton = true;
    options.useNativeTitleBar = true;
    options.resizable = false;
    options.content.setOwned(new AiSetupComponent(
            audioProcessor,
            [safeThis](bool connected, const juce::String& message) {
                juce::ignoreUnused(connected, message);
                if (safeThis == nullptr) {
                    return;
                }

                safeThis->updateGenerateButtonEnabledState();
            }));
    options.launchAsync();
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

    if (message == "AI_PROVIDER_CONNECTED") {
        juce::MessageManager::callAsync([this] {
            updateGenerateButtonEnabledState();
        });
        return;
    }

    // Right now it is hardcoded, only processor will send parameter name, can be changed later
    const juce::String parameterName {message};

    juce::Logger::writeToLog("Parameter is added via the callback");

    juce::MessageManager::callAsync([this, parameterName] {
        setLoading(false);
        addParameterControl(parameterName);
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
    const bool hasProvider = audioProcessor.isAiProviderConnected();
    const bool shouldEnableGenerateButton = hasPrompt && hasProvider
            && !isLoading && !hasGeneratedParameter;

    generateButton.setEnabled(shouldEnableGenerateButton);

    if (shouldEnableGenerateButton) {
        generateButton.setTooltip({});
    } else if (hasGeneratedParameter) {
        generateButton.setTooltip(CaveyUI::GENERATE_TOOLTIP_PARAMETER_EXISTS);
    } else if (!hasProvider) {
        generateButton.setTooltip(CaveyUI::GENERATE_TOOLTIP_SETUP_REQUIRED);
    } else if (!hasPrompt) {
        generateButton.setTooltip(CaveyUI::GENERATE_TOOLTIP_EMPTY_PROMPT);
    } else {
        generateButton.setTooltip(CaveyUI::GENERATE_TOOLTIP_LOADING);
    }
}

void CaveyAudioProcessorEditor::showErrorToast(const juce::String& message) {
    if (message.isEmpty()) {
        return;
    }

    errorToast.setText(message, juce::dontSendNotification);
    errorToast.setVisible(true);
    errorToast.toFront(false);
    juce::Timer::callAfterDelay(4500, [safe_this = juce::Component::SafePointer(this)] {
        if (safe_this != nullptr) {
            safe_this->errorToast.setVisible(false);
        }
    });
}
