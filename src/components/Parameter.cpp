//
// Created by Tarcan Gul on 10/19/25.
//
#include "Parameter.h"
Parameter::Parameter(juce::String name) {
    slider_ = new Slider();
    removeButton_ = new TextButton();
    label_ = new Label();
    label_->setText("unnamed", NotificationType::dontSendNotification);

    slider_->setSliderStyle(juce::Slider::SliderStyle::Rotary);
    slider_->setRange(0, 100, 1);
    slider_->setName(name);

    removeButton_->setButtonText("Remove Parameter");

    addAndMakeVisible(slider_);
    addAndMakeVisible(label_);
    addAndMakeVisible(removeButton_);
}

Parameter::~Parameter() {
    delete slider_;
    delete removeButton_;
    delete label_;
}

Slider *Parameter::getSlider() {
    return slider_;
}

Button * Parameter::getRemoveButton() {
    return removeButton_;
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
