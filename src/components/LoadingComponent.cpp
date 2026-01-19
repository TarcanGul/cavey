//
// Simple component that displays a centered "Loading..." text.
//
#include "LoadingComponent.h"

LoadingComponent::LoadingComponent() {
    setInterceptsMouseClicks(true, true);
    setAlwaysOnTop(true);

    loadingLabel.setText("Generating...", juce::NotificationType::dontSendNotification);
    loadingLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(loadingLabel);
}

void LoadingComponent::resized() {
    loadingLabel.setBounds(getLocalBounds());
}

void LoadingComponent::paint(juce::Graphics& g) { g.fillAll(juce::Colours::black.withAlpha(0.4f)); }


