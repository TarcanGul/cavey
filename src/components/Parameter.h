//
// Created by Tarcan Gul on 10/19/25.
//
#pragma once

#include "../types/CaveyTypes.h"
#include <JuceHeader.h>

typedef juce::AudioProcessorValueTreeState::SliderAttachment SliderAttachment;
typedef juce::AudioProcessorValueTreeState::ButtonAttachment ButtonAttachment;

/**
 * Represents a parameter that can be generated and tuned.
 */
class Parameter : public juce::Component {
public:
    explicit Parameter(const juce::String& name);
    ~Parameter() override = default;
    void resized() override;

    // Both call mutating methods
    Slider& getSlider();
    Button& getRemoveButton();
    void setLabel(String label);
    void setRemoveButtonListener(Button::Listener *listener);
private:
    static constexpr int REMOVE_BUTTON_WIDTH = 100;
    static constexpr int LABEL_WIDTH = 100;

    std::unique_ptr<Label> label_;
    std::unique_ptr<Slider> slider_;
    std::unique_ptr<Button> removeButton_;
};
