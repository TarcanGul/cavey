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
    if (!parameterKnobs.empty()) {
        for (auto it = parameterKnobs.begin(); it != parameterKnobs.end(); it++) {
            delete *it;
        }
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
        for (auto it = parameterKnobs.begin(); it != parameterKnobs.end(); it++) {
            Slider * knob = *it;
            knob->setBounds(screen.removeFromTop(KNOB_INIT_Y_POS + KNOB_HEIGHT * std::distance(parameterKnobs.begin(), it)));
        }
    }

    // Divide the screen to four areas (header - main area - text area - footer)
    promptEditor.setBounds(promptBounds.reduced(MARGIN_SMALL));
    generateButton.setBounds(buttonBounds.reduced(MARGIN_EXTRA_SMALL, MARGIN_SMALL));
}

void CaveyAudioProcessorEditor::buttonClicked(juce::Button *buttonRef) {
    if (buttonRef == &generateButton) {
        whenGenerateButtonClicked();
    }
}

void CaveyAudioProcessorEditor::whenGenerateButtonClicked() {
    // Generate the knob here.
    if (parameterKnobs.size() < MAX_PARAMETER_AMOUNT) {
        Slider * slider = new Slider();
        slider->setSliderStyle(juce::Slider::SliderStyle::Rotary);
        slider->setRange(0, 100, 1);
        addAndMakeVisible(slider);
        parameterKnobs.push_back(slider);
        parameterAdded();
    }
}

void CaveyAudioProcessorEditor::parameterAdded() {
    PRINT("Parameter added");
    auto screen = getLocalBounds();
    for (auto it = parameterKnobs.begin(); it != parameterKnobs.end(); it++) {
        Slider * knob = *it;
        knob->setBounds(screen.removeFromTop(KNOB_INIT_Y_POS + KNOB_HEIGHT * std::distance(parameterKnobs.begin(), it)));
    }
    mainLabel.setVisible(false);
}

