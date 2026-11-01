# Fuzz targets

Coverage-guided fuzzing ([libFuzzer](https://llvm.org/docs/LibFuzzer.html)) of the code that reads
files Kaia didn't write: programs, sounds, samples and expansions from third parties. Each target is
built with AddressSanitizer and UndefinedBehaviorSanitizer, so an out-of-bounds read, use-after-free,
integer overflow or similar fails immediately, even when the plugin itself wouldn't have crashed.

| Target | Feeds the input to | Extra checks |
|---|---|---|
| `KaiaFuzzXpm` | `parseXpm` (MPC XML and gzip-JSON `.xpm`) | the parsed kit is played: all 128 notes on and off through `SamplerEngine`, output must stay finite |
| `KaiaFuzzPgm` | `parsePgm` (MPC1000 family, MPC2000XL, MPC3000) | same playback check |
| `KaiaFuzzSnd` | `decodeMpcSnd` | start/end/loop points must lie inside the decoded audio |
| `KaiaFuzzAudio` | `loadSample` (JUCE WAV/AIFF/FLAC/Ogg/MP3 plus Kaia's loop/root-note metadata) | loop points inside the audio |
| `KaiaFuzzXpn` | `ZipReader` (directory + every entry decompressed), `listPrograms`, `parseProgram`, `readPackArtwork` | |

## Build

Needs clang with its runtime libraries (Debian/Ubuntu: `clang` and `libclang-rt-<version>-dev`).

```sh
cmake -S . -B build-fuzz -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo \
      -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ \
      -DKAIA_BUILD_PLUGIN=OFF -DKAIA_BUILD_FUZZERS=ON
cmake --build build-fuzz
```

`KAIA_FUZZ_SANITIZERS` (default `address,undefined`) picks the sanitizers; `memory` needs an
instrumented libc++ and isn't set up. UBSan's `function` check is off: JUCE 9 builds libFLAC as C,
and C calling JUCE's C++ callbacks counts as a call through the wrong function type, although
the ABI is the same. With `KAIA_JUCE_DIR`, patch that checkout first (`cmake/PatchJuce.cmake`),
or `KaiaFuzzAudio` stops at the MP3 decoder bug again.

## Run

Seed inputs (one valid file of each kind, made by the same generators as the tests):

```sh
build-fuzz/KaiaTests_artefacts/RelWithDebInfo/KaiaTests --write-fuzz-corpus fuzz/seeds
```

Then, per target (`xpm pgm snd audio xpn`):

```sh
mkdir -p fuzz/corpus/xpm
export KAIA_FUZZ_TMP=/dev/shm      # optional: where file-based targets write their input
build-fuzz/KaiaFuzzXpm -max_total_time=600 -max_len=65536 -rss_limit_mb=2048 -timeout=10 \
    fuzz/corpus/xpm fuzz/seeds/xpm
```

* The first directory collects the inputs that reached new code; reuse it next time to pick
  up where the last run stopped. The seeds directory is only read.
* A failure writes `crash-<hash>` (or `timeout-`/`oom-`) to the current directory and stops.
  Rerun it with `build-fuzz/KaiaFuzzXpm crash-<hash>`; `-minimize_crash=1 -max_total_time=60
  -exact_artifact_path=min crash-<hash>` shrinks it.
* `-fork=N -ignore_crashes=1` keeps going past known failures, with N worker processes.
* Speed with both sanitizers on 4 cores: SND ~70k inputs/s, PGM ~1k/s, XPM ~80/s, XPN ~10/s
  (deflated entries expand into large XML documents that are parsed several times). Give XPN
  hours rather than minutes, or `-fork=N`.
* `fuzz/` is only a suggestion, keep it out of the repository (corpora are big and generated).

When a crash is fixed, add the minimised input's case to `KaiaTests` as a regression check
(see the WAV `smpl` and UTF-8 cases in `testFolderAndEdgeCases`).

## Findings so far

* **JUCE XML parser read past the end of the string** with invalid UTF-8 (`<C><n></` + `C4 3E`)
  in an `.xpm`: `CharPointer_UTF8::isValidString` accepts it and `operator++` then skips the
  terminating NUL (JUCE 9 rewrote `operator++`; Kaia's own check stays). Fixed on Kaia's side: `isValidUtf8` (strict) decides between UTF-8 and Latin-1
  in `bytesToText`, zip entry names, and every outside XML file (`readTextFile`, `parseXmlFile`).
* **JUCE WAV reader allocated whatever a metadata chunk claimed** (a 1.7 KB file with a 1.7 GB
  `smpl` chunk: gigabytes of memory and millions of loop entries). `loadSample` now refuses WAVs
  whose non-audio chunks claim more than 1 MB beyond the end of the file.
* **`ZipReader::readEntry` allocated the size the directory claimed** (up to 2 GB per entry) before
  checking the archive held that much. It now checks the entry fits in the file (and, for
  deflate, can't expand more than ~1032:1) first. The central directory bounds check could also
  overflow with a huge Zip64 offset.
* **JUCE MP3 decoder wrote past `xr[1][575]`** (intensity stereo, `doIStereo` in
  `juce_MP3AudioFormat.cpp`; still in JUCE 9.0.4 and on `develop` as of 2026-10). JUCE's decoder is a
  port of mpg123 from before mpg123's fix for CVE-2006-1655. Fixed by patching JUCE at configure
  time (`cmake/PatchJuce.cmake`), which also corrects where short-block bands start (in bounds,
  but the wrong samples). All 17 saved crash inputs pass with the patch and fail without it.
