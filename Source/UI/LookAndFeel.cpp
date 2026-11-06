#include "LookAndFeel.h"
#include "../Model/UserFiles.h"
#include <KaiaFonts.h>

namespace kaia
{

namespace
{
    // Jost (the font Xythos uses), embedded in three weights, and IBM Plex Mono for the Phosphor theme
    struct UiTypefaces
    {
        static juce::Typeface::Ptr load (const char* data, int size)
        {
            return juce::Typeface::createSystemTypefaceFor (data, (size_t) size);
        }

        juce::Typeface::Ptr light   = load (KaiaFontData::JostLight_ttf, KaiaFontData::JostLight_ttfSize);
        juce::Typeface::Ptr regular = load (KaiaFontData::JostRegular_ttf, KaiaFontData::JostRegular_ttfSize);
        juce::Typeface::Ptr bold    = load (KaiaFontData::JostBold_ttf, KaiaFontData::JostBold_ttfSize);
        juce::Typeface::Ptr monoLight   = load (KaiaFontData::IBMPlexMonoLight_ttf, KaiaFontData::IBMPlexMonoLight_ttfSize);
        juce::Typeface::Ptr monoRegular = load (KaiaFontData::IBMPlexMonoRegular_ttf, KaiaFontData::IBMPlexMonoRegular_ttfSize);
        juce::Typeface::Ptr monoBold    = load (KaiaFontData::IBMPlexMonoSemiBold_ttf, KaiaFontData::IBMPlexMonoSemiBold_ttfSize);
    };

    // Jost has a small x-height for its height, so it's drawn larger than the sizes the layout
    // names (which were chosen for the system sans-serif). Large plain text uses Light, smaller
    // text Regular so it stays readable. Plex Mono is wider: at 1.08 its text takes about the room
    // Jost's does.
    constexpr float kFontScale = 1.4f;
    constexpr float kMonoFontScale = 1.08f;
    constexpr float kLightFromHeight = 24.0f;

