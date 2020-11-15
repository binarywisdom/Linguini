#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent()
{
    addAndMakeVisible(mSourceSelectButton);
    mSourceSelectButton.setButtonText("Select a source...");
    mSourceSelectButton.onClick = [this] {SourceSelectClicked(); };

    addAndMakeVisible(mCreateNoSilenceButton);
    mCreateNoSilenceButton.setButtonText("Create a \"no silence\" file");
    mCreateNoSilenceButton.onClick = [this] {CreateNoSilenceClicked(); };

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

    mSourceSelectButton.setBounds(getWidth()/2, 10, 200, 100);
    mCreateNoSilenceButton.setBounds(getWidth()/2, 150, 200, 100);
}

void MainComponent::SourceSelectClicked()
{
    juce::FileChooser chooser("Select a Wave file to play...",
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
        } 
    }
}

void MainComponent::CreateNoSilenceClicked()
{
    //
    //  Handle no-opened-file case
    //

    juce::File file{ "D:/C++ Projects/Linguini/testfiles/result.wav" };
    file.deleteFile();

    juce::ScopedPointer<juce::FileOutputStream> fStream{
        file.createOutputStream().release() };

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

    writer->writeFromAudioSampleBuffer(mAudioBuffer, 0, mAudioBuffer.getNumSamples());
}