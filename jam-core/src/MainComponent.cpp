#include "MainComponent.h"

#include <iostream>

//==============================================================================
MainComponent::MainComponent()
{
    // Make sure you set the size of the component after
    // you add any child components.
    setSize (800, 600);

    addKeyListener(this);
    setWantsKeyboardFocus(true);
    setFocusContainer(true);
    grabKeyboardFocus();

    DBG("Hahaha");

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
        
        addAndMakeVisible(looperToggleButton);
        looperToggleButton.onClick = [this] {
            looperEnabled = !looperEnabled;
            looperToggleButton.setButtonText(looperEnabled ? "Disable Looper" : "Enable Looper");
        };
        
        addAndMakeVisible(tubeOverdriveToggleButton);
        tubeOverdriveToggleButton.onClick = [this] {
            tubeOverdriveEnabled = !tubeOverdriveEnabled;
            tubeOverdriveToggleButton.setButtonText(tubeOverdriveEnabled ? "Disable Tube Overdrive" : "Enable Tube Overdrive");
        };

        addAndMakeVisible(loopingLabel);
        loopingLabel.onClick = [this] {
            if (!isRecordingLoop && !hasLoop)
                startLoopRecording();
            else if (isRecordingLoop)
                stopLoopRecording();
            // looperToggleButton.setButtonText(looperEnabled ? "Disable Looper" : "Enable Looper");
        };
        
        addAndMakeVisible(atanSoftToggleButton);
        atanSoftToggleButton.onClick = [this] {
            atanSoftEnabled = !atanSoftEnabled;
            atanSoftToggleButton.setButtonText(atanSoftEnabled ? "Disable Rock" : "Enable Rock");
        };
        
        addAndMakeVisible(delayToggleButton);
        delayToggleButton.onClick = [this] {
            delayEnabled = !delayEnabled;
            delayToggleButton.setButtonText(delayEnabled ? "Disable Delay" : "Enable Delay");
        };
        
        addAndMakeVisible(clickToggleButton);
        clickToggleButton.onClick = [this] {
            clickEnabled = !clickEnabled;
            clickToggleButton.setButtonText(clickEnabled ? "Disable Click" : "Enable Click");
        };
        
        // addAndMakeVisible(cubicSoftToggleButton);
        // cubicSoftToggleButton.onClick = [this] {
        //     cubicSoftEnabled = !cubicSoftEnabled;
        //     cubicSoftToggleButton.setButtonText(cubicSoftEnabled ? "Disable Cubic Softclip" : "Enable Cubic Softclip");
        // };
        
        // addAndMakeVisible(expSoftSatToggleButton);
        // expSoftSatToggleButton.onClick = [this] {
        //     expSoftSatEnabled = !expSoftSatEnabled;
        //     expSoftSatToggleButton.setButtonText(expSoftSatEnabled ? "Disable Exp Soft Saturation" : "Enable Exp Soft Saturation");
        // };
    }
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
    
    juce::ignoreUnused (samplesPerBlockExpected, sampleRate);

    // delay
    for (int ch = 0; ch < 2; ++ch) {
        delayBuffer[ch].resize(maxDelaySamples, 0.0f);
        delayWritePos[ch] = 0;
    }

    // metronome
    clickSampleRate = sampleRate;
    samplesPerBeat = static_cast<int>((60.0 / bpm) * sampleRate);
    samplesSinceLastClick = 0;
    clickSamplePos = clickLength;
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    // Your audio-processing code goes here!

    // For more details, see the help for AudioProcessor::getNextAudioBlock()

    // Right now we are not producing any data, in which case we need to clear the buffer
    // (to prevent the output of random noise)
    // Just passthrough input to output
    auto* device = deviceManager.getCurrentAudioDevice();
    auto activeInputChannels  = device->getActiveInputChannels();
    auto activeOutputChannels = device->getActiveOutputChannels();
    auto maxChannels = std::min(activeInputChannels.getHighestBit(), activeOutputChannels.getHighestBit()) + 1;

    auto* inputBuffer  = bufferToFill.buffer;
    auto numSamples = bufferToFill.numSamples;

    // Static filters per channel (basic 1-pole filters)
    static float hpState[2] = { 0.0f, 0.0f };
    static float lpState[2] = { 0.0f, 0.0f };

    int delaySamples = 22050; // ~0.5 second delay

    for (int channel = 0; channel < maxChannels; ++channel)
    {
        if ((! activeInputChannels[channel]) || (! activeOutputChannels[channel]))
            continue;

        auto* in  = inputBuffer->getReadPointer (channel, bufferToFill.startSample);
        auto* out = inputBuffer->getWritePointer (channel, bufferToFill.startSample);

        // std::memcpy(out, in, sizeof(float) * (size_t) numSamples);
        for (int i = 0; i < numSamples; i++) {
            float sample = in[i];
            
            // if (hardClipEnabled) {
            //     sample = std::clamp(sample * 5.0f, -0.3f, 0.3f); // hard clipping with 5.0 gain
            // }
            if (tubeOverdriveEnabled) {
                sample = std::tanh(sample * 5.0f); // tanh soft clipping (tube-style overdrive) with 5.0 gain
            }
            if (atanSoftEnabled) {
                // sample *= 10.0f;
                // sample *= std::tanh(sample + 0.2f);
                // sample *= 0.6f;

                // Tighten low end (high-pass filter)
                float alpha = 0.85f; //0.995f;
                hpState[channel] = sample + alpha * (hpState[channel] - sample);
                sample = hpState[channel];

                // // Boost input signal
                sample *= 12.0f;

                // Hard clipping
                sample = std::clamp(sample, -0.4f, 0.4f);

                // Tame high-end fizz (low-pass filter)
                float beta = 0.05f;
                lpState[channel] += beta * (sample - lpState[channel]);
                sample = lpState[channel];

                // // Output gain
                sample *= 2.0f;
            }
            // if (cubicSoftEnabled) {
            //     if (sample > 1.0f) {
            //         sample = 1.0f;
            //     } else if (sample < -1.0f) {
            //         sample = -1.0f;
            //     } else {
            //         sample = sample - (1.0f / 3.0f) * sample * sample * sample;
            //     }
            // }
            // if (expSoftSatEnabled) {
            //     sample = sample / (1.0f + std::abs(sample));
            // }

            if (delayEnabled) {
                int readPos = (delayWritePos[channel] + maxDelaySamples - delaySamples) % maxDelaySamples;
                float delayedSample = delayBuffer[channel][readPos];

                float output = sample * (1.0f - mix) + delayedSample * mix;

                delayBuffer[channel][delayWritePos[channel]] = sample + delayedSample * feedback;

                sample = output;

                delayWritePos[channel] = (delayWritePos[channel] + 1) % maxDelaySamples;
            }

            if (looperEnabled) {
                handleLooper(sample, channel);
            }
            
            out[i] = sample;
        }
    }
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
    // g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    // You can add your drawing code here!
    
    g.fillAll (juce::Colours::black);
    g.setColour (juce::Colours::white);
    g.setFont (20.0f);
    g.drawText ("Live Guitar Monitor + Fuzz", getLocalBounds(), juce::Justification::centred, true);
}

