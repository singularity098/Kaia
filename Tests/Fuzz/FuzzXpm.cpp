// libFuzzer target: MPC .xpm programs (XML for MPC 1.x/2.x, gzip-JSON for MPC 3.x).
#include "FuzzCommon.h"
#include "Formats/Parsers.h"

extern "C" int LLVMFuzzerTestOneInput (const uint8_t* data, size_t size)
{
    juce::MemoryBlock mb (data, size);
    kaia::Kit kit;
    juce::StringArray warnings;
    juce::String error;
    if (kaia::parseXpm (mb, kit, warnings, error))
        kaia::fuzz::renderKit (kit);
    return 0;
}
