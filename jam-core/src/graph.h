enum SampleFormat { FORMAT_PCM16, FORMAT_PCM24IN32 };

void processAudioBlock(const void* input, void* output, const int numSamples, const SampleFormat format);