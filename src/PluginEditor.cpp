#include "PluginEditor.h"
#include "PluginProcessor.h"

class CaveyAudioProcessor; // forward-declare to match include order

CaveyAudioProcessorEditor::CaveyAudioProcessorEditor(CaveyAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
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
    // Generate the knob here.
    if (parameterKnobs.size() < MAX_PARAMETER_AMOUNT) {
        // TODO: Get the name from user somehow?
        auto * parameter = new Parameter("Gain");
        parameter->setRemoveButtonListener(this);
        parameterKnobs.emplace_back(parameter);
        addAndMakeVisible(parameter);
        parameter->setLabel("Gain");
        parameterAdded();

        // Get text from the editor
        auto const& inputPrompt = promptEditor.getText();
        PRINT(inputPrompt);
        // Call into llama here with the prompt.

        // Generate the parameter
        audioProcessor.addBackendParameter( "Gain");
    }
}

void CaveyAudioProcessorEditor::sliderValueChanged(juce::Slider *slider) {
    // We have to set the processor value as that
    PRINT(slider->getValue());
    audioProcessor.setBackendParameterValue(slider->getName(), slider->getValue() / 100);
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



