//
// Simple component that displays a centered "Loading..." text.
//
#pragma once

#include <JuceHeader.h>

class LoadingComponent : public juce::Component {
public:
    LoadingComponent();
    ~LoadingComponent() override = default;

    void resized() override;
    void paint(juce::Graphics& g) override;

private:
    juce::Label loadingLabel;
};

