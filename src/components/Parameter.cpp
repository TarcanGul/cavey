//
// Created by Tarcan Gul on 10/19/25.
//
#include "Parameter.h"
Parameter::Parameter(const juce::String& name) {
    slider_ = std::make_unique<Slider>();
    removeButton_ = std::make_unique<TextButton>();
    label_ = std::make_unique<Label>();
    label_->setText("unnamed", NotificationType::dontSendNotification);

    slider_->setSliderStyle(juce::Slider::SliderStyle::Rotary);
    slider_->setRange(0, 1, 0.01);
    slider_->setValue(0);
    slider_->setName(name);

    removeButton_->setButtonText("Remove Parameter");

    addAndMakeVisible(slider_.get());
    addAndMakeVisible(label_.get());
    addAndMakeVisible(removeButton_.get());
}

Slider& Parameter::getSlider() {
    return *slider_;
}

Button& Parameter::getRemoveButton() {
    return *removeButton_;
}

void Parameter::setRemoveButtonListener(Button::Listener * listener) {
    removeButton_->addListener(listener);
}

void Parameter::resized() {
    auto componentBounds = getLocalBounds();
    label_->setBounds(componentBounds.removeFromLeft(LABEL_WIDTH));
    slider_->setBounds(componentBounds);
    removeButton_->setBounds(componentBounds.removeFromRight(REMOVE_BUTTON_WIDTH) );
}

void Parameter::setLabel(juce::String label) {
    label_->setText(label, NotificationType::dontSendNotification);
}
