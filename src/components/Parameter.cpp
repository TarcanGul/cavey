//
// Created by Tarcan Gul on 10/19/25.
//
#include "Parameter.h"
Parameter::Parameter(const juce::String& name, juce::AudioProcessorValueTreeState& valueTree) {
    slider_ = std::make_unique<Slider>();
    sliderAttachment_ = std::make_unique<SliderAttachment>(valueTree, name, *slider_);
    removeButton_ = std::make_unique<TextButton>();
    label_ = std::make_unique<Label>();
    label_->setText("unnamed", NotificationType::dontSendNotification);

    slider_->setSliderStyle(juce::Slider::SliderStyle::Rotary);
    slider_->setRotaryParameters(
        juce::degreesToRadians(240.0f),
        juce::degreesToRadians(480.0f),
        true);
    slider_->setTextBoxStyle(
        juce::Slider::TextBoxBelow,
        false,
        SLIDER_VALUE_TEXT_WIDTH,
        SLIDER_VALUE_TEXT_HEIGHT);
    slider_->setRange(0, 1, 0.01);
    slider_->setValue(0);
    slider_->setName(name);

    removeButton_->setButtonText("Remove Parameter");
    removeButton_->setMouseCursor(juce::MouseCursor::PointingHandCursor);

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
    auto removeButtonBounds = componentBounds
                                      .removeFromRight(REMOVE_BUTTON_WIDTH)
                                      .reduced(REMOVE_BUTTON_HORIZONTAL_PADDING, 0);
    removeButtonBounds.removeFromTop(REMOVE_BUTTON_TOP_PADDING);
    removeButton_->setBounds(removeButtonBounds);
    slider_->setBounds(componentBounds);
}

void Parameter::setLabel(const juce::String& label) {
    label_->setText(label, NotificationType::dontSendNotification);
}
