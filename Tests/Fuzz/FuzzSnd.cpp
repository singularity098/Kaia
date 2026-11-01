// libFuzzer target: MPC2000XL / MPC3000 .SND sounds.
#include "FuzzCommon.h"
#include "Formats/SndDecoder.h"

extern "C" int LLVMFuzzerTestOneInput (const uint8_t* data, size_t size)
{
    kaia::SampleData out;
    juce::String error;
    if (kaia::decodeMpcSnd (data, size, out, error))
    {
        // The engine trusts these: they must describe the decoded audio
        if (out.numFrames < 0 || out.defaultStart < 0 || out.defaultStart > out.defaultEnd || out.defaultEnd > out.numFrames
            || (out.loopEnabled && (out.loopStart < 0 || out.loopStart > out.loopEnd || out.loopEnd > out.numFrames)))
        {
            fprintf (stderr, "inconsistent sample bounds: frames %lld start %lld end %lld loop %lld..%lld\n",
                     (long long) out.numFrames, (long long) out.defaultStart, (long long) out.defaultEnd,
                     (long long) out.loopStart, (long long) out.loopEnd);
            std::abort();
        }
    }
    return 0;
}
