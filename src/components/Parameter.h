//
// Created by Tarcan Gul on 10/19/25.
//
#pragma once

#include "../types/CaveyTypes.h"
#include <JuceHeader.h>

/**
 * Represents a parameter that can be generated and tuned.
 */
class Parameter : public juce::Component {
public:
    explicit Parameter(juce::String name);
    ~Parameter() override;
    void resized() override;

    Slider * getSlider();
    Button * getRemoveButton();
    void setLabel(String label);
    void setRemoveButtonListener(Button::Listener *listener);
private:
    static constexpr int REMOVE_BUTTON_WIDTH = 100;
    static constexpr int LABEL_WIDTH = 100;

    Label * label_;
    Slider * slider_;
    Button * removeButton_;
};
