#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent()
{
    addAndMakeVisible(mSourceSelectButton);
    mSourceSelectButton.setButtonText("Select a source...");
    mSourceSelectButton.onClick = [this] {SourceSelectClicked(); };

    addAndMakeVisible(mSourceLabel);
    mSourceLabel.setText("Select a source .wav file", juce::dontSendNotification);

    addAndMakeVisible(mCreateNoSilenceButton);
    mCreateNoSilenceButton.setButtonText("Create a \"no silence\" file");
    mCreateNoSilenceButton.onClick = [this] {CreateNoSilenceClicked(); };
    mCreateNoSilenceButton.setEnabled(false);

    // Make sure you set the size of the component after
    // you add any child components.
    setSize (800, 600);

    // Some platforms require permissions to open input channels so request that here
    if (juce::RuntimePermissions::isRequired (juce::RuntimePermissions::recordAudio)
        && ! juce::RuntimePermissions::isGranted (juce::RuntimePermissions::recordAudio))
    {
        juce::RuntimePermissions::request (juce::RuntimePermissions::recordAudio,
                                           [&] (bool granted) { setAudioChannels (granted ? 2 : 0, 2); });
    }
    else
    {
        // Specify the number of input and output channels that we want to open
        setAudioChannels (2, 2);
    }

    mFormatManager.registerBasicFormats();
}

MainComponent::~MainComponent()
{
    // This shuts down the audio device and clears the audio source.
    shutdownAudio();
}

//==============================================================================
void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    // This function will be called when the audio device is started, or when
    // its settings (i.e. sample rate, block size, etc) are changed.

    // You can use this function to initialise any resources you might need,
    // but be careful - it will be called on the audio thread, not the GUI thread.

    // For more details, see the help for AudioProcessor::prepareToPlay()
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    // Your audio-processing code goes here!

    // For more details, see the help for AudioProcessor::getNextAudioBlock()

    // Right now we are not producing any data, in which case we need to clear the buffer
    // (to prevent the output of random noise)
    bufferToFill.clearActiveBufferRegion();
}

void MainComponent::releaseResources()
{
    // This will be called when the audio device stops, or when it is being
    // restarted due to a setting change.

    // For more details, see the help for AudioProcessor::releaseResources()
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    // You can add your drawing code here!
}

void MainComponent::resized()
{
    // This is called when the MainContentComponent is resized.
    // If you add any child components, this is where you should
    // update their positions.

    mSourceSelectButton.setBounds(10, 10, 200, 100);
    mSourceLabel.setBounds(220, 10, 400, 100);
    mCreateNoSilenceButton.setBounds(10, 150, 200, 100);
}

void MainComponent::SourceSelectClicked()
{
    juce::FileChooser chooser("Select a source Wave file...",
        {},
        "*.wav");                                       

    if (chooser.browseForFileToOpen())                                          
    {
        auto file = chooser.getResult();                                        
        std::unique_ptr<juce::AudioFormatReader> reader{ mFormatManager.createReaderFor(file) };

        if (reader != nullptr)
        {
            mAudioBuffer.setSize((int)reader->numChannels, (int)reader->lengthInSamples);
            reader->read(&mAudioBuffer,                                                      
                0,                                                          
                (int)reader->lengthInSamples,                                   
                0,                                                                
                true,                                                            
                true);

            mSampleRate = reader->sampleRate;
            mBitsPerSample = reader->bitsPerSample;

            mSourceLabel.setText(file.getFullPathName(), juce::dontSendNotification);
            mCreateNoSilenceButton.setEnabled(true);
        } 
    }
}

juce::AudioBuffer<float> MainComponent::GetRidOfSilence()
{
    //  0.2  0.6  0.7  0.0  0.2
    //  0.1  0.3  0.0  0.1  0.4
    //  from                    to     

    juce::AudioBuffer<float> resultBuffer{ mAudioBuffer.getNumChannels(), mAudioBuffer.getNumSamples() };
    int totalSamples{ mAudioBuffer.getNumSamples() };

    auto FindNextSilence = [this, totalSamples](int startFrom)
    {
        auto IsQuite = [this](int index)
        {
            for (int channel{ 0 }; channel < mAudioBuffer.getNumChannels(); channel++)
                if (mAudioBuffer.getSample(channel, index) < abs(mSilenceCuttingSettings.threshold))
                    return true;

            return false;
        };

        int from{ startFrom };
        int quiteSamplesLimit{ 
            static_cast<int>(mSilenceCuttingSettings.maxSilence * mSampleRate) }; //.count()) * 
            
        while (from < totalSamples)
        {
            while (from < totalSamples && !IsQuite(from))
                from++;

            if (from == totalSamples)
                return std::pair<int, int>{totalSamples, totalSamples};

            int numQuiteSamples{ 1 };
            int to{ from + 1 };
            while (to < totalSamples && IsQuite(to))
                to++;

            if (to - from > quiteSamplesLimit)
                return std::pair<int, int>{from, to};
            else
                from = to;
        }

        return std::pair<int, int>{totalSamples, totalSamples};
    };

    std::vector<float> temp2;
    for (int i{ 0 }; i < 100; i++)
        temp2.push_back(mAudioBuffer.getSample(0, 100+i*1));

    int startCopyFrom{ 0 };
    int copyToIndex{ 0 };
    while (startCopyFrom < totalSamples)
    {
        auto [from, to] = FindNextSilence(startCopyFrom);

        if(startCopyFrom != from)
            for (int channel{ 0 }; channel < resultBuffer.getNumChannels(); channel++)
                resultBuffer.copyFrom(channel, 
                                    copyToIndex, 
                                    mAudioBuffer, 
                                    channel, 
                                    startCopyFrom, 
                                    from - startCopyFrom);

        copyToIndex += from - startCopyFrom;
        startCopyFrom = to;
    }

    std::vector<float> temp1;
    for (int i{ 0 }; i < 100; i++)
        temp1.push_back(resultBuffer.getSample(0, i*1));

    resultBuffer.setSize(resultBuffer.getNumChannels(), copyToIndex, true, true, true);
    return resultBuffer;
}

void MainComponent::CreateNoSilenceClicked()
{
    juce::FileChooser chooser("Select a resulting Wave file...",
        {},
        "*.wav");

    juce::File outFile;

    if (chooser.browseForFileToSave(true))
    {
        outFile = chooser.getResult();
    }
    else
        return;

    if(outFile.exists())
        outFile.deleteFile();

    juce::ScopedPointer<juce::FileOutputStream> fStream{
        outFile.createOutputStream().release() };

    juce::WavAudioFormat afObject;
 
    std::unique_ptr<juce::AudioFormatWriter>
        writer{ afObject.createWriterFor(
            fStream,
            mSampleRate,
            mAudioBuffer.getNumChannels(),
            mBitsPerSample,
            {},
            0
        )};
    fStream.release();

    juce::AudioBuffer<float> noSilenceBuffer{ GetRidOfSilence() };

    writer->writeFromAudioSampleBuffer(noSilenceBuffer, 0, noSilenceBuffer.getNumSamples());
}