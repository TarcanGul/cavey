#pragma once

#include <JuceHeader.h>

class CaveyLookAndFeel final : public juce::LookAndFeel_V4 {
 public:
  CaveyLookAndFeel();
  ~CaveyLookAndFeel() override = default;

  void drawRotarySlider(juce::Graphics& graphics, int x, int y, int width,
                        int height, float sliderPosProportional,
                        float rotaryStartAngle, float rotaryEndAngle,
                        juce::Slider& slider) override;

  void drawButtonBackground(juce::Graphics& graphics, juce::Button& button,
                            const juce::Colour& backgroundColour,
                            bool shouldDrawButtonAsHighlighted,
                            bool shouldDrawButtonAsDown) override;

 private:
  static juce::Colour PrimaryColour();
  static juce::Colour SecondaryColour();
  static juce::Colour TertiaryColour();
  static juce::Colour BackgroundColour();
  static juce::Colour TextColour();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CaveyLookAndFeel)
};
