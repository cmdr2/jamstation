#include <cassert>
#include <iostream>

#include "../src/graph.h"

void test_single_sample_pcm16() {
    const int numSamples = 1;
    int16_t inputData[2] = {8192, 16384};  // sample 0: ch0=8192 (0.25), ch1=16384 (0.5)
    int16_t outputData[2] = {0, 0};

    processAudioBlock(inputData, outputData, numSamples, FORMAT_PCM16);

    // Convert output back to float for assertion
    float out_ch0 = outputData[0] / 32768.0f;
    float out_ch1 = outputData[1] / 32768.0f;

    assert(abs(out_ch0 - 0.5f) < 0.001f);  // 0.25 * 2
    assert(abs(out_ch1 - 1.0f) < 0.001f);  // 0.5 * 2

    std::cout << "test_single_sample_pcm16 passed\n";
}

void test_multiple_samples_pcm16() {
    const int numSamples = 3;
    int16_t inputData[6] = {
        4096,  8192,  // sample0: ch0=4096 (0.125), ch1=8192 (0.25)
        12288, 4096,  // sample1: ch0=12288 (0.375), ch1=4096 (0.125)
        16384, 0      // sample2: ch0=16384 (0.5), ch1=0 (0.0)
    };
    int16_t outputData[6] = {0, 0, 0, 0, 0, 0};

    processAudioBlock(inputData, outputData, numSamples, FORMAT_PCM16);

    // Convert and check
    float out[6];
    for (int i = 0; i < 6; i++) {
        out[i] = outputData[i] / 32768.0f;
    }

    assert(abs(out[0] - 0.25f) < 0.001f);  // 0.125 * 2
    assert(abs(out[1] - 0.5f) < 0.001f);   // 0.25 * 2
    assert(abs(out[2] - 0.75f) < 0.001f);  // 0.375 * 2
    assert(abs(out[3] - 0.25f) < 0.001f);  // 0.125 * 2
    assert(abs(out[4] - 1.0f) < 0.001f);   // 0.5 * 2
    assert(abs(out[5] - 0.0f) < 0.001f);   // 0.0 * 2

    std::cout << "test_multiple_samples_pcm16 passed\n";
}

void test_single_sample_pcm24in32() {
    const int numSamples = 1;
    int32_t inputData[2] = {536870656, 1073741696};  // sample 0: ch0=0.25 left-justified, ch1=0.5 left-justified
    int32_t outputData[2] = {0, 0};

    processAudioBlock(inputData, outputData, numSamples, FORMAT_PCM24IN32);

    // Convert output back to float for assertion
    int32_t v0 = outputData[0] >> 8;
    int32_t v1 = outputData[1] >> 8;
    float out_ch0 = v0 * (1.0f / 8388608.0f);
    float out_ch1 = v1 * (1.0f / 8388608.0f);

    assert(abs(out_ch0 - 0.5f) < 0.001f);  // 0.25 * 2
    assert(abs(out_ch1 - 1.0f) < 0.001f);  // 0.5 * 2

    std::cout << "test_single_sample_pcm24in32 passed\n";
}

void test_multiple_samples_pcm24in32() {
    const int numSamples = 3;
    int32_t inputData[6] = {
        268435200,  536870656,  // sample0: ch0=0.125 left-justified, ch1=0.25 left-justified
        805306368,  268435200,  // sample1: ch0=0.375 left-justified, ch1=0.125 left-justified
        1073741696, 0           // sample2: ch0=0.5 left-justified, ch1=0.0
    };
    int32_t outputData[6] = {0, 0, 0, 0, 0, 0};

    processAudioBlock(inputData, outputData, numSamples, FORMAT_PCM24IN32);

    // Convert and check
    float out[6];
    for (int i = 0; i < 6; i++) {
        int32_t v = outputData[i] >> 8;
        out[i] = v * (1.0f / 8388608.0f);
    }

    assert(abs(out[0] - 0.25f) < 0.001f);  // 0.125 * 2
    assert(abs(out[1] - 0.5f) < 0.001f);   // 0.25 * 2
    assert(abs(out[2] - 0.75f) < 0.001f);  // 0.375 * 2
    assert(abs(out[3] - 0.25f) < 0.001f);  // 0.125 * 2
    assert(abs(out[4] - 1.0f) < 0.001f);   // 0.5 * 2
    assert(abs(out[5] - 0.0f) < 0.001f);   // 0.0 * 2

    std::cout << "test_multiple_samples_pcm24in32 passed\n";
}

int main() {
    test_single_sample_pcm16();
    test_multiple_samples_pcm16();
    test_single_sample_pcm24in32();
    test_multiple_samples_pcm24in32();

    std::cout << "All tests passed!\n";
    return 0;
}