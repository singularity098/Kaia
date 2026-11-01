// libFuzzer target: Akai .pgm programs (MPC500/1000/2500, MPC2000/2000XL, MPC3000).
#include "FuzzCommon.h"
#include "Formats/Parsers.h"

extern "C" int LLVMFuzzerTestOneInput (const uint8_t* data, size_t size)
{
    juce::MemoryBlock mb (data, size);
    kaia::Kit kit;
    juce::StringArray warnings;
    juce::String error;
    if (kaia::parsePgm (mb, kit, warnings, error))
        kaia::fuzz::renderKit (kit);
    return 0;
}
