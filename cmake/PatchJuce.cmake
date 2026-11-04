# Kaia's fixes to the JUCE sources, applied after CMake fetches JUCE (PATCH_COMMAND in
# CMakeLists.txt, and again at every configure, which is a no-op once they're in). Run by hand for
# a local checkout (KAIA_JUCE_DIR):
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
# juce_MP3AudioFormat.cpp:
# 1. MP3 intensity stereo, long blocks (found by Tests/Fuzz): JUCE's decoder is a port of mpg123
#    from before mpg123's fix for CVE-2006-1655. A corrupt frame can make maxBandl 22, whose band
#    starts at index 576, one past the end of the channel, and the last band's loop then writes
#    beyond the right channel's buffer. mpg123 returns early when maxbandl > 21; so does this.
# 2. MP3 intensity stereo, short blocks: JUCE starts each band at shortDiff[sfb] (the band's
#    width) instead of shortIndex[sfb] (where it starts), as mpg123 does. In bounds, but it scales
#    the wrong samples. Same function, same porting slip.
#
# juce_Windowing_linux.cpp:
# 3. Mouse cursor: on X11 JUCE shows MouseCursor::NormalCursor by setting the window's cursor to
#    None, which means "use the parent window's cursor". An embedded plugin editor then shows
#    whatever the host last set on its own window: in Reaper, entered from the side, the resize
#    arrow of the window edge. The fix defines the standard arrow (XC_left_ptr, themed by
#    Xcursor like any other X cursor) on the window instead; ParentCursor still inherits.
#    The cursor is freed right after XDefineCursor (the window keeps it), so nothing outlives the
#    display. PluginEditor forces a cursor update on mouse enter so the arrow is set even when
#    JUCE thinks the normal cursor is already showing.

cmake_minimum_required(VERSION 3.22)

if(NOT JUCE_SOURCE_DIR)
    message(FATAL_ERROR "PatchJuce.cmake: pass -DJUCE_SOURCE_DIR=<JUCE checkout>")
endif()

set(marker "Kaia patch")

# Reads ${file} into `text`, sets `lineEnding` and `numFound` (markers already in it).
# file(READ) drops the carriage returns of CRLF files (JUCE's sources are CRLF): work out which
# line endings the file has from its size, and write the same ones back.
macro(kaia_read_source file)
    if(NOT EXISTS "${file}")
        message(FATAL_ERROR "PatchJuce.cmake: ${file} not found")
    endif()
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
endmacro()

# Returns from the script's current file section early: sets `skip` when the file is done
# (or only checked), stops on a partly patched file.
macro(kaia_check_state file expectedMarkers what)
    set(skip OFF)
    if(numFound EQUAL ${expectedMarkers})
        message(STATUS "JUCE: Kaia's ${what} fixes are applied")
        set(skip ON)
    elseif(CHECK_ONLY)
        message(WARNING "The JUCE checkout in ${JUCE_SOURCE_DIR} doesn't have Kaia's ${what} fixes. Apply them with:\n"
                        "  cmake -DJUCE_SOURCE_DIR=${JUCE_SOURCE_DIR} -P ${CMAKE_CURRENT_LIST_FILE}")
        set(skip ON)
    elseif(NOT numFound EQUAL 0)
        message(FATAL_ERROR "PatchJuce.cmake: ${file} is partly patched (${numFound} of ${expectedMarkers} markers); "
                            "restore the original file and configure again")
    endif()
endmacro()

macro(kaia_write_source file expectedMarkers what)
    string(REGEX MATCHALL "${marker}" found "${text}")
    list(LENGTH found numFound)
    if(NOT numFound EQUAL ${expectedMarkers})
        message(FATAL_ERROR "PatchJuce.cmake: applied ${numFound} of ${expectedMarkers} fixes to ${file}; nothing written")
    endif()
    if(lineEnding STREQUAL "\r\n")
        string(REPLACE "\n" "\r\n" text "${text}")
    endif()
    file(WRITE "${file}" "${text}")
    message(STATUS "JUCE: applied Kaia's ${what} fixes to ${file}")
endmacro()

#-------------------------------------------------------------------------------------------------
# MP3 decoder (fixes 1 and 2)
set(file "${JUCE_SOURCE_DIR}/modules/juce_audio_formats/codecs/juce_MP3AudioFormat.cpp")
kaia_read_source("${file}")
kaia_check_state("${file}" 4 "MP3 decoder")
if(NOT skip)
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

    kaia_write_source("${file}" 4 "MP3 decoder")
endif()

#-------------------------------------------------------------------------------------------------
# X11 normal cursor (fix 3)
set(file "${JUCE_SOURCE_DIR}/modules/juce_gui_basics/native/juce_Windowing_linux.cpp")
kaia_read_source("${file}")
kaia_check_state("${file}" 1 "X11 cursor")
if(NOT skip)
    set(cursorLine "const auto cursor = handle != nullptr \\? handle->cursorHandle : Cursor[{][}]")  # no ";": MATCHALL would split the match there
    string(REGEX MATCHALL "${cursorLine}" cursorLines "${text}")
    list(LENGTH cursorLines numCursorLines)
    if(NOT numCursorLines EQUAL 1)
        message(FATAL_ERROR "PatchJuce.cmake: MouseCursor::PlatformSpecificHandle::showInWindow in this JUCE version "
                            "doesn't look like the one the fix was written for. Check whether JUCE still shows "
                            "NormalCursor as None (the parent's cursor), then update or remove the fix.")
    endif()

    string(REGEX REPLACE "${cursorLine}"
           "if (handle == nullptr && peer != nullptr) if (auto* d = XWindowSystem::getInstance()->getDisplay()) { /* Kaia patch: NormalCursor is the arrow, not the parent window's cursor */ XWindowSystemUtilities::ScopedXLock xLock; auto* x = X11Symbols::getInstance(); const auto arrow = x->xCreateFontCursor (d, XC_left_ptr); x->xDefineCursor (d, (::Window) peer->getNativeHandle(), arrow); x->xFreeCursor (d, arrow); return; } const auto cursor = handle != nullptr ? handle->cursorHandle : Cursor{}"
           text "${text}")

    kaia_write_source("${file}" 1 "X11 cursor")
endif()
