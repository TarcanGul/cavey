#include "PluginEditor.h"
#include "PluginProcessor.h"

class CaveyAudioProcessor; // forward-declare to match include order

CaveyAudioProcessorEditor::CaveyAudioProcessorEditor(CaveyAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setSize(600, 600); // Set the GUI size to 600x600 pixels

    // Create the parameter area that first be a text box.
    mainLabel.setText("Welcome to Cavey!", NotificationType::dontSendNotification);
    mainLabel.setEditable(false);

    promptEditor.setText("Write your prompt here.", false);

    addAndMakeVisible(&mainLabel);
    addAndMakeVisible(&promptEditor);
}

CaveyAudioProcessorEditor::~CaveyAudioProcessorEditor() = default;

void CaveyAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void CaveyAudioProcessorEditor::resized() {
    auto screen = getLocalBounds();
    // Divide the screen to four areas (header - main area - text area - footer)
    mainLabel.setBounds(screen.withSizeKeepingCentre(mainLabel.getFont().getStringWidth("Welcome to Cavey!"), 20));
    promptEditor.setBounds(screen.removeFromBottom(200));
}

