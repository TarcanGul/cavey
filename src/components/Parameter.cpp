//
// Created by Tarcan Gul on 10/19/25.
//
#include "Parameter.h"
Parameter::Parameter() {
    slider_ = new Slider();
    removeButton_ = new TextButton();

    slider_->setSliderStyle(juce::Slider::SliderStyle::Rotary);
    slider_->setRange(0, 100, 1);

    removeButton_->setButtonText("Remove");

    addAndMakeVisible(slider_);
    addAndMakeVisible(removeButton_);
}

Parameter::~Parameter() {
    delete slider_;
    delete removeButton_;
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
    slider_->setBounds(componentBounds);
    removeButton_->setBounds(componentBounds.removeFromLeft(50) );
}

