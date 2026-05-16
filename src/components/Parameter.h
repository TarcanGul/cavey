//
// Created by Tarcan Gul on 10/19/25.
//
#pragma once

#include "../types/CaveyTypes.h"
#include <JuceHeader.h>

using SliderAttachment =  juce::AudioProcessorValueTreeState::SliderAttachment;

/**
 * Represents a parameter that can be generated and tuned.
 */
class Parameter : public juce::Component {
public:
    Parameter(const juce::String& name, juce::AudioProcessorValueTreeState& valueTree);
    ~Parameter() override = default;
    void resized() override;

    // Both call mutating methods
    Slider& getSlider();
    Button& getRemoveButton();
    void setLabel(const String& label);
    void setRemoveButtonListener(Button::Listener *listener);
private:
    static constexpr int REMOVE_BUTTON_WIDTH = 100;
    static constexpr int REMOVE_BUTTON_HORIZONTAL_PADDING = 10;
    static constexpr int REMOVE_BUTTON_TOP_PADDING = 10;
    static constexpr int LABEL_WIDTH = 100;
    static constexpr int SLIDER_VALUE_TEXT_WIDTH = 60;
    static constexpr int SLIDER_VALUE_TEXT_HEIGHT = 18;

    std::unique_ptr<Label> label_;
    std::unique_ptr<Slider> slider_;
    std::unique_ptr<SliderAttachment> sliderAttachment_;
    std::unique_ptr<Button> removeButton_;
};
