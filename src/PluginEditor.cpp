#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "controllers/OllamaController.h"

class CaveyAudioProcessor; // forward-declare to match include order

CaveyAudioProcessorEditor::CaveyAudioProcessorEditor(CaveyAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    juce::Logger::writeToLog("Editor is starting...");
    setSize(INIT_SCREEN_WIDTH, INIT_SCREEN_HEIGHT);
    setResizable(true, true);

    // Create the parameter area that first be a text box.
    mainLabel.setText(MAIN_LABEL_TEXT, NotificationType::dontSendNotification);
    mainLabel.setEditable(false);

    promptEditor.setTextToShowWhenEmpty(PROMPT_PLACEHOLDER_TEXT, Colours::grey.withAlpha(.6f));

    generateButton.setButtonText(GENERATE_BUTTON_TEXT);
    generateButton.addListener(this);

    addAndMakeVisible(&mainLabel);
    addAndMakeVisible(&promptEditor);
    addAndMakeVisible(&generateButton);

    // TODO: Maybe a static factory is good
    llm = static_cast<LLMController *>(new OllamaController());

    juce::Logger::writeToLog("Editor has been initialized.");
}

CaveyAudioProcessorEditor::~CaveyAudioProcessorEditor() {
    generateButton.removeListener(this);
    for (auto parameter : parameterKnobs) {
        delete parameter;
    }
}

void CaveyAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void CaveyAudioProcessorEditor::resized() {
    auto screen = getLocalBounds();
    auto promptBounds = screen.removeFromBottom(PROMPT_HEIGHT);
    auto buttonBounds = promptBounds.removeFromRight(PROMPT_WIDTH);

    if (parameterKnobs.empty()) {
        mainLabel.setVisible(true);
        mainLabel.setBounds(screen.withSizeKeepingCentre(mainLabel.getFont().getStringWidth(MAIN_LABEL_TEXT), 20));
    } else {
        renderParameterKnobs();
    }

    // Divide the screen to four areas (header - main area - text area - footer)
    promptEditor.setBounds(promptBounds.reduced(MARGIN_SMALL));
    generateButton.setBounds(buttonBounds.reduced(MARGIN_EXTRA_SMALL, MARGIN_SMALL));
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
    juce::Logger::writeToLog("Generate button is clicked");

    // Generate the knob here.
    if (parameterKnobs.size() < MAX_PARAMETER_AMOUNT) {
        // Get text from the editor
        auto const& inputPrompt = promptEditor.getText();

        // Async operation, show loading screen
        const juce::String response = this->llm->prompt(inputPrompt);

        boost::system::error_code errorCode;
        const boost::json::value readResponse = boost::json::parse(response.toStdString(), errorCode);
        const boost::json::object parsedResponse = readResponse.as_object();

        juce::Logger::writeToLog("Response is returned");

        juce::String parameterName = juce::String(parsedResponse.at("NAME").get_string().c_str());

        auto * parameter = new Parameter(parameterName);
        parameter->setRemoveButtonListener(this);
        parameterKnobs.emplace_back(parameter);
        addAndMakeVisible(parameter);
        parameter->setLabel(parameterName);
        parameter->getSlider()->addListener(this);
        parameterAdded();

        // Generate the parameter
        audioProcessor.addBackendParameter( parameterName, {
                { BaseEffect::VOLUME, parsedResponse.at("VOLUME").get_double() },
                { BaseEffect::LOW_PASS, parsedResponse.at("LOW_PASS").get_double() },
                { BaseEffect::HIGH_PASS, parsedResponse.at("HIGH_PASS").get_double() },
                { BaseEffect::REVERB, parsedResponse.at("REVERB").get_double() },
                { BaseEffect::DISTORTION, parsedResponse.at("DISTORTION").get_double()}
        });
    }
}

void CaveyAudioProcessorEditor::sliderValueChanged(juce::Slider *slider) {
    audioProcessor.setBackendParameterValue(slider->getName(), static_cast<float>(slider->getValue()));
}

void CaveyAudioProcessorEditor::whenRemoveParameterButtonClicked(Parameter * parameterGroup) {
    Slider * parameter = parameterGroup->getSlider();
    Button * removeButton = parameterGroup->getRemoveButton();
    PRINT("remove button clicked");
    const auto newListHead = std::remove_if(
            parameterKnobs.begin(),
            parameterKnobs.end(),
            [parameter, removeButton](Parameter * parameterKnob) {
                return parameterKnob->getSlider() == parameter &&
                parameterKnob->getRemoveButton() == removeButton;
            }
        );
    parameterKnobs.erase(newListHead, parameterKnobs.end());
    removeButton->removeListener(this);
    delete parameterGroup;
    parameterRemoved();
}

inline void CaveyAudioProcessorEditor::parameterAdded() {
    PRINT("Parameter added");
    renderParameterKnobs();
    mainLabel.setVisible(false);
}

inline void CaveyAudioProcessorEditor::parameterRemoved() {
    renderParameterKnobs();
    if (parameterKnobs.empty()) {
        mainLabel.setVisible(true);
    }
}

void CaveyAudioProcessorEditor::renderParameterKnobs() const noexcept {
    auto screen = getLocalBounds();
    for (size_t i = 0; i < parameterKnobs.size(); ++i) {
        auto parameter = parameterKnobs[i];
        parameter->setBounds(screen.removeFromTop(KNOB_INIT_Y_POS + KNOB_HEIGHT * i));
    }
}

std::optional<Parameter *> CaveyAudioProcessorEditor::getParameterGroup(Button *buttonRef) {
    for (auto parameter : parameterKnobs) {
        Button * removeButton = parameter->getRemoveButton();
        if (removeButton == buttonRef)
            return parameter;
    }

    return {};
}


