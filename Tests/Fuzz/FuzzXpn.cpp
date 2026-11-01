// libFuzzer target: .xpn expansions (zip). Runs what opening a pack does: ZipReader's directory
// parse, every entry's decompression, Expansion.xml / cover art, program listing and parsing.
#include "FuzzCommon.h"
#include "Formats/ZipReader.h"
#include "Formats/KitLoader.h"

extern "C" int LLVMFuzzerTestOneInput (const uint8_t* data, size_t size)
{
    kaia::fuzz::ScratchFile scratch (data, size, ".xpn");
    auto& file = scratch.file;

    {
        kaia::ZipReader zip (file);
        if (zip.isValid())
        {
            auto& entries = zip.getEntries();
            for (int i = 0; i < (int) juce::jmin<size_t> (entries.size(), 32); ++i)
            {
                juce::MemoryBlock out;
                juce::String error;
                zip.readEntry (i, out, error);
            }
        }
    }

    kaia::PackInfo info;
    juce::String error;
    auto programs = kaia::KitLoader::listPrograms (file, &info, error);
    for (size_t i = 0; i < juce::jmin<size_t> (programs.size(), 4); ++i)
    {
        kaia::Kit kit;
        juce::StringArray warnings;
        kaia::KitLoader::parseProgram (programs[i].location, kit, warnings, error);
    }

    juce::MemoryBlock cover;
    kaia::KitLoader::readPackArtwork (file, cover);
    return 0;
}
