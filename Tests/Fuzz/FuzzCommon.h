// Shared helpers for the libFuzzer targets in this folder (see Tests/Fuzz/README.md).
#pragma once

#include "Model/Kit.h"
#include "Engine/KitRuntime.h"
#include "Engine/SamplerEngine.h"
#include <juce_core/juce_core.h>
#include <cmath>
#include <cstdlib>
#include <unistd.h>

namespace kaia::fuzz
{

// Some code paths only take a file (ZipReader, the audio decoders), so the input is written to a
// scratch file (in $KAIA_FUZZ_TMP if set, e.g. /dev/shm, else the temp folder). Every input gets a
// new name: ZipReader::open caches readers by path and modification time. The file is deleted
// when the ScratchFile goes out of scope.
struct ScratchFile
{
    ScratchFile (const uint8_t* data, size_t size, const char* extension)
    {
        static int counter = 0;
        auto dir = juce::File::getSpecialLocation (juce::File::tempDirectory);
        if (auto* tmp = std::getenv ("KAIA_FUZZ_TMP"))
            dir = juce::File (juce::String::fromUTF8 (tmp));
        file = dir.getChildFile ("kaia-fuzz-" + juce::String ((int) getpid()) + "-" + juce::String (counter++) + extension);
        file.replaceWithData (data, size);
    }
    ~ScratchFile() { file.deleteFile(); }

    juce::File file;
};

// Plays a parsed kit the way the plugin would: every layer gets a short synthetic sample, every
// note is struck, and the output must stay finite. Catches parameter values from a file
// (tuning, envelope, filter, gains) that would make the engine produce NaN/Inf or crash.
inline void renderKit (const Kit& kit)
{
    static SamplerEngine engine;
    static bool prepared = false;
    if (! prepared)
    {
        engine.prepare (44100.0);
        prepared = true;
    }

    static SampleDataPtr tone = []
    {
        auto sd = std::make_shared<SampleData>();
        sd->sampleRate = 44100.0;
        sd->allocate (2, 600);
        for (int c = 0; c < 2; ++c)
            for (int i = 0; i < 600; ++i)
                sd->writePointer (c)[i] = 0.5f * std::sin (0.07f * (float) i * (float) (c + 1));
        return sd;
    }();

    SampleMap samples;
    for (auto& ref : kit.allSampleRefs())
        samples[ref.toString()] = tone;

    engine.allSoundOff();
    engine.setRuntime (buildRuntime (kit, &samples));

    constexpr int block = 256;
    static float buffers[SamplerEngine::kNumOutputs][2][block];
    OutputBus outs[SamplerEngine::kNumOutputs];
    for (int o = 0; o < SamplerEngine::kNumOutputs; ++o)
        outs[o] = { buffers[o][0], buffers[o][1] };

    juce::MidiBuffer midi;
    for (int b = 0; b < 6; ++b)
    {
        midi.clear();
        if (b == 0)
            for (int n = 0; n < 128; ++n)
                midi.addEvent (juce::MidiMessage::noteOn (1, n, (juce::uint8) (1 + (n * 37) % 127)), n);
        if (b == 3)
            for (int n = 0; n < 128; ++n)
                midi.addEvent (juce::MidiMessage::noteOff (1, n), n);

        for (int o = 0; o < SamplerEngine::kNumOutputs; ++o)
            for (int c = 0; c < 2; ++c)
                std::fill (buffers[o][c], buffers[o][c] + block, 0.0f);

        engine.process (outs, block, midi);

        for (int o = 0; o < SamplerEngine::kNumOutputs; ++o)
            for (int c = 0; c < 2; ++c)
                for (int i = 0; i < block; ++i)
                    if (! std::isfinite (buffers[o][c][i]))
                    {
                        fprintf (stderr, "non-finite output: output %d channel %d sample %d (block %d)\n", o, c, i, b);
                        std::abort();
                    }
    }

    engine.allSoundOff();
    engine.collectGarbage();
}

}
