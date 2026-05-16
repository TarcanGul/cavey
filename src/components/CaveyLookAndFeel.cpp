#include "CaveyLookAndFeel.h"

#include <algorithm>
#include <cmath>

namespace {

constexpr juce::uint32 kPrimaryHex = 0xff05299e;
constexpr juce::uint32 kSecondaryHex = 0xff5e4ae3;
constexpr juce::uint32 kTertiaryHex = 0xff947bd3;
constexpr juce::uint32 kBackgroundHex = 0xfff0a7a0;
constexpr juce::uint32 kTextHex = 0xff151629;

constexpr float kButtonCornerRadius = 6.0f;
constexpr float kButtonShadowAlpha = 0.18f;
constexpr float kDisabledAlpha = 0.42f;
constexpr float kRotaryTrackThickness = 4.0f;
constexpr float kRotaryValueThickness = 5.5f;

}  // namespace

CaveyLookAndFeel::CaveyLookAndFeel() {
  setColour(juce::ResizableWindow::backgroundColourId, BackgroundColour());
  setColour(juce::DocumentWindow::backgroundColourId, BackgroundColour());
  setColour(juce::Label::textColourId, TextColour());
  setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
  setColour(juce::TextButton::buttonColourId, PrimaryColour());
  setColour(juce::TextButton::buttonOnColourId, SecondaryColour());
  setColour(juce::TextButton::textColourOffId, juce::Colours::white);
  setColour(juce::TextButton::textColourOnId, juce::Colours::white);
  setColour(juce::Slider::rotarySliderFillColourId, SecondaryColour());
  setColour(juce::Slider::rotarySliderOutlineColourId,
            TertiaryColour().withAlpha(0.42f));
  setColour(juce::Slider::thumbColourId, PrimaryColour());
  setColour(juce::Slider::textBoxTextColourId, TextColour());
  setColour(juce::Slider::textBoxBackgroundColourId,
            juce::Colours::white.withAlpha(0.68f));
  setColour(juce::Slider::textBoxOutlineColourId,
            TertiaryColour().withAlpha(0.7f));
  setColour(juce::TextEditor::backgroundColourId,
            juce::Colours::white.withAlpha(0.72f));
  setColour(juce::TextEditor::textColourId, TextColour());
  setColour(juce::TextEditor::highlightColourId,
            SecondaryColour().withAlpha(0.35f));
  setColour(juce::TextEditor::highlightedTextColourId, TextColour());
  setColour(juce::TextEditor::outlineColourId,
            TertiaryColour().withAlpha(0.8f));
  setColour(juce::TextEditor::focusedOutlineColourId, PrimaryColour());
  setColour(juce::ComboBox::backgroundColourId,
            juce::Colours::white.withAlpha(0.82f));
  setColour(juce::ComboBox::textColourId, TextColour());
  setColour(juce::ComboBox::outlineColourId, TertiaryColour());
  setColour(juce::ComboBox::arrowColourId, PrimaryColour());
  setColour(juce::ComboBox::buttonColourId, juce::Colours::transparentBlack);
  setColour(juce::PopupMenu::backgroundColourId, juce::Colours::white);
  setColour(juce::PopupMenu::textColourId, TextColour());
  setColour(juce::PopupMenu::highlightedBackgroundColourId, SecondaryColour());
  setColour(juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
}

void CaveyLookAndFeel::drawRotarySlider(juce::Graphics& graphics, int x, int y,
                                        int width, int height,
                                        float sliderPosProportional,
                                        float rotaryStartAngle,
                                        float rotaryEndAngle,
                                        juce::Slider& slider) {
  const auto bounds = juce::Rectangle<float>(
      static_cast<float>(x), static_cast<float>(y), static_cast<float>(width),
      static_cast<float>(height));
  const float radius = std::max(
      0.0f, std::min(bounds.getWidth(), bounds.getHeight()) * 0.5f - 8.0f);
  const auto centre = bounds.getCentre();
  const auto knobBounds = juce::Rectangle<float>(
      centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);
  const float angle =
      rotaryStartAngle +
      sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

  graphics.setColour(juce::Colours::black.withAlpha(0.22f));
  graphics.fillEllipse(knobBounds.translated(0.0f, 3.0f));

  juce::ColourGradient baseGradient(
      TertiaryColour().brighter(0.35f), knobBounds.getX(), knobBounds.getY(),
      PrimaryColour().darker(0.18f), knobBounds.getRight(),
      knobBounds.getBottom(), false);
  baseGradient.addColour(0.55, SecondaryColour());
  graphics.setGradientFill(baseGradient);
  graphics.fillEllipse(knobBounds);

  graphics.setColour(juce::Colours::white.withAlpha(0.25f));
  graphics.fillEllipse(knobBounds.reduced(radius * 0.18f)
                           .translated(-radius * 0.08f, -radius * 0.12f));

  juce::Path trackArc;
  trackArc.addCentredArc(centre.x, centre.y, radius + 4.0f, radius + 4.0f, 0.0f,
                         rotaryStartAngle, rotaryEndAngle, true);
  graphics.setColour(
      slider.findColour(juce::Slider::rotarySliderOutlineColourId));
  graphics.strokePath(trackArc, juce::PathStrokeType(kRotaryTrackThickness));

  juce::Path valueArc;
  valueArc.addCentredArc(centre.x, centre.y, radius + 4.0f, radius + 4.0f, 0.0f,
                         rotaryStartAngle, angle, true);
  graphics.setColour(slider.findColour(juce::Slider::rotarySliderFillColourId));
  graphics.strokePath(valueArc, juce::PathStrokeType(kRotaryValueThickness));

  const float indicatorLength = radius * 0.68f;
  const float indicatorStart = radius * 0.18f;
  juce::Line<float> indicator(centre.x + std::cos(angle) * indicatorStart,
                              centre.y + std::sin(angle) * indicatorStart,
                              centre.x + std::cos(angle) * indicatorLength,
                              centre.y + std::sin(angle) * indicatorLength);
  const juce::Line<float> indicatorShadow(
      indicator.getStartX() + 1.0f, indicator.getStartY() + 1.0f,
      indicator.getEndX() + 1.0f, indicator.getEndY() + 1.0f);

  graphics.setColour(juce::Colours::black.withAlpha(0.18f));
  graphics.drawLine(indicatorShadow, 3.0f);
  graphics.setColour(juce::Colours::white.withAlpha(0.92f));
  graphics.drawLine(indicator, 3.0f);
}

void CaveyLookAndFeel::drawButtonBackground(
    juce::Graphics& graphics, juce::Button& button,
    const juce::Colour& backgroundColour, bool shouldDrawButtonAsHighlighted,
    bool shouldDrawButtonAsDown) {
  auto bounds = button.getLocalBounds().toFloat().reduced(1.0f, 2.0f);
  const bool isEnabled = button.isEnabled();
  juce::Colour baseColour = backgroundColour;

  if (shouldDrawButtonAsDown) {
    baseColour = SecondaryColour().darker(0.1f);
    bounds.translate(0.0f, 1.0f);
  } else if (shouldDrawButtonAsHighlighted) {
    baseColour = SecondaryColour();
  }

  if (!isEnabled) {
    baseColour = TertiaryColour().interpolatedWith(BackgroundColour(), 0.55f);
  }

  graphics.setColour(
      juce::Colours::black.withAlpha(isEnabled ? kButtonShadowAlpha : 0.08f));
  graphics.fillRoundedRectangle(
      bounds.translated(0.0f, isEnabled ? 2.0f : 1.0f), kButtonCornerRadius);

  juce::ColourGradient gradient(baseColour.brighter(isEnabled ? 0.18f : 0.06f),
                                bounds.getX(), bounds.getY(),
                                baseColour.darker(isEnabled ? 0.18f : 0.04f),
                                bounds.getX(), bounds.getBottom(), false);
  graphics.setGradientFill(gradient);
  graphics.setOpacity(isEnabled ? 1.0f : kDisabledAlpha);
  graphics.fillRoundedRectangle(bounds, kButtonCornerRadius);

  graphics.setOpacity(1.0f);
  graphics.setColour(juce::Colours::white.withAlpha(isEnabled ? 0.2f : 0.08f));
  graphics.drawRoundedRectangle(bounds.reduced(0.5f), kButtonCornerRadius,
                                1.0f);
}

juce::Colour CaveyLookAndFeel::PrimaryColour() {
  return juce::Colour(kPrimaryHex);
}

juce::Colour CaveyLookAndFeel::SecondaryColour() {
  return juce::Colour(kSecondaryHex);
}

juce::Colour CaveyLookAndFeel::TertiaryColour() {
  return juce::Colour(kTertiaryHex);
}

juce::Colour CaveyLookAndFeel::BackgroundColour() {
  return juce::Colour(kBackgroundHex);
}

juce::Colour CaveyLookAndFeel::TextColour() { return juce::Colour(kTextHex); }
