#include "graph.h"

#include <algorithm>
#include <cassert>
#include <cmath>

static constexpr int CH = 2;

const void* g_input;  // array of input sample pointers. sample0Channel0, sample0Channel1, sample1Channel0, ...
void* g_output;       // array of output sample pointers. sample0Channel0, sample0Channel1, sample1Channel0, ...
SampleFormat g_format;
int g_sampleIdx = 0;

namespace nodes {

inline void lineIn(float* out_sample) {
    assert(g_format == FORMAT_PCM16 || g_format == FORMAT_PCM24IN32);

    if (g_format == FORMAT_PCM16) {
        auto input = (const int16_t*)g_input;
        for (int c = 0; c < CH; c++) {
            out_sample[c] = input[g_sampleIdx * CH + c] / 32768.0f;
        }
    } else if (g_format == FORMAT_PCM24IN32) {
        auto input = (const int32_t*)g_input;
        for (int c = 0; c < CH; c++) {
            // Extract valid 24-bit sample (signed)
            int32_t v = input[g_sampleIdx * CH + c] >> 8;
            // Normalize
            out_sample[c] = v * (1.0f / 8388608.0f);
        }
    }
}

inline void gain(float* in_sample, float in_gain, float* out_sample) {
    for (int c = 0; c < CH; c++) {
        out_sample[c] = in_sample[c] * in_gain;
    }
}

inline float clip(float* in_sample, float in_threshold, float* out_sample) {
    for (int c = 0; c < CH; c++) {
        out_sample[c] = std::clamp(in_sample[c], -in_threshold, in_threshold);
    }
}

inline void lineOut(float* in_sample) {
    assert(g_format == FORMAT_PCM16 || g_format == FORMAT_PCM24IN32);

    if (g_format == FORMAT_PCM16) {
        auto output = (int16_t*)g_output;
        for (int c = 0; c < CH; c++) {
            float clamped = std::clamp(in_sample[c], -1.0f, 1.0f);
            output[g_sampleIdx * CH + c] = clamped * 32767.0f;
        }
    } else if (g_format == FORMAT_PCM24IN32) {
        auto output = (int32_t*)g_output;
        for (int c = 0; c < CH; c++) {
            float clamped = std::clamp(in_sample[c], -1.0f, 1.0f);
            int32_t y = (int32_t)(clamped * 8388607.0f);
            output[g_sampleIdx * CH + c] = y << 8;
        }
    }
}

float* node0_out_sample = new float[CH];
float* node1_out_sample = new float[CH];

inline void graph() {
    lineIn(node0_out_sample);                        // node 0
    gain(node0_out_sample, 2.0f, node1_out_sample);  // node 1
    lineOut(node1_out_sample);                       // node 2
}

}  // namespace nodes

void processAudioBlock(const void* input, void* output, const int numSamples, const SampleFormat format) {
    g_input = input;
    g_output = output;
    g_format = format;

    for (g_sampleIdx = 0; g_sampleIdx < numSamples; g_sampleIdx++) {
        nodes::graph();
    }
}
