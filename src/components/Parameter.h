//
// Created by Tarcan Gul on 10/19/25.
//

#ifndef CAVEYPLUGIN_PARAMETER_H
#define CAVEYPLUGIN_PARAMETER_H

#include <JuceHeader.h>

/**
 * Represents a parameter that can be generated and tuned.
 */
class Parameter : public juce::Component {
public:
    // Location to render
    Parameter();
    ~Parameter() override;
    void resized() override;

    Slider * getSlider();
    Button * getRemoveButton();
    void setRemoveButtonListener(Button::Listener *listener);
private:
    Slider * slider_;
    Button * removeButton_;
};


#endif //CAVEYPLUGIN_PARAMETER_H
