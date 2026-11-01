// libFuzzer target: sample decoding through Kaia's loadSample (JUCE's WAV/AIFF/FLAC/Ogg/MP3
// readers plus Kaia's own handling of their metadata: root note, loop points).
#include "FuzzCommon.h"
#include "Formats/SampleSource.h"

extern "C" int LLVMFuzzerTestOneInput (const uint8_t* data, size_t size)
{
    kaia::fuzz::ScratchFile scratch (data, size, ".wav");
    auto& file = scratch.file;
    juce::String error;
    auto sample = kaia::loadSample ({ file.getFullPathName(), {} }, error);
    if (sample != nullptr && sample->loopEnabled
        && (sample->loopStart < 0 || sample->loopStart >= sample->loopEnd || sample->loopEnd > sample->numFrames + 1))
    {
        fprintf (stderr, "inconsistent loop: %lld..%lld of %lld frames\n",
                 (long long) sample->loopStart, (long long) sample->loopEnd, (long long) sample->numFrames);
        std::abort();
    }
    return 0;
}