    bool monoFont = false;    // the current theme's (message thread only)
}

juce::Font uiFont (float height, bool bold)
{
    juce::SharedResourcePointer<UiTypefaces> faces;
    auto size = height * (monoFont ? kMonoFontScale : kFontScale);
    auto light = size >= kLightFromHeight;
    auto& face = monoFont ? (bold ? faces->monoBold : light ? faces->monoLight : faces->monoRegular)
                          : (bold ? faces->bold : light ? faces->light : faces->regular);
    return juce::Font (juce::FontOptions (face).withHeight (size));
}

//==============================================================================
const std::vector<Theme>& themes()
{
    //                  background  tint        text        textDim     inkA        inkB        inkC        glowA       glowB       dominant
    static const std::vector<Theme> list {
        { "Default",    0xff0a0b0d, 0,          0xffe6e7ea, 0xff858a94, 0xff3d8bff, 0xff3d8bff, 0xff3d8bff, 0,          0,          0xff3d8bff },
        { "Iris",       0xff09080c, 0xffffffff, 0xffe6e2ee, 0xff8e889c, 0xffd59ae0, 0xffa85eea, 0xff7d84ff, 0x12a85eea, 0x0d7278fe, 0xffa85eea },
        { "Ember",      0xff0c0807, 0xffffdcc8, 0xfff0e2d8, 0xff9c8a80, 0xffffb36b, 0xffff6a3d, 0xffe0336f, 0x14ff6e28, 0x0dc8285a, 0xffff6a3d },
        { "Abyss",      0xff05090d, 0xffa0e6ff, 0xffd8eaf0, 0xff7f99a5, 0xff7ff5e0, 0xff2ec8e8, 0xff3a7bff, 0x1200c8dc, 0x0f285aff, 0xff2ec8e8 },
        { "Phosphor",   0xff030603, 0xff5aff8c, 0xff9cf0b0, 0xff5f9c6c, 0xffb8ffc8, 0xff4dff88, 0xff22d36a, 0x123cff78, 0,          0xff4dff88, true, true },
        { "Synthwave",  0xff0a0614, 0xffff78dc, 0xffece2fa, 0xff9486ab, 0xffff4fd8, 0xffb65cff, 0xff29e0ff, 0x1cff3caa, 0x1400dcff, 0xffb65cff, false, false, true },
        { "Gold Leaf",  0xff0a0907, 0xffffe6b4, 0xffece4d4, 0xff948a76, 0xfff6e2b3, 0xffd4af6a, 0xffa77c3c, 0x0fe6be6e, 0,          0xffd4af6a },
        { "Aurora",     0xff050a09, 0xffaaffdc, 0xffdcefe8, 0xff82988f, 0xff8dffc3, 0xff2fd6c3, 0xff9a6bff, 0x123cffaa, 0x14965aff, 0xff2fd6c3 },
        { "Blood Moon", 0xff0b0506, 0xffffaab4, 0xffeedcde, 0xff9b8285, 0xffff9aa4, 0xffe0263f, 0xffa3123a, 0x1ac81428, 0x0f780028, 0xffe0263f },
        { "Glacier",    0xff0a0d11, 0xffc8e1ff, 0xffe6eef7, 0xff8796a8, 0xffffffff, 0xffcfe6ff, 0xff8fbaf0, 0x0faad2ff, 0x08ffffff, 0xff8fbaf0 },
        { "Graphite",   0xff0b0b0b, 0xffffffff, 0xffe8e8e8, 0xff8e8e8e, 0xffffffff, 0xffd0d0d0, 0xff8e8e8e, 0x08ffffff, 0,          0xffa0a0a0 },
        { "Oxide",      0xff0b0908, 0xffffd2b4, 0xffeee2d8, 0xff978a80, 0xfff0a46c, 0xffc8693c, 0xff4fc2a6, 0x14d27846, 0x0f3cbea0, 0xffc8693c },
    };
    return list;
}

int savedTheme()
{
    juce::SharedResourcePointer<SharedSettings> settings;
    auto name = settings->get().getValue ("theme");
    auto& list = themes();
    for (int i = 0; i < (int) list.size(); ++i)
        if (name == list[(size_t) i].name)
            return i;
    return 0;
}

bool savedTintPads()
{
    juce::SharedResourcePointer<SharedSettings> settings;
    return settings->get().getBoolValue ("tintPads", false);
}

void applyTheme (int index, bool tint)
{
    auto& list = themes();
    index = juce::jlimit (0, (int) list.size() - 1, index);
    auto& t = list[(size_t) index];
    Colours::theme = index;
    Colours::tintPads = tint;
    Colours::gradient = index != 0;
    Colours::text = juce::Colour (t.text);
    Colours::textDim = juce::Colour (t.textDim);
    Colours::background = juce::Colour (t.background);
    monoFont = t.mono;

    if (index == 0)
    {
        // Kaia's original colours
        Colours::panel       = juce::Colour (0xff111316);
        Colours::panelRaised = juce::Colour (0xff1a1d21);
        Colours::outline     = juce::Colour (0xff272a30);
        Colours::padEmpty    = juce::Colour (0xff131518);
        Colours::padLoaded   = juce::Colour (0xff1c1f24);
        auto accent = juce::Colour (t.inkA);
        Colours::accent = Colours::inkA = Colours::inkB = Colours::inkC = Colours::dominant = accent;
        Colours::accentDim = accent.withMultipliedBrightness (0.5f).withMultipliedSaturation (0.8f);
        return;
    }

    // Panels are opaque (they nest); pads are translucent so the window's glow shows through them
    auto bg = juce::Colour (t.background), tintColour = juce::Colour (t.tint);
    Colours::panel       = bg.interpolatedWith (tintColour, 0.03f);
    Colours::panelRaised = bg.interpolatedWith (tintColour, 0.07f);
    Colours::outline     = bg.interpolatedWith (tintColour, 0.12f);
    Colours::padEmpty    = tintColour.withAlpha (0.018f);
    Colours::padLoaded   = tintColour.withAlpha (0.038f);
    Colours::inkA = juce::Colour (t.inkA);
    Colours::inkB = juce::Colour (t.inkB);
    Colours::inkC = juce::Colour (t.inkC);
    Colours::accent = Colours::inkB;
    Colours::accentDim = Colours::inkB.withMultipliedBrightness (0.45f);
    Colours::dominant = juce::Colour (t.dominant);
}

juce::Colour inkAt (float position)
{
    position = juce::jlimit (0.0f, 1.0f, position);
    return position < 0.55f ? Colours::inkA.interpolatedWith (Colours::inkB, position / 0.55f)
                            : Colours::inkB.interpolatedWith (Colours::inkC, (position - 0.55f) / 0.45f);
}

juce::ColourGradient inkGradient (juce::Rectangle<float> area, float alpha)
{
    // Slightly tilted, like the CSS 100deg of the mockups
    juce::ColourGradient grad (Colours::inkA.withMultipliedAlpha (alpha), area.getX(), area.getY(),
                               Colours::inkC.withMultipliedAlpha (alpha), area.getRight(), area.getY() + area.getHeight() * 0.35f, false);
    grad.addColour (0.55, Colours::inkB.withMultipliedAlpha (alpha));
    return grad;
}

void setInk (juce::Graphics& g, juce::Rectangle<float> area, float alpha)
{
    if (Colours::gradient && area.getWidth() > 0.5f)
        g.setGradientFill (inkGradient (area, alpha));
    else
        g.setColour (Colours::accent.withMultipliedAlpha (alpha));
}

void drawInkText (juce::Graphics& g, const juce::String& text, juce::Rectangle<int> area,
                  juce::Justification justification, bool fitted)
{
    auto width = (float) juce::jmin (area.getWidth(), juce::GlyphArrangement::getStringWidthInt (g.getCurrentFont(), text));
    auto x = (float) area.getX();
    if (justification.testFlags (juce::Justification::horizontallyCentred))
        x += ((float) area.getWidth() - width) * 0.5f;
    else if (justification.testFlags (juce::Justification::right))
        x += (float) area.getWidth() - width;
    setInk (g, { x, (float) area.getY(), juce::jmax (1.0f, width), (float) area.getHeight() });
    if (fitted)
        g.drawFittedText (text, area, justification, 1);
    else
        g.drawText (text, area, justification, true);
}

void fillPadTint (juce::Graphics& g, juce::Rectangle<float> r, float cornerSize, bool loaded)
{
    if (! Colours::tintPads)
        return;
    g.setColour (Colours::dominant.withAlpha (loaded ? 0.13f : 0.06f));
    g.fillRoundedRectangle (r, cornerSize);
}

void drawInkOutline (juce::Graphics& g, juce::Rectangle<float> r, float cornerSize, float thickness)
{
    if (Colours::theme >= 0 && themes()[(size_t) Colours::theme].neon)
    {
        // Synthwave: a soft glow around highlighted outlines
        g.setColour (Colours::inkA.withAlpha (0.10f));
        g.drawRoundedRectangle (r.expanded (1.5f), cornerSize + 1.5f, 3.0f);
        g.setColour (Colours::inkB.withAlpha (0.06f));
        g.drawRoundedRectangle (r.expanded (3.0f), cornerSize + 3.0f, 3.0f);
    }
    setInk (g, r);
    g.drawRoundedRectangle (r, cornerSize, thickness);
}

void paintPanel (juce::Graphics& g, juce::Rectangle<float> bounds)
{
    g.setColour (Colours::panel);
    g.fillRoundedRectangle (bounds, 8.0f);
    if (Colours::gradient)
    {
        g.setColour (Colours::outline);
        g.drawRoundedRectangle (bounds.reduced (0.5f), 8.0f, 1.0f);
    }
}

ThemeSnapshot ThemeSnapshot::current()
{
    return { Colours::background, Colours::panel, Colours::panelRaised, Colours::outline, Colours::text,
             Colours::textDim, Colours::padEmpty, Colours::padLoaded, Colours::accent };
}

void restyleComponents (juce::Component& root, const ThemeSnapshot& before)
{
    auto after = ThemeSnapshot::current();
    const std::pair<juce::Colour, juce::Colour> swaps[] = {
        { before.text, after.text }, { before.textDim, after.textDim }, { before.accent, after.accent },
        { before.panel, after.panel }, { before.panelRaised, after.panelRaised }, { before.outline, after.outline },
        { before.background, after.background }, { before.padEmpty, after.padEmpty }, { before.padLoaded, after.padLoaded },
    };

    // Only fonts from uiFont() (Jost or Plex Mono); JUCE's defaults go through getTypefaceForFont anyway
    auto isUiFont = [] (const juce::Font& f)
    {
        return f.getTypefaceName().containsIgnoreCase ("Jost") || f.getTypefaceName().containsIgnoreCase ("Plex Mono");
    };
    auto restyleFont = [] (const juce::Font& f)
    {
        auto mono = f.getTypefaceName().containsIgnoreCase ("Mono");
        auto layoutHeight = f.getHeight() / (mono ? kMonoFontScale : kFontScale);
        auto bold = f.getTypefaceStyle().containsIgnoreCase ("Bold");
        return uiFont (layoutHeight, bold);
    };

    std::function<void (juce::Component&)> visit = [&] (juce::Component& c)
    {
        // Colours set with setColour live in the component's properties as "jcclr_<id>"
        auto& props = c.getProperties();
        for (int i = 0; i < props.size(); ++i)
        {
            auto name = props.getName (i).toString();
            if (! name.startsWith ("jcclr_"))
                continue;
            auto colour = juce::Colour ((juce::uint32) (int) props.getValueAt (i));
            for (auto& [from, to] : swaps)
            {
                if (colour.withAlpha (1.0f) == from.withAlpha (1.0f))
                {
                    auto alpha = from.getFloatAlpha() > 0.0f ? colour.getFloatAlpha() / from.getFloatAlpha() : 1.0f;
                    c.setColour (name.substring (6).getHexValue32(), to.withMultipliedAlpha (alpha));
                    break;
                }
            }
        }

        if (auto* label = dynamic_cast<juce::Label*> (&c))
        {
            if (isUiFont (label->getFont()))
                label->setFont (restyleFont (label->getFont()));
        }
        else if (auto* editor = dynamic_cast<juce::TextEditor*> (&c))
        {
            if (isUiFont (editor->getFont()))
                editor->applyFontToAllText (restyleFont (editor->getFont()));
        }

        for (auto* child : c.getChildren())
            visit (*child);
        c.repaint();
    };
    visit (root);
}

KaiaLookAndFeel::KaiaLookAndFeel()
{
    applyTheme (savedTheme(), savedTintPads());
    applyColours();
}

void KaiaLookAndFeel::applyColours()
{
    setColour (juce::ResizableWindow::backgroundColourId, Colours::background);
    setColour (juce::Label::textColourId, Colours::text);
    setColour (juce::TextButton::buttonColourId, Colours::panelRaised);
    setColour (juce::TextButton::buttonOnColourId, Colours::accent);
    setColour (juce::TextButton::textColourOffId, Colours::text);
    // Toggled buttons: filled with the accent in the Default theme; outlined in the gradient in the others
    setColour (juce::TextButton::textColourOnId, Colours::gradient ? Colours::text : juce::Colours::black);
    setColour (juce::ComboBox::backgroundColourId, Colours::panelRaised);
    setColour (juce::ComboBox::textColourId, Colours::text);
    setColour (juce::ComboBox::outlineColourId, Colours::outline);
    setColour (juce::ComboBox::arrowColourId, Colours::textDim);
    setColour (juce::PopupMenu::backgroundColourId, Colours::panelRaised);
    setColour (juce::PopupMenu::textColourId, Colours::text);
    setColour (juce::PopupMenu::headerTextColourId, Colours::textDim);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, Colours::accent.withAlpha (Colours::gradient ? 0.22f : 0.85f));
    setColour (juce::PopupMenu::highlightedTextColourId, Colours::gradient ? Colours::text : juce::Colours::black);
    setColour (juce::Slider::textBoxTextColourId, Colours::text);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxHighlightColourId, Colours::accent.withAlpha (0.4f));
    setColour (juce::TextEditor::backgroundColourId, Colours::panelRaised);
    setColour (juce::TextEditor::textColourId, Colours::text);
    setColour (juce::TextEditor::outlineColourId, Colours::outline);
    setColour (juce::TextEditor::focusedOutlineColourId, Colours::accent);
    setColour (juce::TextEditor::highlightColourId, Colours::accent.withAlpha (0.35f));
    setColour (juce::CaretComponent::caretColourId, Colours::accent);
    setColour (juce::ListBox::backgroundColourId, Colours::panel);
    setColour (juce::ScrollBar::thumbColourId, Colours::outline.brighter (0.3f));
    setColour (juce::TooltipWindow::backgroundColourId, Colours::panelRaised);
    setColour (juce::TooltipWindow::textColourId, Colours::text);
    setColour (juce::ToggleButton::textColourId, Colours::text);
    setColour (juce::ToggleButton::tickColourId, Colours::accent);
    setColour (juce::AlertWindow::backgroundColourId, Colours::panel);
    setColour (juce::AlertWindow::textColourId, Colours::text);
    setColour (juce::AlertWindow::outlineColourId, Colours::outline);
    setColour (juce::ProgressBar::backgroundColourId, Colours::panelRaised);
    setColour (juce::ProgressBar::foregroundColourId, Colours::accent);
}

void KaiaLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float pos,
                                            float startAngle, float endAngle, juce::Slider& slider)
{
    auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (3.0f);
    auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) / 2.0f;
    auto centre = bounds.getCentre();
    auto enabled = slider.isEnabled();

    // Bipolar knobs (pan, tune) fill from the centre
    auto isBipolar = slider.getMinimum() < 0.0 && slider.getMaximum() > 0.0;
    auto fromAngle = isBipolar ? (startAngle + endAngle) * 0.5f : startAngle;
    auto toAngle = startAngle + pos * (endAngle - startAngle);

    if (Colours::gradient)
    {
        // A ring of ticks lit in the gradient up to the value, a dark knob and a dot for the position
        const int ticks = radius > 26.0f ? 31 : 25;
        auto r1 = radius * 0.78f, r2 = radius;
        auto lo = juce::jmin (fromAngle, toAngle), hi = juce::jmax (fromAngle, toAngle);
        auto lineW = juce::jmax (1.2f, radius * 0.075f);
        for (int i = 0; i < ticks; ++i)
        {
            auto t = (float) i / (float) (ticks - 1);
            auto angle = startAngle + t * (endAngle - startAngle);
            auto step = (endAngle - startAngle) / (float) (ticks - 1);
            bool lit = angle >= lo - step * 0.5f && angle <= hi + step * 0.5f && (isBipolar || pos > 0.0f || i == 0);
            g.setColour (! lit ? Colours::outline.brighter (0.25f)
                               : enabled ? inkAt (t) : Colours::textDim);
            auto dx = std::sin (angle), dy = -std::cos (angle);
            g.drawLine (centre.x + dx * r1, centre.y + dy * r1, centre.x + dx * r2, centre.y + dy * r2, lit ? lineW * 1.2f : lineW);
        }
        auto knobR = radius * 0.56f;
        g.setColour (Colours::panelRaised);
        g.fillEllipse (centre.x - knobR, centre.y - knobR, knobR * 2, knobR * 2);
        g.setColour (Colours::outline.brighter (0.15f));
        g.drawEllipse (centre.x - knobR, centre.y - knobR, knobR * 2, knobR * 2, 1.0f);
        auto dotR = juce::jmax (1.6f, radius * 0.09f), dotAt = knobR * 0.62f;
        g.setColour (enabled ? Colours::text : Colours::textDim);
        g.fillEllipse (centre.x + std::sin (toAngle) * dotAt - dotR, centre.y - std::cos (toAngle) * dotAt - dotR, dotR * 2, dotR * 2);
        return;
    }

    auto lineW = juce::jmax (2.0f, radius * 0.16f);
    auto arcRadius = radius - lineW * 0.5f;

    juce::Path track;
    track.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f, startAngle, endAngle, true);
    g.setColour (Colours::outline);
    g.strokePath (track, juce::PathStrokeType (lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path value;
    value.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f, juce::jmin (fromAngle, toAngle), juce::jmax (fromAngle, toAngle), true);
    g.setColour (enabled ? Colours::accent : Colours::textDim);
    g.strokePath (value, juce::PathStrokeType (lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    auto knobR = arcRadius - lineW * 1.4f;
    g.setColour (Colours::panelRaised);
    g.fillEllipse (centre.x - knobR, centre.y - knobR, knobR * 2, knobR * 2);

    juce::Path pointer;
    auto pLen = knobR * 0.75f;
    pointer.addRoundedRectangle (-1.25f, -knobR + 2.0f, 2.5f, pLen * 0.55f, 1.0f);
    g.setColour (enabled ? Colours::text : Colours::textDim);
    g.fillPath (pointer, juce::AffineTransform::rotation (toAngle).translated (centre));
}

void KaiaLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                                            float minSliderPos, float maxSliderPos, juce::Slider::SliderStyle style, juce::Slider& slider)
{
    if (style == juce::Slider::LinearBar || style == juce::Slider::LinearBarVertical)
    {
        auto r = juce::Rectangle<int> (x, y, width, height).toFloat();
        g.setColour (Colours::panelRaised);
        g.fillRoundedRectangle (r, 3.0f);
        auto isBipolar = slider.getMinimum() < 0.0 && slider.getMaximum() > 0.0;
        juce::Rectangle<float> fill;
        if (isBipolar)
        {
            auto zero = (float) slider.getPositionOfValue (0.0);
            fill = juce::Rectangle<float>::leftTopRightBottom (juce::jmin (zero, sliderPos), r.getY(), juce::jmax (zero, sliderPos), r.getBottom());
        }
        else
        {
            fill = r.withRight (sliderPos);
        }
        setInk (g, r, slider.isEnabled() ? 0.35f : 0.15f);
        g.fillRoundedRectangle (fill, 3.0f);
        return;
    }
    LookAndFeel_V4::drawLinearSlider (g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, style, slider);
}

void KaiaLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour& backgroundColour,
                                                bool isMouseOverButton, bool isButtonDown)
{
    auto r = button.getLocalBounds().toFloat().reduced (0.5f);
    auto on = button.getToggleState();
    // JUCE passes buttonOnColourId for toggled buttons; gradient themes keep the plain fill and outline them
    auto c = on && Colours::gradient ? button.findColour (juce::TextButton::buttonColourId)
           : on ? Colours::accent : backgroundColour;
    if (isButtonDown)
        c = c.brighter (0.15f);
    else if (isMouseOverButton)
        c = c.brighter (0.07f);
    if (! button.isEnabled())
        c = c.withAlpha (0.5f);
    g.setColour (c);
    g.fillRoundedRectangle (r, 4.0f);

    if (on && Colours::gradient)
    {
        drawInkOutline (g, r, 4.0f);
        return;
    }
    g.setColour (Colours::outline.withAlpha (on ? 0.0f : 1.0f));
    g.drawRoundedRectangle (r, 4.0f, 1.0f);
}

void KaiaLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& button, bool over, bool down)
{
    if (! (button.getToggleState() && Colours::gradient && button.isEnabled()))
    {
        LookAndFeel_V4::drawButtonText (g, button, over, down);
        return;
    }
    g.setFont (getTextButtonFont (button, button.getHeight()));
    auto inset = juce::jmin (4, button.proportionOfHeight (0.3f));
    drawInkText (g, button.getButtonText(), button.getLocalBounds().reduced (inset + 2, inset), juce::Justification::centred, true);
}

void KaiaLookAndFeel::drawLabel (juce::Graphics& g, juce::Label& label)
{
    if (! label.getProperties().getWithDefault (inkLabelProperty, false) || label.isBeingEdited())
    {
        LookAndFeel_V4::drawLabel (g, label);
        return;
    }
    g.setFont (getLabelFont (label));
    auto area = getLabelBorderSize (label).subtractedFrom (label.getLocalBounds());
    drawInkText (g, label.getText(), area, label.getJustificationType(), true);
}

void KaiaLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button, bool highlighted, bool)
{
    auto r = button.getLocalBounds().toFloat();
    auto box = r.removeFromLeft (juce::jmin (r.getHeight(), 18.0f)).reduced (2.0f).withSizeKeepingCentre (14.0f, 14.0f);
    if (button.getToggleState())
        setInk (g, box);
    else
        g.setColour (Colours::panelRaised);
    g.fillRoundedRectangle (box, 3.0f);
    g.setColour (highlighted ? Colours::textDim : Colours::outline);
    if (! button.getToggleState() || ! Colours::gradient)
        g.drawRoundedRectangle (box, 3.0f, 1.0f);
    if (button.getToggleState())
    {
        juce::Path tick;
        tick.startNewSubPath (box.getX() + 3.5f, box.getCentreY());
        tick.lineTo (box.getX() + 6.0f, box.getBottom() - 3.5f);
        tick.lineTo (box.getRight() - 3.0f, box.getY() + 3.5f);
        g.setColour (Colours::gradient ? Colours::background : juce::Colours::black);
        g.strokePath (tick, juce::PathStrokeType (1.8f));
    }
    g.setColour (button.isEnabled() ? Colours::text : Colours::textDim);
    g.setFont (uiFont (13.0f));
    g.drawFittedText (button.getButtonText(), r.toNearestInt().withTrimmedLeft (4), juce::Justification::centredLeft, 1);
}

void KaiaLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool, int, int, int, int, juce::ComboBox& box)
{
    auto r = juce::Rectangle<float> (0, 0, (float) width, (float) height).reduced (0.5f);
    g.setColour (box.findColour (juce::ComboBox::backgroundColourId).withAlpha (box.isEnabled() ? 1.0f : 0.5f));
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (box.hasKeyboardFocus (false) ? Colours::accent : Colours::outline);
    g.drawRoundedRectangle (r, 4.0f, 1.0f);

    juce::Path arrow;
    auto ax = (float) width - 14.0f, ay = (float) height * 0.5f;
    arrow.addTriangle (ax - 4.0f, ay - 2.0f, ax + 4.0f, ay - 2.0f, ax, ay + 3.0f);
    g.setColour (Colours::textDim);
    g.fillPath (arrow);
}

juce::Font KaiaLookAndFeel::getComboBoxFont (juce::ComboBox&)               { return uiFont (13.5f); }
juce::Font KaiaLookAndFeel::getTextButtonFont (juce::TextButton&, int h)    { return uiFont (juce::jmin (14.0f, (float) h * 0.55f)); }
juce::Font KaiaLookAndFeel::getLabelFont (juce::Label& l)                   { return l.getFont(); }
juce::Font KaiaLookAndFeel::getPopupMenuFont()                              { return uiFont (14.0f); }
juce::Font KaiaLookAndFeel::getAlertWindowTitleFont()                       { return uiFont (16.0f, true); }
juce::Font KaiaLookAndFeel::getAlertWindowMessageFont()                     { return uiFont (14.0f); }
juce::Font KaiaLookAndFeel::getAlertWindowFont()                            { return uiFont (13.0f); }

