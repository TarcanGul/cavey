#include "AiSetupComponent.h"

#include "../PluginProcessor.h"

namespace {

constexpr int kDialogWidth = 520;
constexpr int kDialogHeight = 320;
constexpr int kNavWidth = 130;
constexpr int kMargin = 16;
constexpr int kRowHeight = 28;

}  // namespace

AiSetupComponent::AiSetupComponent(
        CaveyAudioProcessor& processor,
        CompletionCallback completion_callback)
    : processor_(processor),
      completion_callback_(std::move(completion_callback)) {
    setSize(kDialogWidth, kDialogHeight);

    for (auto* button : {
             &openai_nav_button_,
             &anthropic_nav_button_,
             &ollama_nav_button_,
             &refresh_models_button_,
             &connect_button_}) {
        button->addListener(this);
        addAndMakeVisible(button);
    }

    title_label_.setFont(juce::Font(juce::FontOptions(18.0f, juce::Font::bold)));
    detail_label_.setColour(juce::Label::textColourId, juce::Colours::grey);
    stored_key_label_.setColour(juce::Label::textColourId,
                                juce::Colours::lightgreen);
    status_label_.setJustificationType(juce::Justification::centredLeft);

    openai_key_editor_.setPasswordCharacter(0x2022);
    anthropic_key_editor_.setPasswordCharacter(0x2022);
    openai_key_editor_.setTextToShowWhenEmpty("OpenAI API key",
                                              juce::Colours::grey);
    anthropic_key_editor_.setTextToShowWhenEmpty("Anthropic API key",
                                                 juce::Colours::grey);
    ollama_model_box_.addListener(this);

    for (auto* component : std::initializer_list<juce::Component*>{
             &title_label_,
             &detail_label_,
             &stored_key_label_,
             &openai_key_editor_,
             &anthropic_key_editor_,
             &ollama_model_box_,
             &status_label_}) {
        addAndMakeVisible(component);
    }

    refreshPane();
}

AiSetupComponent::~AiSetupComponent() {
    for (auto* button : {
             &openai_nav_button_,
             &anthropic_nav_button_,
             &ollama_nav_button_,
             &refresh_models_button_,
             &connect_button_}) {
        button->removeListener(this);
    }

    ollama_model_box_.removeListener(this);
    openai_key_editor_.clear();
    anthropic_key_editor_.clear();
}

void AiSetupComponent::paint(juce::Graphics& graphics) {
    graphics.fillAll(getLookAndFeel().findColour(
            juce::ResizableWindow::backgroundColourId));
    graphics.setColour(juce::Colours::darkgrey);
    graphics.drawVerticalLine(kNavWidth, 0.0f,
                              static_cast<float>(getHeight()));
}

void AiSetupComponent::resized() {
    auto bounds = getLocalBounds();
    auto nav_bounds = bounds.removeFromLeft(kNavWidth).reduced(kMargin);
    openai_nav_button_.setBounds(nav_bounds.removeFromTop(kRowHeight));
    nav_bounds.removeFromTop(8);
    anthropic_nav_button_.setBounds(nav_bounds.removeFromTop(kRowHeight));
    nav_bounds.removeFromTop(8);
    ollama_nav_button_.setBounds(nav_bounds.removeFromTop(kRowHeight));

    auto pane_bounds = bounds.reduced(kMargin);
    title_label_.setBounds(pane_bounds.removeFromTop(30));
    detail_label_.setBounds(pane_bounds.removeFromTop(24));
    pane_bounds.removeFromTop(12);
    stored_key_label_.setBounds(pane_bounds.removeFromTop(24));
    pane_bounds.removeFromTop(8);

    openai_key_editor_.setBounds(pane_bounds.removeFromTop(32));
    anthropic_key_editor_.setBounds(openai_key_editor_.getBounds());
    ollama_model_box_.setBounds(openai_key_editor_.getBounds());

    auto refresh_bounds = ollama_model_box_.getBounds().removeFromRight(96);
    refresh_models_button_.setBounds(refresh_bounds.reduced(4, 0));

    pane_bounds.removeFromTop(16);
    connect_button_.setBounds(pane_bounds.removeFromTop(34).removeFromLeft(120));
    pane_bounds.removeFromTop(10);
    status_label_.setBounds(pane_bounds.removeFromTop(44));
}

void AiSetupComponent::buttonClicked(juce::Button* button) {
    if (button == &openai_nav_button_) {
        selectProvider(Cavey::AiProvider::kOpenAI);
    } else if (button == &anthropic_nav_button_) {
        selectProvider(Cavey::AiProvider::kAnthropic);
    } else if (button == &ollama_nav_button_) {
        selectProvider(Cavey::AiProvider::kOllama);
    } else if (button == &refresh_models_button_) {
        refreshOllamaModels();
    } else if (button == &connect_button_) {
        connectSelectedProvider();
    }
}

void AiSetupComponent::comboBoxChanged(juce::ComboBox* combo_box) {
    juce::ignoreUnused(combo_box);
}

