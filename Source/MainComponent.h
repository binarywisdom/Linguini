#pragma once

#include <JuceHeader.h>
#include <chrono>

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent  : public juce::AudioAppComponent
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent() override;

    //==============================================================================
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    //==============================================================================
    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    void SourceSelectClicked();
    void CreateNoSilenceClicked();
    juce::AudioBuffer<float> GetRidOfSilence();

    //==============================================================================
    // Your private member variables go here...
    juce::TextButton mSourceSelectButton;
    juce::TextButton mCreateNoSilenceButton;


    struct SilenceCuttingSettings
    {
        // everything below threshhold must go
        float threshold{ 0.004f };

        // maximum duration of silence to leave in the file
        //std::chrono::duration<float, std::chrono::seconds> maxSilence{ 2.0f };
        float maxSilence{ 1.0f };
    };

    SilenceCuttingSettings mSilenceCuttingSettings;

    juce::AudioFormatManager mFormatManager;
    juce::AudioBuffer<float> mAudioBuffer;

    double mSampleRate;
    int mBitsPerSample;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
