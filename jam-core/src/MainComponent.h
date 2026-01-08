#pragma once

#include <JuceHeader.h>

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent  : public juce::AudioAppComponent, public juce::KeyListener
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
    //==============================================================================
    // Your private member variables go here...
    juce::TextButton looperToggleButton { "Enable Looper" };
    juce::TextButton atanSoftToggleButton { "Enable Rock" };
    juce::TextButton tubeOverdriveToggleButton { "Enable Tube Overdrive" };
    juce::TextButton delayToggleButton { "Enable Delay" };
    juce::TextButton clickToggleButton { "Enable Click" };
    juce::TextButton loopingLabel { "Looping" };

    bool looperEnabled = false;
    bool tubeOverdriveEnabled = false;
    bool atanSoftEnabled = false;
    bool delayEnabled = false;
    bool clickEnabled = false;

    // looper
    bool isRecordingLoop = false;
    bool hasLoop = false;

    juce::AudioSampleBuffer loopBuffer;
    int loopBufferPos = 0;
    int loopBufferLength = 0;
    int maxLoopSamples = 60 * 44100; // 60 seconds max

    int maxDelaySamples = 44100; // 1 second at 44.1kHz
    std::vector<float> delayBuffer[2]; // one per channel
    int delayWritePos[2] = {};         // write index
    float feedback = 0.4f;                       // 0.0 to <1.0
    float mix = 0.5f;                            // dry/wet mix (0=dry, 1=wet)

    double clickSampleRate = 44100.0;
    double bpm = 92.0;
    int samplesPerBeat = 0;
    int samplesSinceLastClick = 0;

    // Click parameters
    float clickVolume = 0.7f;
    int clickLength = 200; // in samples
    int clickSamplePos = 0;

    void handleLooper(float &sample, int channel);
    void startLoopRecording();
    void stopLoopRecording();
    bool isLoopBtnDown() const;

    bool keyPressed (const juce::KeyPress& key, juce::Component* originatingComponent) override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
