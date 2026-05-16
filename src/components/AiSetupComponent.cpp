#include "AiSetupComponent.h"

#include "../PluginProcessor.h"

namespace {

constexpr int kDialogWidth = 360;
constexpr int kDialogHeight = 190;
constexpr int kMargin = 14;
constexpr int kRowHeight = 28;
constexpr int kGap = 8;

}  // namespace

AiSetupComponent::AiSetupComponent(
    CaveyAudioProcessor& processor,
    std::function<void()> selectionChangedCallback)
    : processor_(processor),
      selectionChangedCallback_(std::move(selectionChangedCallback)) {
    setSize(kDialogWidth, kDialogHeight);

    titleLabel_.setText("AI setup", juce::NotificationType::dontSendNotification);
    titleLabel_.setFont(titleLabel_.getFont().withHeight(18.0f).boldened());
    addAndMakeVisible(titleLabel_);

    providerLabel_.setText("Ollama", juce::NotificationType::dontSendNotification);
    providerLabel_.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(providerLabel_);

    modelComboBox_.setTextWhenNothingSelected("Choose a local model");
    modelComboBox_.onChange = [this] {
        if (!isPopulating_) {
            saveSelectedModel();
        }
    };
    addAndMakeVisible(modelComboBox_);

    refreshButton_.setButtonText("Refresh");
    refreshButton_.onClick = [this] { refreshModels(); };
    addAndMakeVisible(refreshButton_);

    selectButton_.setButtonText("Select");
    selectButton_.onClick = [this] {
        saveSelectedModel();
        if (auto * dialogWindow = findParentComponentOfClass<juce::DialogWindow>()) {
            dialogWindow->exitModalState(0);
        }
    };
    addAndMakeVisible(selectButton_);

    statusLabel_.setJustificationType(juce::Justification::centredLeft);
    statusLabel_.setColour(juce::Label::textColourId, juce::Colours::grey);
    addAndMakeVisible(statusLabel_);

    refreshModels();
}

AiSetupComponent::~AiSetupComponent() {
    isAlive_ = false;
}

void AiSetupComponent::resized() {
    auto bounds = getLocalBounds().reduced(kMargin);
    titleLabel_.setBounds(bounds.removeFromTop(kRowHeight));
    bounds.removeFromTop(kGap);

    providerLabel_.setBounds(bounds.removeFromTop(kRowHeight));
    bounds.removeFromTop(kGap);

    auto modelRow = bounds.removeFromTop(kRowHeight);
    refreshButton_.setBounds(modelRow.removeFromRight(90).reduced(kGap / 2, 0));
    modelComboBox_.setBounds(modelRow);
    bounds.removeFromTop(kGap);

    auto actionRow = bounds.removeFromTop(kRowHeight);
    selectButton_.setBounds(actionRow.removeFromRight(90));
    bounds.removeFromTop(kGap);

    statusLabel_.setBounds(bounds);
}

void AiSetupComponent::refreshModels() {
    refreshButton_.setEnabled(false);
    selectButton_.setEnabled(false);
    modelComboBox_.setEnabled(false);
    setStatus("Checking Ollama...");

    juce::Component::SafePointer<AiSetupComponent> safeThis(this);
    std::thread([safeThis, processor = &processor_] {
        juce::StringArray models;
        juce::String status;

        try {
            models = processor->fetchOllamaModels();
            if (models.isEmpty()) {
                status = "No downloaded Ollama models found.";
            }
        } catch (const std::exception& exception) {
            status = juce::String("Unable to read Ollama models: ") + exception.what();
        }

        juce::MessageManager::callAsync([safeThis, models, status] {
            if (safeThis == nullptr || !safeThis->isAlive_) {
                return;
            }

            safeThis->refreshButton_.setEnabled(true);
            safeThis->populateModels(models);
            safeThis->modelComboBox_.setEnabled(!models.isEmpty());
            safeThis->selectButton_.setEnabled(!models.isEmpty());

            if (status.isNotEmpty()) {
                safeThis->setStatus(status);
            } else {
                const juce::String selectedModel = safeThis->processor_.getSelectedOllamaModel();
                if (selectedModel.isNotEmpty()) {
                    safeThis->setStatus("Selected: " + selectedModel);
                } else {
                    safeThis->setStatus("Select an Ollama model to enable generation.");
                }
            }
        });
    }).detach();
}

void AiSetupComponent::populateModels(const juce::StringArray& models) {
    models_ = models;
    isPopulating_ = true;
    modelComboBox_.clear(juce::NotificationType::dontSendNotification);

    for (int index = 0; index < models_.size(); ++index) {
        modelComboBox_.addItem(models_[index], index + 1);
    }

    selectCurrentComboModel();
    isPopulating_ = false;
}

void AiSetupComponent::selectCurrentComboModel() {
    const juce::String selectedModel = processor_.getSelectedOllamaModel();
    const int selectedIndex = models_.indexOf(selectedModel);
    if (selectedIndex >= 0) {
        modelComboBox_.setSelectedId(selectedIndex + 1, juce::NotificationType::dontSendNotification);
    } else {
        modelComboBox_.setSelectedId(0, juce::NotificationType::dontSendNotification);
    }
}

void AiSetupComponent::saveSelectedModel() {
    const int selectedIndex = modelComboBox_.getSelectedId() - 1;
    if (selectedIndex < 0 || selectedIndex >= models_.size()) {
        setStatus("Choose a downloaded Ollama model first.");
        return;
    }

    const juce::String model = models_[selectedIndex];
    processor_.setSelectedOllamaModel(model);
    if (selectionChangedCallback_) {
        selectionChangedCallback_();
    }
    setStatus("Selected: " + model);
}

void AiSetupComponent::setStatus(const juce::String& status) {
    statusLabel_.setText(status, juce::NotificationType::dontSendNotification);
}
