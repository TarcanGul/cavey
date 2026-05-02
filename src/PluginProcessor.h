#pragma once

#include <JuceHeader.h>
#include "components/Parameter.h"
#include "types/CaveyTypes.h"
#include "types/BackendParameter.h"
#include <stdexcept>
#include "controllers/LLMController.h"

class CaveyAudioProcessor : public juce::AudioProcessor, public juce::ActionBroadcaster {
public:
    CaveyAudioProcessor();
    ~CaveyAudioProcessor() override;

    // AudioProcessor overrides
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    // Editor
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    // Basic info
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return JucePlugin_WantsMidiInput; }
    bool producesMidi() const override { return JucePlugin_ProducesMidiOutput; }
    bool isMidiEffect() const override { return JucePlugin_IsMidiEffect; }
    double getTailLengthSeconds() const override { return 0.0; }

    // Programs (not used)
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    // State
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // Sends async message
    void addCaveyParameter(const juce::String& prompt);

    void setBackendParameterValue(const juce::String& parameterName, float value);
    juce::AudioProcessorValueTreeState& getValueTree();
private:
    juce::AudioProcessorValueTreeState apvts;

    // TODO: Maybe a struct instead of `const std::map<Cavey::BaseEffect, float>& coefficients`?
    void addBackendParameter(const juce::String& parameterName, const std::map<Cavey::BaseEffect, float>& coefficients);
    std::unique_ptr<LLMController> llm;
    // TODO: look here later to see if you can get rid of the pointer
    std::map<juce::String, std::unique_ptr<BackendParameter>> parameters;
    float lastCutoffHz { 20000.0f };
    float lastTargetGain {1.0};

    enum {
        lowPassFilterIndex,
        highPassFilterIndex,
        gainIndex,
        reverbIndex,
        driveIndex,
        distortionIndex,
        chorusIndex
    };
    juce::dsp::ProcessorChain<
        juce::dsp::LadderFilter<float>,
        juce::dsp::LadderFilter<float>,
        juce::dsp::Gain<float>,
        juce::dsp::Reverb,
        juce::dsp::Gain<float>,
        juce::dsp::WaveShaper<float>,
        juce::dsp::Chorus<float>
    > processorChain;

    std::unique_ptr<juce::FileLogger> logger;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CaveyAudioProcessor)
};