void MainComponent::resized()
{
    // This is called when the MainContentComponent is resized.
    // If you add any child components, this is where you should
    // update their positions.
    looperToggleButton.setBounds(10, 10, 150, 40);
    tubeOverdriveToggleButton.setBounds(10, 60, 150, 40);
    loopingLabel.setBounds(10, 110, 150, 40);
    atanSoftToggleButton.setBounds(10, 160, 150, 40);
    delayToggleButton.setBounds(10, 210, 150, 40);
    clickToggleButton.setBounds(10, 260, 150, 40);
}

bool MainComponent::keyPressed(const juce::KeyPress& key, juce::Component*)
{
    if (!looperEnabled)
        return false;

    if (key.getKeyCode() == juce::KeyPress::createFromDescription("ctrl").getKeyCode())
    {
        if (!isRecordingLoop && !hasLoop)
            startLoopRecording();
        else if (isRecordingLoop)
            stopLoopRecording();

        return true;
    }

    return false;
}

void MainComponent::handleLooper(float &sample, int channel) {
    if (isLoopBtnDown()) {
        if (!isRecordingLoop && !hasLoop) {
            startLoopRecording();
        } else if (isRecordingLoop) {
            stopLoopRecording();
        }
    }

    if (isRecordingLoop) {
        if (loopBufferPos < maxLoopSamples) {
            loopBuffer.setSample(channel, loopBufferPos, sample);
            loopBufferPos++;
        }
    } else if (hasLoop) {
        if (loopBufferLength > 0) {
            float loopedSample = loopBuffer.getSample(channel, loopBufferPos);
            sample += loopedSample * 0.5f; // mix original + loop
        }

        loopBufferPos = (loopBufferPos + 1) % loopBufferLength;
    }
}

bool MainComponent::isLoopBtnDown() const {
    return juce::ModifierKeys::getCurrentModifiers().isCtrlDown();
}

void MainComponent::startLoopRecording() {
    loopBuffer.setSize(2, maxLoopSamples);
    loopBuffer.clear();
    loopBufferPos = 0;
    isRecordingLoop = true;
    hasLoop = false;
    loopingLabel.setButtonText("Recording");
}

void MainComponent::stopLoopRecording() {
    isRecordingLoop = false;
    hasLoop = true;
    loopBufferLength = loopBufferPos;
    loopBufferPos = 0;
    loopingLabel.setButtonText("Looping");
}