juce::Typeface::Ptr KaiaLookAndFeel::getTypefaceForFont (const juce::Font& font)
{
    // Fonts JUCE makes itself (text editors, menus, dialogs) use Jost too
    if (font.getTypefaceName() == juce::Font::getDefaultSansSerifFontName())
        return uiFont (font.getHeight(), font.isBold()).getTypefacePtr();
    return LookAndFeel_V4::getTypefaceForFont (font);
}

void KaiaLookAndFeel::drawTreeviewPlusMinusBox (juce::Graphics& g, const juce::Rectangle<float>& area, juce::Colour,
                                                bool isOpen, bool isMouseOver)
{
    // A large triangle: the whole indent column is the button, so it's easy to hit
    auto c = area.getCentre();
    auto s = juce::jmin (area.getWidth(), area.getHeight()) * 0.2f;
    juce::Path p;
    if (isOpen)
        p.addTriangle (c.x - s, c.y - s * 0.5f, c.x + s, c.y - s * 0.5f, c.x, c.y + s * 0.65f);
    else
        p.addTriangle (c.x - s * 0.5f, c.y - s, c.x - s * 0.5f, c.y + s, c.x + s * 0.65f, c.y);
    g.setColour (isMouseOver ? Colours::accent : Colours::textDim);
    g.fillPath (p);
}

void KaiaLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    label.setBounds (6, 1, box.getWidth() - 24, box.getHeight() - 2);
    label.setFont (getComboBoxFont (box));
}

void KaiaLookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int width, int height)
{
    g.fillAll (Colours::panelRaised);
    g.setColour (Colours::outline);
    g.drawRect (0, 0, width, height);
}

void KaiaLookAndFeel::drawTooltip (juce::Graphics& g, const juce::String& text, int width, int height)
{
    auto r = juce::Rectangle<int> (width, height).toFloat();
    g.setColour (Colours::panelRaised);
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (Colours::outline);
    g.drawRoundedRectangle (r.reduced (0.5f), 4.0f, 1.0f);
    g.setColour (Colours::text);
    g.setFont (uiFont (13.0f));
    g.drawFittedText (text, r.toNearestInt().reduced (8, 4), juce::Justification::centredLeft, 6);
}

juce::Label* KaiaLookAndFeel::createSliderTextBox (juce::Slider& slider)
{
    auto* l = LookAndFeel_V4::createSliderTextBox (slider);
    l->setFont (uiFont (12.0f));
    l->setColour (juce::Label::textColourId, Colours::text);
    l->setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    l->setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);
    l->setColour (juce::Label::textWhenEditingColourId, Colours::text);
    l->setColour (juce::Label::backgroundWhenEditingColourId, Colours::panelRaised);
    l->setColour (juce::Label::outlineWhenEditingColourId, Colours::accent);
    return l;
}

}
