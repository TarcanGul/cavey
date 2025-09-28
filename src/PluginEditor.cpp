#include "PluginEditor.h"
#include "PluginProcessor.h"

class CaveyAudioProcessor; // forward-declare to match include order

CaveyAudioProcessorEditor::CaveyAudioProcessorEditor(CaveyAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setSize(600, 600); // Set the GUI size to 600x600 pixels
}

CaveyAudioProcessorEditor::~CaveyAudioProcessorEditor() = default;

void CaveyAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(32.0f, juce::Font::bold));
    g.drawFittedText("Hello World!", getLocalBounds(), juce::Justification::centred, 1);
}

