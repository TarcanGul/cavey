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
    for (auto tuple : parameterKnobs) {
        auto removeButton = std::get<Button* >(tuple);
        removeButton->removeListener(this);
        delete &tuple;
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

    std::optional<std::tuple<Slider *, Button *>> maybeParameterTuple = getParameterGroup(buttonRef);
    if (maybeParameterTuple.has_value()) {
        whenRemoveParameterButtonClicked(maybeParameterTuple.value());
    }
}

void CaveyAudioProcessorEditor::whenGenerateButtonClicked() {
    // Generate the knob here.
    if (parameterKnobs.size() < MAX_PARAMETER_AMOUNT) {
        auto * slider = new Slider();
        auto * removeButton = new TextButton();
        slider->setSliderStyle(juce::Slider::SliderStyle::Rotary);
        slider->setRange(0, 100, 1);
        removeButton->setButtonText("Remove");
        removeButton->addListener(this);
        addAndMakeVisible(slider);
        addAndMakeVisible(removeButton);
        parameterKnobs.emplace_back(std::tuple(slider, removeButton));
        parameterAdded();
    }
}

void CaveyAudioProcessorEditor::whenRemoveParameterButtonClicked(std::tuple<Slider *, Button *> parameterGroup) {
    Slider * parameter = std::get<Slider *>(parameterGroup);
    Button * removeButton = std::get<Button *>(parameterGroup);
    PRINT("remove button clicked");
    const auto newListHead = std::remove_if(
            parameterKnobs.begin(),
            parameterKnobs.end(),
            [parameter, removeButton](std::tuple<Slider *, Button *> parameterKnob) {
                return std::get<Slider *>(parameterKnob) == parameter &&
                std::get<Button *>(parameterKnob) == removeButton;
            }
        );
    removeChildComponent(parameter);
    removeChildComponent(removeButton);
    parameterKnobs.erase(newListHead, parameterKnobs.end());
    delete parameter, delete removeButton;
    // Have to remove everything related, I think I need to go to the drawing board and design OOP.
}

void CaveyAudioProcessorEditor::parameterAdded() {
    PRINT("Parameter added");
    renderParameterKnobs();
    mainLabel.setVisible(false);
}

void CaveyAudioProcessorEditor::renderParameterKnobs() const noexcept {
    auto screen = getLocalBounds();
    for (size_t i = 0; i < parameterKnobs.size(); ++i) {
        auto tuple = parameterKnobs[i];
        Slider * knob = std::get<Slider *>(tuple);
        Button * removeButton = std::get<Button *>(tuple);
        knob->setBounds(screen.removeFromTop(KNOB_INIT_Y_POS + KNOB_HEIGHT * i));
        removeButton->setBounds(screen.removeFromLeft(50) );
    }
}

std::optional<std::tuple<Slider *, Button *>> CaveyAudioProcessorEditor::getParameterGroup(Button *buttonRef) {
    for (auto tuple : parameterKnobs) {
        Button * removeButton = std::get<Button *>(tuple);
        if (removeButton == buttonRef)
            return {tuple};
    }

    return {};
}



