#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace kaia
{

namespace Colours
{
    // Theme colours: set by applyTheme() (message thread only). The values here are the Default theme's.
    inline juce::Colour background   { 0xff0a0b0d };   // near black
    inline juce::Colour panel        { 0xff111316 };
    inline juce::Colour panelRaised  { 0xff1a1d21 };
    inline juce::Colour outline      { 0xff272a30 };
    inline juce::Colour text         { 0xffe6e7ea };
    inline juce::Colour textDim      { 0xff858a94 };
    inline juce::Colour padEmpty     { 0xff131518 };
    inline juce::Colour padLoaded    { 0xff1c1f24 };
    const juce::Colour good         { 0xff58c49a };
    const juce::Colour warning      { 0xffe8b04a };
    const juce::Colour error        { 0xffe8625a };

    // The highlight colour: the user's choice in the Default theme, the gradient's middle colour in the others
    inline juce::Colour accent      { 0xff3d8bff };   // blue
    inline juce::Colour accentDim   { 0xff1f4580 };

    // Gradient themes paint highlights (text, outlines, knob ticks, the waveform) with a three-colour
    // gradient, inkA -> inkB -> inkC. In the Default theme all three are the accent.
    inline juce::Colour inkA { 0xff3d8bff }, inkB { 0xff3d8bff }, inkC { 0xff3d8bff };
    inline bool gradient = false;
    inline int theme = 0;                             // index into themes()
    inline bool tintPads = false;                     // pads tinted with the theme's dominant colour
    inline juce::Colour dominant { 0xff3d8bff };
}

// Colour themes. The first one ("Default") is Kaia's original look (blue highlights);
// the others are dark themes with a gradient for highlights (the Xythos/Modometer look and relatives).
struct Theme
{
    const char* name;
    juce::uint32 background;
    juce::uint32 tint;                                // panels, outlines and pads are the background mixed with this
    juce::uint32 text, textDim;
    juce::uint32 inkA, inkB, inkC;                    // the highlight gradient
    juce::uint32 glowA, glowB;                        // faint glows in the top right and bottom left corners (ARGB)
    juce::uint32 dominant;                            // what "Tint pads" colours the pads with
    bool mono = false;                                // IBM Plex Mono instead of Jost
    bool scanlines = false;                           // faint CRT lines over the whole window
    bool neon = false;                                // highlighted outlines get a soft glow
};

const std::vector<Theme>& themes();
int savedTheme();                                     // index into themes(), from the settings file ("theme")
bool savedTintPads();                                 // setting "tintPads", off by default
void applyTheme (int index, bool tint);   // sets the Colours

// Highlights. In gradient themes these paint with the gradient across `area` (left to right);
// in the Default theme with the accent colour.
juce::ColourGradient inkGradient (juce::Rectangle<float> area, float alpha = 1.0f);
void setInk (juce::Graphics&, juce::Rectangle<float> area, float alpha = 1.0f);
juce::Colour inkAt (float position);                  // 0..1 along the gradient
// Text in the highlight: the gradient spans the text itself, not the whole box
void drawInkText (juce::Graphics&, const juce::String& text, juce::Rectangle<int> area,
                  juce::Justification justification, bool fitted = false);
// A panel's background (pad editor, browsers, settings): gradient themes add a hairline outline
void paintPanel (juce::Graphics&, juce::Rectangle<float> bounds);
// A highlighted outline: the gradient (with Synthwave's glow), or the accent
void drawInkOutline (juce::Graphics&, juce::Rectangle<float> r, float cornerSize, float thickness = 1.0f);
// The pad tint ("Tint pads"): fills over a pad's own colour, or nothing
void fillPadTint (juce::Graphics&, juce::Rectangle<float> r, float cornerSize, bool loaded);

// After a theme change: gives every Label/TextEditor under `root` the new theme's font and swaps
// colours set on components (setColour) from the old theme's to the new one's.
struct ThemeSnapshot
{
    juce::Colour background, panel, panelRaised, outline, text, textDim, padEmpty, padLoaded, accent;
    static ThemeSnapshot current();
};
void restyleComponents (juce::Component& root, const ThemeSnapshot& before);

class KaiaLookAndFeel : public juce::LookAndFeel_V4
{
public:
    KaiaLookAndFeel();
    void applyColours();          // call after the theme or the accent colour changed

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height, float sliderPos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawLinearSlider (juce::Graphics&, int x, int y, int width, int height, float sliderPos,
                           float minSliderPos, float maxSliderPos, juce::Slider::SliderStyle, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour& backgroundColour,
                               bool isMouseOverButton, bool isButtonDown) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool isMouseOverButton, bool isButtonDown) override;
    void drawLabel (juce::Graphics&, juce::Label&) override;
    void drawComboBox (juce::Graphics&, int width, int height, bool isButtonDown, int buttonX, int buttonY,
                       int buttonW, int buttonH, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;
    juce::Font getLabelFont (juce::Label&) override;
    juce::Font getPopupMenuFont() override;
    juce::Font getAlertWindowTitleFont() override;
    juce::Font getAlertWindowMessageFont() override;
    juce::Font getAlertWindowFont() override;
    juce::Typeface::Ptr getTypefaceForFont (const juce::Font&) override;
    void drawTreeviewPlusMinusBox (juce::Graphics&, const juce::Rectangle<float>& area, juce::Colour backgroundColour,
                                   bool isOpen, bool isMouseOver) override;
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
    void drawPopupMenuBackground (juce::Graphics&, int width, int height) override;
    void drawTooltip (juce::Graphics&, const juce::String& text, int width, int height) override;
    juce::Label* createSliderTextBox (juce::Slider&) override;
    void drawLinearSliderOutline (juce::Graphics&, int, int, int, int, juce::Slider::SliderStyle, juce::Slider&) override {}
};

// The UI font (Jost; IBM Plex Mono in the Phosphor theme). Heights are layout sizes: the text is drawn
// somewhat larger (see kFontScale)
juce::Font uiFont (float height, bool bold = false);

// Labels with this property draw their text in the highlight (inkGradient)
inline const juce::Identifier inkLabelProperty { "kaiaInk" };

}
