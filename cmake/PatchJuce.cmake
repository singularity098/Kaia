# Kaia's fixes to the JUCE sources, applied after CMake fetches JUCE (PATCH_COMMAND in
# CMakeLists.txt). Run by hand for a local checkout (KAIA_JUCE_DIR):
#
#   cmake -DJUCE_SOURCE_DIR=<juce checkout> -P cmake/PatchJuce.cmake
#
# CHECK_ONLY=ON reports whether the fixes are in without changing anything.
#
# The fixes are plain text replacements inside single lines, so they work whatever line endings
# the checkout has (Windows CI checks out with CRLF, which breaks `git apply`). Already patched
# sources are left alone. If JUCE changes this code, the script stops with an error: check whether
# JUCE fixed the bug itself, then update or drop the fix here.
#
# 1. MP3 intensity stereo, long blocks (found by Tests/Fuzz): JUCE's decoder is a port of mpg123
#    from before mpg123's fix for CVE-2006-1655. A corrupt frame can make maxBandl 22, whose band
#    starts at index 576, one past the end of the channel, and the last band's loop then writes
#    beyond the right channel's buffer. mpg123 returns early when maxbandl > 21; so does this.
# 2. MP3 intensity stereo, short blocks: JUCE starts each band at shortDiff[sfb] (the band's
#    width) instead of shortIndex[sfb] (where it starts), as mpg123 does. In bounds, but it scales
#    the wrong samples. Same function, same porting slip.

cmake_minimum_required(VERSION 3.22)

if(NOT JUCE_SOURCE_DIR)
    message(FATAL_ERROR "PatchJuce.cmake: pass -DJUCE_SOURCE_DIR=<JUCE checkout>")
endif()

set(file "${JUCE_SOURCE_DIR}/modules/juce_audio_formats/codecs/juce_MP3AudioFormat.cpp")
if(NOT EXISTS "${file}")
    message(FATAL_ERROR "PatchJuce.cmake: ${file} not found")
endif()

set(marker "Kaia patch")
set(expectedMarkers 4)

# file(READ) drops the carriage returns of CRLF files (JUCE's sources are CRLF): work out which
# line endings the file has from its size, and write the same ones back.
file(READ "${file}" text)
file(SIZE "${file}" fileSize)
string(LENGTH "${text}" textLength)
string(REGEX MATCHALL "\n" newlines "${text}")
list(LENGTH newlines numNewlines)
math(EXPR crlfSize "${textLength} + ${numNewlines}")
if(fileSize EQUAL crlfSize)
    set(lineEnding "\r\n")
elseif(fileSize EQUAL textLength)
    set(lineEnding "\n")
else()
    message(FATAL_ERROR "PatchJuce.cmake: ${file} has mixed line endings (or isn't plain text); not touching it")
endif()

string(REGEX MATCHALL "${marker}" found "${text}")
list(LENGTH found numFound)

if(numFound EQUAL expectedMarkers)
    message(STATUS "JUCE: Kaia's MP3 fixes are applied")
    return()
endif()

if(CHECK_ONLY)
    message(WARNING "The JUCE checkout in ${JUCE_SOURCE_DIR} doesn't have Kaia's MP3 decoder fixes "
                    "(an out-of-bounds write on corrupt MP3 files). Apply them with:\n"
                    "  cmake -DJUCE_SOURCE_DIR=${JUCE_SOURCE_DIR} -P ${CMAKE_CURRENT_LIST_FILE}")
    return()
endif()

if(NOT numFound EQUAL 0)
    message(FATAL_ERROR "PatchJuce.cmake: ${file} is partly patched (${numFound} of ${expectedMarkers} markers); "
                        "restore the original file and configure again")
endif()

# Check that the code still looks the way the fixes expect
string(REGEX MATCHALL "int index = bi\\.longIndex\\[maxBandl\\]" longStarts "${text}")
string(REGEX MATCHALL "auto index = \\(uint32\\) sb \\+ lwin" shortStarts "${text}")
list(LENGTH longStarts numLong)
list(LENGTH shortStarts numShort)
if(NOT numLong EQUAL 2 OR NOT numShort EQUAL 2)
    message(FATAL_ERROR "PatchJuce.cmake: the MP3 decoder in this JUCE version doesn't look like the one the "
                        "fixes were written for (found ${numLong}/2 long-block and ${numShort}/2 short-block "
                        "starts). Check whether JUCE fixed doIStereo itself, then update or remove the fixes.")
endif()

# 1. Long blocks: no band 22
string(REGEX REPLACE "int index = bi\\.longIndex\\[maxBandl\\]"
       "if (maxBandl > 21) return; /* Kaia patch: mpg123 CVE-2006-1655 fix, band 22 starts past the end */ int index = bi.longIndex[maxBandl]"
       text "${text}")

# 2. Short blocks: start each band where it starts
string(REGEX REPLACE "(shortDiff\\[sfb\\][^\n]*\n[^\n]*)\\(uint32\\) sb \\+ lwin"
       "\\1(uint32) bi.shortIndex[sfb] + lwin /* Kaia patch: band start, as in mpg123 */"
       text "${text}")
string(REGEX REPLACE "(shortDiff\\[12\\][^\n]*\n[^\n]*)\\(uint32\\) sb \\+ lwin"
       "\\1(uint32) bi.shortIndex[12] + lwin /* Kaia patch: band start, as in mpg123 */"
       text "${text}")

string(REGEX MATCHALL "${marker}" found "${text}")
list(LENGTH found numFound)
if(NOT numFound EQUAL expectedMarkers)
    message(FATAL_ERROR "PatchJuce.cmake: applied ${numFound} of ${expectedMarkers} fixes to ${file}; nothing written")
endif()

if(lineEnding STREQUAL "\r\n")
    string(REPLACE "\n" "\r\n" text "${text}")
endif()
file(WRITE "${file}" "${text}")
message(STATUS "JUCE: applied Kaia's MP3 fixes to ${file}")