void AiSetupComponent::selectProvider(Cavey::AiProvider provider) {
    selected_provider_ = provider;
    refreshPane();
}

void AiSetupComponent::refreshPane() {
    openai_nav_button_.setToggleState(selected_provider_ == Cavey::AiProvider::kOpenAI,
                                      juce::dontSendNotification);
    anthropic_nav_button_.setToggleState(
            selected_provider_ == Cavey::AiProvider::kAnthropic,
            juce::dontSendNotification);
    ollama_nav_button_.setToggleState(selected_provider_ == Cavey::AiProvider::kOllama,
                                      juce::dontSendNotification);

    openai_key_editor_.setVisible(selected_provider_ == Cavey::AiProvider::kOpenAI);
    anthropic_key_editor_.setVisible(
            selected_provider_ == Cavey::AiProvider::kAnthropic);
    ollama_model_box_.setVisible(selected_provider_ == Cavey::AiProvider::kOllama);
    refresh_models_button_.setVisible(
            selected_provider_ == Cavey::AiProvider::kOllama);

    title_label_.setText(Cavey::ToProviderDisplayName(selected_provider_),
                         juce::dontSendNotification);

    if (selected_provider_ == Cavey::AiProvider::kOllama) {
        detail_label_.setText("Use a model already downloaded in Ollama.",
                              juce::dontSendNotification);
        stored_key_label_.setText(ollama_model_box_.getText().isNotEmpty()
                                          ? "Model selected"
                                          : "Select a local model",
                                  juce::dontSendNotification);
        if (ollama_model_box_.getNumItems() == 0) {
            refreshOllamaModels();
        }
    } else {
        detail_label_.setText("Enter your API key to connect this provider.",
                              juce::dontSendNotification);
        stored_key_label_.setText(
                processor_.hasStoredCredential(selected_provider_)
                        ? "API key is set in secure storage"
                        : "No API key stored",
                juce::dontSendNotification);
    }

    connect_button_.setButtonText(
            selected_provider_ == Cavey::AiProvider::kOllama
                    ? "Connect"
                    : "Connect");
    resized();
}

void AiSetupComponent::refreshOllamaModels() {
    setBusy(true);
    setStatus({}, false);

    const juce::Component::SafePointer<AiSetupComponent> safe_this(this);
    CaveyAudioProcessor& processor = processor_;
    std::thread([safe_this, &processor] {
        try {
            const auto models = processor.fetchOllamaModels();
            juce::MessageManager::callAsync([safe_this, models] {
                if (safe_this == nullptr) {
                    return;
                }

                safe_this->ollama_model_box_.clear();
                for (int i = 0; i < models.size(); ++i) {
                    safe_this->ollama_model_box_.addItem(models[i], i + 1);
                }
                if (models.size() > 0) {
                    safe_this->ollama_model_box_.setSelectedId(1);
                }
                safe_this->setBusy(false);
                safe_this->refreshPane();
            });
        } catch (const std::exception& exception) {
            juce::MessageManager::callAsync([safe_this,
                                             message = juce::String(exception.what())] {
                if (safe_this == nullptr) {
                    return;
                }

                safe_this->setBusy(false);
                safe_this->setStatus(message, true);
            });
        }
    }).detach();
}

void AiSetupComponent::connectSelectedProvider() {
    Cavey::ProviderConnectionConfig config;
    if (selected_provider_ == Cavey::AiProvider::kOpenAI) {
        config.api_key = openai_key_editor_.getText();
    } else if (selected_provider_ == Cavey::AiProvider::kAnthropic) {
        config.api_key = anthropic_key_editor_.getText();
    } else if (selected_provider_ == Cavey::AiProvider::kOllama) {
        config.ollama_model = ollama_model_box_.getText();
    }

    setBusy(true);
    setStatus({}, false);
    const auto provider = selected_provider_;
    const juce::Component::SafePointer<AiSetupComponent> safe_this(this);
    CaveyAudioProcessor& processor = processor_;

    std::thread([safe_this, &processor, provider, config] {
        auto result = processor.connectAiProvider(provider, config);
        juce::MessageManager::callAsync([safe_this, result] {
            if (safe_this == nullptr) {
                return;
            }

            safe_this->openai_key_editor_.clear();
            safe_this->anthropic_key_editor_.clear();
            safe_this->setBusy(false);
            safe_this->setStatus(result.message, !result.connected);
            safe_this->refreshPane();
            if (safe_this->completion_callback_) {
                safe_this->completion_callback_(result.connected, result.message);
            }
        });
    }).detach();
}

void AiSetupComponent::setBusy(bool should_be_busy) {
    is_busy_ = should_be_busy;
    connect_button_.setEnabled(!is_busy_);
    refresh_models_button_.setEnabled(!is_busy_);
}

void AiSetupComponent::setStatus(const juce::String& status, bool is_error) {
    status_label_.setText(status, juce::dontSendNotification);
    status_label_.setColour(juce::Label::textColourId,
                            is_error ? juce::Colours::orange
                                     : juce::Colours::lightgreen);
}
