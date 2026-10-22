#include "PluginEditor.h"

#include <cmath>

namespace Colours = juce::Colours;

using juce::Colour;
using juce::Graphics;
using juce::Rectangle;

namespace
{
constexpr float pi = juce::MathConstants<float>::pi;

juce::ColourGradient diag (Colour c0, juce::Point<float> p0, Colour c1, juce::Point<float> p1)
{
    return juce::ColourGradient (c0, p0, c1, p1, false);
}

juce::ColourGradient brassGradient (Rectangle<float> r)
{
    auto g = diag (ui::brassHi, r.getTopLeft(), ui::brassLo, r.getBottomRight());
    g.addColour (0.5, ui::brassMid);
    return g;
}

juce::ColourGradient chromeGradient (Rectangle<float> r)
{
    auto g = diag (Colour (0xfffafbfc), r.getTopLeft(), Colour (0xff5f6369), r.getBottomRight());
    g.addColour (0.5, Colour (0xffa8adb4));
    return g;
}

// text with an engraved highlight, used on the silver plate
void engraved (Graphics& g, const juce::String& text, Rectangle<int> r, juce::Font f, juce::Justification j)
{
    g.setFont (f);
    g.setColour (Colours::white.withAlpha (0.6f));
    g.drawText (text, r.translated (0, 1), j, false);
    g.setColour (ui::ink);
    g.drawText (text, r, j, false);
}
}


//==============================================================================
juce::Font ui::font (float height, bool bold)
{
    return juce::Font (juce::FontOptions (height, bold ? juce::Font::bold : juce::Font::plain));
}

juce::Font ui::display (float height)
{
    return juce::Font (juce::FontOptions ("Impact", height, juce::Font::plain)).withExtraKerningFactor (0.1f);
}

juce::Font ui::mono (float height)
{
    return juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), height, juce::Font::plain));
}

//==============================================================================
TubeLookAndFeel::TubeLookAndFeel()
{
    setColour (juce::PopupMenu::backgroundColourId, Colour (0xff1a1713));
    setColour (juce::PopupMenu::textColourId, Colour (0xffece8de));
    setColour (juce::PopupMenu::highlightedBackgroundColourId, Colour (0xff2f2819));
    setColour (juce::PopupMenu::highlightedTextColourId, ui::brassHi);
    setColour (juce::PopupMenu::headerTextColourId, ui::brassHi);
}

void TubeLookAndFeel::drawBakelite (Graphics& g, Rectangle<float> r, float radius, bool lit)
{
    juce::ColourGradient grad (lit ? ui::brassHi : Colour (0xff4f4a44), r.getX(), r.getY(),
                               lit ? ui::brassMid : Colour (0xff221f1b), r.getX(), r.getBottom(), false);
    if (! lit)
    {
        grad.addColour (0.48, Colour (0xff2a2723));
        grad.addColour (0.52, Colour (0xff14120f));
    }
    g.setGradientFill (grad);
    g.fillRoundedRectangle (r, radius);
    g.setColour (lit ? Colour (0xff4a3810) : Colours::black);
    g.drawRoundedRectangle (r, radius, 1.0f);
    g.setColour (Colours::white.withAlpha (lit ? 0.5f : 0.28f));
    g.drawLine (r.getX() + radius * 0.6f, r.getY() + 1.5f, r.getRight() - radius * 0.6f, r.getY() + 1.5f, 1.0f);
}

void TubeLookAndFeel::drawChromeFrame (Graphics& g, Rectangle<float> r, float radius)
{
    juce::ColourGradient grad (Colours::white, r.getTopLeft(), Colour (0xff4f5359), r.getBottomRight(), false);
    grad.addColour (0.22, Colour (0xffc9cdd2));
    grad.addColour (0.38, Colour (0xff7b8087));
    grad.addColour (0.52, Colour (0xffe9ebee));
    grad.addColour (0.74, Colour (0xff8e939a));
    g.setGradientFill (grad);
    g.fillRoundedRectangle (r, radius);
    g.setColour (Colour (0xff5d6066));
    g.drawRoundedRectangle (r, radius, 1.0f);
    g.setColour (Colours::white.withAlpha (0.8f));
    g.drawLine (r.getX() + radius, r.getY() + 1.5f, r.getRight() - radius, r.getY() + 1.5f, 1.0f);
}

void TubeLookAndFeel::drawScrew (Graphics& g, juce::Point<float> c, float d, float angle)
{
    auto r = Rectangle<float> (d, d).withCentre (c);
    g.setGradientFill (chromeGradient (r));
    g.fillEllipse (r);
    g.setColour (Colour (0xff5d6066));
    g.drawEllipse (r, 1.0f);
    g.setColour (Colour (0xff3a3d42));
    const auto v = juce::Point<float> (std::cos (angle), std::sin (angle)) * (d * 0.36f);
    g.drawLine (juce::Line<float> (c - v, c + v), 1.4f);
}

void TubeLookAndFeel::drawKnob (Graphics& g, Rectangle<float> b, float pos, bool chrome, juce::Colour scale, bool numerals)
{
    const float s = std::min (b.getWidth(), b.getHeight()) / 84.0f;
    const auto o = b.getCentre() - juce::Point<float> (42.0f * s, 42.0f * s);
    auto P = [&] (float x, float y) { return juce::Point<float> (o.x + x * s, o.y + y * s); };
    auto circle = [&] (float cx, float cy, float r) { return Rectangle<float> (2 * r * s, 2 * r * s).withCentre (P (cx, cy)); };
    auto polar = [&] (float aDeg, float r) { const float a = aDeg * pi / 180.0f; return P (42.0f + r * std::cos (a), 42.0f + r * std::sin (a)); };

    // printed 0-10 scale
    if (! scale.isTransparent())
    {
        g.setColour (scale);
        for (int i = 0; i <= 10; ++i)
        {
            const float a = 135.0f + 27.0f * (float) i;
            g.drawLine (juce::Line<float> (polar (a, i % 5 == 0 ? 29.0f : 31.0f), polar (a, 35.0f)), 1.2f * s + 0.3f);
        }
        if (numerals && s > 0.8f)
        {
            g.setFont (ui::font (8.0f * s, true));
            g.drawText ("0",  Rectangle<float> (14.0f * s, 8.0f * s).withCentre (P (14.0f, 72.0f)), juce::Justification::centred, false);
            g.drawText ("10", Rectangle<float> (16.0f * s, 8.0f * s).withCentre (P (70.0f, 72.0f)), juce::Justification::centred, false);
        }
    }

    // drop shadow + skirt
    g.setColour (Colours::black.withAlpha (0.35f));
    g.fillEllipse (circle (43.5f, 44.0f, 26.0f));
    auto skirt = circle (42.0f, 42.0f, 26.0f);
    g.setGradientFill (chrome ? chromeGradient (skirt) : brassGradient (skirt));
    g.fillEllipse (skirt);
    g.setColour (Colour (0xff4a3810));
    g.drawEllipse (skirt, 0.8f * s);

    // knurled edge
    juce::Path ring;
    ring.addEllipse (circle (42.0f, 42.0f, 25.0f));
    const float dashes[] = { 0.9f * s, 1.7f * s };
    juce::Path dashed;
    juce::PathStrokeType (2.0f * s).createDashedStroke (dashed, ring, dashes, 2);
    g.setColour (Colour (0x802b2008));
    g.fillPath (dashed);

    // specular sheen over the skirt
    {
        juce::ColourGradient shine (Colours::white.withAlpha (0.7f), P (31.6f, 29.0f), Colours::black.withAlpha (0.4f), P (31.6f + 47.0f * 0.9f, 29.0f + 47.0f * 0.9f), true);
        shine.addColour (0.35, Colours::white.withAlpha (0.12f));
        shine.addColour (0.6, Colours::transparentBlack);
        g.setGradientFill (shine);
        g.fillEllipse (skirt);
        juce::Path hi;
        hi.addCentredArc (P (42.0f, 42.0f).x, P (42.0f, 42.0f).y, 22.0f * s, 22.0f * s, 0.0f, -1.03f, -0.175f, true);
        g.setColour (Colours::white.withAlpha (0.8f));
        g.strokePath (hi, juce::PathStrokeType (2.0f * s, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // bakelite body
    auto body = circle (42.0f, 42.0f, 19.0f);
    juce::ColourGradient bake (Colour (0xff5a554e), P (38.0f, 35.0f), Colour (0xff0b0a08), P (38.0f + 30.0f, 35.0f + 30.0f), true);
    bake.addColour (0.6, Colour (0xff1f1c18));
    g.setGradientFill (bake);
    g.fillEllipse (body);
    g.setColour (Colours::black);
    g.drawEllipse (body, 1.0f);
    g.setColour (Colours::white.withAlpha (0.24f));
    g.fillEllipse (Rectangle<float> (18.0f * s, 10.0f * s).withCentre (P (35.0f, 33.0f)));
    g.setColour (Colours::white.withAlpha (0.45f));
    g.fillEllipse (Rectangle<float> (7.0f * s, 3.6f * s).withCentre (P (33.0f, 31.0f)));

    // pointer
    const float a = 135.0f + 270.0f * juce::jlimit (0.0f, 1.0f, pos);
    g.setColour (Colour (0xfff6f1e0));
    g.drawLine (juce::Line<float> (polar (a, 5.0f), polar (a, 17.5f)), 3.4f * s);
    g.fillEllipse (Rectangle<float> (3.4f * s, 3.4f * s).withCentre (polar (a, 17.5f)));
    g.fillEllipse (Rectangle<float> (3.4f * s, 3.4f * s).withCentre (polar (a, 5.0f)));
}

void TubeLookAndFeel::drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos, float, float, juce::Slider& s)
{
    const bool dark = (bool) s.getProperties()["dark"];
    const bool numerals = ! s.getProperties().contains ("noNumerals");
    drawKnob (g, Rectangle<float> ((float) x, (float) y, (float) w, (float) h), pos, (bool) s.getProperties()["chrome"],
              dark ? ui::cream : ui::ink, numerals);
}

void TubeLookAndFeel::drawButtonBackground (Graphics& g, juce::Button& b, const Colour&, bool, bool down)
{
    const auto style = b.getProperties()["style"].toString();
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    const bool on = b.getToggleState();

    if (style == "jewel")
    {
        const float d = std::min (r.getWidth(), r.getHeight());
        auto outer = Rectangle<float> (d, d).withCentre (r.getCentre());
        g.setGradientFill (chromeGradient (outer));
        g.fillEllipse (outer);
        g.setColour (Colour (0xff5d6066));
        g.drawEllipse (outer, 1.0f);
        auto jewel = outer.reduced (d * 0.32f);
        if (on)                                     // bypassed: dark lamp
        {
            g.setColour (Colour (0xff4a1612));
            g.fillEllipse (jewel);
        }
        else
        {
            juce::ColourGradient jg (Colour (0xffffb3a8), jewel.getX() + jewel.getWidth() * 0.4f, jewel.getY() + jewel.getHeight() * 0.35f,
                                     Colour (0xff6e0c06), jewel.getRight(), jewel.getBottom(), true);
            jg.addColour (0.5, Colour (0xffe0301f));
            g.setGradientFill (jg);
            g.fillEllipse (jewel);
            g.setColour (Colour (0x55ff3b2a));
            g.fillEllipse (jewel.expanded (3.0f));
            g.setGradientFill (jg);
            g.fillEllipse (jewel);
        }
        g.setColour (Colour (0xff3a0a06));
        g.drawEllipse (jewel, 1.5f);
        g.setColour (Colours::white.withAlpha (0.45f));
        g.fillEllipse (Rectangle<float> (jewel.getWidth() * 0.4f, jewel.getHeight() * 0.25f).withCentre (jewel.getCentre() - juce::Point<float> (jewel.getWidth() * 0.12f, jewel.getHeight() * 0.18f)));
        return;
    }
    if (style == "mode")
    {
        if (on) { g.setGradientFill (juce::ColourGradient (ui::brassHi, r.getX(), r.getY(), ui::brassMid, r.getX(), r.getBottom(), false)); g.fillRoundedRectangle (r, 7.0f);
                  g.setColour (Colours::white.withAlpha (0.5f)); g.drawLine (r.getX() + 5, r.getY() + 1.5f, r.getRight() - 5, r.getY() + 1.5f); }
        return;
    }
    if (style == "pill")
    {
        if (on) { g.setColour (ui::teal); g.fillRoundedRectangle (r, 6.0f); }
        else    { g.setColour (Colour (0xff3a3328)); g.drawRoundedRectangle (r, 6.0f, 1.0f); }
        return;
    }
    if (style == "learn" && on)
    {
        g.setGradientFill (juce::ColourGradient (ui::redSoft, r.getX(), r.getY(), Colour (0xffc0392b), r.getX(), r.getBottom(), false));
        g.fillRoundedRectangle (r, 9.0f);
        g.setColour (Colour (0xff4a0f08));
        g.drawRoundedRectangle (r, 9.0f, 1.0f);
        return;
    }
    const float radius = style == "round" ? r.getHeight() * 0.5f : (style == "small" ? 7.0f : 9.0f);
    drawBakelite (g, down ? r.reduced (0.0f, 0.5f) : r, radius, on && style != "learn" && style != "preset");
}

void TubeLookAndFeel::drawButtonText (Graphics& g, juce::TextButton& b, bool, bool)
{
    const auto style = b.getProperties()["style"].toString();
    auto r = b.getLocalBounds();
    const bool on = b.getToggleState();

    if (style == "jewel") return;
    if (style == "preset")
    {
        g.setColour (ui::cream.withAlpha (0.7f));
        g.setFont (ui::font (11.0f));
        g.drawText ("PRESET", r.withTrimmedLeft (14), juce::Justification::centredLeft, false);
        g.setColour (Colour (0xfff0e3bc));
        g.setFont (ui::font (13.0f, true));
        g.drawText (b.getButtonText(), r.withTrimmedLeft (74).withTrimmedRight (28), juce::Justification::centredLeft, true);
        juce::Path chev;
        const auto c = juce::Point<float> ((float) r.getRight() - 18.0f, (float) r.getCentreY());
        chev.startNewSubPath (c.x - 5, c.y - 2.5f); chev.lineTo (c.x, c.y + 2.5f); chev.lineTo (c.x + 5, c.y - 2.5f);
        g.setColour (ui::cream);
        g.strokePath (chev, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        return;
    }

    Colour c = ui::brassHi;
    if (style == "mode")       c = on ? ui::ink : ui::cream;
    else if (style == "pill")  c = on ? Colour (0xff10201f) : ui::dim;
    else if (style == "learn") c = on ? ui::ink : ui::brassHi;
    else if (on)               c = ui::ink;

    g.setColour (c);
    g.setFont (ui::font (style == "round" ? 17.0f : (style == "pill" ? 11.0f : 13.0f), true));
    g.drawText (b.getButtonText(), r, juce::Justification::centred, false);
}

//==============================================================================
// Spectrum
SpectrumDisplay::SpectrumDisplay (themask::MaskEngine& e) : engine_ (e)
{
    in_.fill (-120.0f); out_.fill (-120.0f); env_.fill (-120.0f); red_.fill (0.0f);
    startTimerHz (30);
}

void SpectrumDisplay::timerCallback()
{
    std::array<float, N> i {}, o {}, e {}, r {};
    engine_.getDisplay (i.data(), o.data(), e.data(), r.data());
    for (int g = 0; g < N; ++g)
    {
        const auto k = (size_t) g;
        in_[k]  += (juce::jmax (i[k], -120.0f) - in_[k])  * 0.5f;
        out_[k] += (juce::jmax (o[k], -120.0f) - out_[k]) * 0.5f;
        env_[k] += (juce::jmax (e[k], -120.0f) - env_[k]) * 0.5f;
        red_[k] += (r[k] - red_[k]) * 0.5f;
    }
    repaint();
}

void SpectrumDisplay::paint (Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    constexpr float X0 = 48.0f, W = 642.0f, Y0 = 18.0f, H = 360.0f;
    auto fx = [] (float f) { return X0 + W * std::log10 (f / 20.0f) / 3.0f; };
    auto fy = [] (float db) { return juce::jlimit (Y0, Y0 + H, Y0 + (-10.0f - db) / 80.0f * H); };
    auto gx = [] (int idx) { return X0 + W * (float) idx / (float) (N - 1); };

    juce::Graphics::ScopedSaveState state (g);
    juce::Path clip; clip.addRoundedRectangle (bounds, 8.0f);
    g.reduceClipRegion (clip);
    g.fillAll (Colour (0xff0f0e0b));

    // grid
    g.setColour (Colour (0xff2e2a24));
    for (float f : { 50.f, 100.f, 200.f, 500.f, 1000.f, 2000.f, 5000.f, 10000.f })
        g.drawVerticalLine ((int) fx (f), Y0, Y0 + H);
    for (int d = -20; d >= -80; d -= 10)
        g.drawHorizontalLine ((int) fy ((float) d), X0, X0 + W);
    g.drawHorizontalLine ((int) (Y0 + H), X0, X0 + W);

    g.setFont (ui::mono (10.0f));
    g.setColour (ui::dim);
    for (float f : { 50.f, 100.f, 200.f, 500.f, 1000.f, 2000.f, 5000.f, 10000.f })
        g.drawText (f >= 1000.0f ? juce::String ((int) (f / 1000.0f)) + "k" : juce::String ((int) f),
                    Rectangle<float> (32.0f, 12.0f).withCentre ({ fx (f), Y0 + H + 14.0f }).toNearestInt(), juce::Justification::centred, false);
    for (int d : { -20, -40, -60, -80 })
        g.drawText (juce::String (d), Rectangle<int> (6, (int) fy ((float) d) - 7, 34, 14), juce::Justification::centredRight, false);

    // input
    juce::Path inLine, inFill, outLine, envLine, mask;
    for (int i = 0; i < N; ++i)
    {
        const auto k = (size_t) i;
        const juce::Point<float> p (gx (i), fy (in_[k]));
        if (i == 0) { inLine.startNewSubPath (p); inFill.startNewSubPath (p.x, Y0 + H); inFill.lineTo (p); }
        else        { inLine.lineTo (p); inFill.lineTo (p); }
        const juce::Point<float> q (gx (i), fy (out_[k]));
        if (i == 0) outLine.startNewSubPath (q); else outLine.lineTo (q);
        const juce::Point<float> e (gx (i), fy (env_[k]));
        if (i == 0) envLine.startNewSubPath (e); else envLine.lineTo (e);
    }
    inFill.lineTo (gx (N - 1), Y0 + H);
    inFill.closeSubPath();
    g.setColour (ui::steel.withAlpha (0.15f));
    g.fillPath (inFill);
    g.setColour (ui::steel.withAlpha (0.85f));
    g.strokePath (inLine, juce::PathStrokeType (1.2f));

    // envelope (dashed)
    {
        const float dashes[] = { 4.0f, 4.0f };
        juce::Path dashed;
        juce::PathStrokeType (1.0f).createDashedStroke (dashed, envLine, dashes, 2);
        g.setColour (Colour (0xffece8de).withAlpha (0.55f));
        g.fillPath (dashed);
    }

    // masked region (between input and output)
    mask.startNewSubPath (gx (0), fy (in_[0]));
    for (int i = 1; i < N; ++i) mask.lineTo (gx (i), fy (in_[(size_t) i]));
    for (int i = N - 1; i >= 0; --i) mask.lineTo (gx (i), fy (out_[(size_t) i]));
    mask.closeSubPath();
    g.setColour (ui::redHot.withAlpha (0.6f));
    g.fillPath (mask);

    // labelled resonances: local minima of the reduction curve
    struct Peak { int idx; float red; };
    std::vector<Peak> peaks;
    for (int i = 2; i < N - 2; ++i)
    {
        const float r = red_[(size_t) i];
        if (r < -1.5f && r <= red_[(size_t) i - 1] && r < red_[(size_t) i + 1])
            peaks.push_back ({ i, r });
    }
    std::sort (peaks.begin(), peaks.end(), [] (const Peak& a, const Peak& b) { return a.red < b.red; });
    std::vector<Peak> chosen;
    for (auto& p : peaks)
    {
        bool ok = true;
        for (auto& c : chosen) if (std::abs (c.idx - p.idx) < 14) ok = false;
        if (ok) chosen.push_back (p);
        if (chosen.size() >= 4) break;
    }
    for (auto& p : chosen)
    {
        const float x = gx (p.idx), yIn = fy (in_[(size_t) p.idx]);
        const float dashes[] = { 2.0f, 3.0f };
        juce::Path marker, dashed;
        marker.startNewSubPath (x, yIn - 4.0f); marker.lineTo (x, Y0 + H);
        juce::PathStrokeType (1.0f).createDashedStroke (dashed, marker, dashes, 2);
        g.setColour (ui::redHot.withAlpha (0.8f));
        g.fillPath (dashed);

        const float f = themask::MaskEngine::gridFrequency (p.idx);
        const auto freq = f >= 1000.0f ? juce::String (f / 1000.0f, 1) + " kHz" : juce::String ((int) std::lround (f)) + " Hz";
        g.setFont (ui::font (11.0f));
        g.setColour (Colour (0xffece8de));
        g.drawText (freq, Rectangle<float> (100.0f, 15.0f).withCentre ({ x, yIn - 32.0f }).toNearestInt(), juce::Justification::centred, false);
        g.setColour (ui::redSoft);
        g.drawText (juce::String::fromUTF8 ("\xe2\x88\x92") + juce::String (-p.red, 1) + " dB",
                    Rectangle<float> (100.0f, 15.0f).withCentre ({ x, yIn - 18.0f }).toNearestInt(), juce::Justification::centred, false);
    }

    // output trace with glow
    g.setColour (ui::amber.withAlpha (0.16f));
    g.strokePath (outLine, juce::PathStrokeType (7.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (ui::amber);
    g.strokePath (outLine, juce::PathStrokeType (2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // glass glare
    {
        juce::ColourGradient glare (Colours::white.withAlpha (0.10f), 0.0f, 0.0f, Colours::transparentWhite, bounds.getWidth() * 0.42f, bounds.getHeight() * 0.25f, false);
        g.setGradientFill (glare);
        juce::Path p;
        p.addTriangle (0.0f, 0.0f, bounds.getWidth() * 0.62f, 0.0f, 0.0f, bounds.getHeight() * 0.9f);
        g.fillPath (p);
    }

    // legend
    {
        auto lg = Rectangle<float> (292.0f, 22.0f).withRightX (bounds.getRight() - 12.0f).withY (10.0f);
        g.setColour (Colour (0xd90f0e0b));
        g.fillRoundedRectangle (lg, 6.0f);
        g.setFont (ui::font (10.0f));
        struct Item { const char* name; Colour col; };
        const Item items[] = { { "Input", ui::steel }, { "Envelope", Colour (0xffece8de) }, { "Masked", ui::redHot }, { "Output", ui::amber } };
        float x = lg.getX() + 10.0f;
        for (auto& it : items)
        {
            g.setColour (it.col);
            g.fillRect (x, lg.getCentreY() - 1.0f, 12.0f, 2.0f);
            g.setColour (Colour (0xffc9c3b8));
            const int w = ui::font (10.0f).getStringWidth (it.name);
            g.drawText (it.name, juce::Rectangle<int> ((int) x + 16, (int) lg.getY(), w + 4, (int) lg.getHeight()), juce::Justification::centredLeft, false);
            x += 16.0f + (float) w + 12.0f;
        }
    }
}

//==============================================================================
// Analogue gain-reduction meter
VuMeter::VuMeter (themask::MaskEngine& e) : engine_ (e) { startTimerHz (30); }

void VuMeter::timerCallback()
{
    grDb_ = engine_.getReductionDb();
    const float target = juce::jlimit (0.0f, 1.0f, -grDb_ / 20.0f);
    needle_ += (target - needle_) * (target > needle_ ? 0.35f : 0.10f);
    repaint();
}

void VuMeter::paint (Graphics& g)
{
    auto frame = getLocalBounds().toFloat();
    TubeLookAndFeel::drawChromeFrame (g, frame, 9.0f);

    const auto face = Rectangle<float> (164.0f, 74.0f).withCentre (frame.getCentre());
    juce::Path clip; clip.addRoundedRectangle (face, 4.0f);
    juce::Graphics::ScopedSaveState state (g);
    g.reduceClipRegion (clip);

    juce::ColourGradient bg (Colour (0xfff8f0d4), face.getCentreX(), face.getBottom() + face.getHeight() * 0.25f,
                             Colour (0xffc2b07a), face.getX(), face.getY(), true);
    bg.addColour (0.55, Colour (0xffe8dcb4));
    g.setGradientFill (bg);
    g.fillRect (face);

    const auto o = face.getTopLeft();
    constexpr float PX = 82.0f, PY = 96.0f;
    auto vp = [&] (float r, float fr)
    {
        const float t = (-47.0f + 94.0f * fr) * pi / 180.0f;
        return juce::Point<float> (o.x + PX + r * std::sin (t), o.y + PY - r * std::cos (t));
    };
    auto arc = [&] (float r, float f0, float f1, float width, Colour c)
    {
        juce::Path p;
        const float a0 = (-47.0f + 94.0f * f0) * pi / 180.0f, a1 = (-47.0f + 94.0f * f1) * pi / 180.0f;
        p.addCentredArc (o.x + PX, o.y + PY, r, r, 0.0f, a0, a1, true);
        g.setColour (c);
        g.strokePath (p, juce::PathStrokeType (width));
    };

    arc (36.0f, 0.0f, 1.0f, 0.8f, Colour (0xff6b5a33));
    for (int j = 0; j <= 10; ++j)
    {
        g.setColour (Colour (0xff6b5a33));
        g.drawLine (juce::Line<float> (vp (j % 5 == 0 ? 31.0f : 33.0f, (float) j / 10.0f), vp (36.0f, (float) j / 10.0f)), 0.8f);
    }
    arc (62.0f, 0.0f, 1.0f, 1.2f, Colour (0xff2b2824));
    arc (65.0f, 0.75f, 1.0f, 4.0f, Colour (0xffb03a2e));
    g.setColour (Colour (0xff2b2824));
    for (int i = 0; i < 20; ++i)
        g.drawLine (juce::Line<float> (vp (60.0f, ((float) i + 0.5f) / 20.0f), vp (62.0f, ((float) i + 0.5f) / 20.0f)), 0.7f);
    for (int i = 0; i <= 20; ++i)
        g.drawLine (juce::Line<float> (vp (i % 5 == 0 ? 54.0f : 58.0f, (float) i / 20.0f), vp (62.0f, (float) i / 20.0f)), 1.1f);

    g.setFont (ui::font (10.0f, true));
    const int nums[] = { 0, 5, 10, 15, 20 };
    for (int i = 0; i < 5; ++i)
    {
        g.setColour (nums[i] >= 15 ? Colour (0xffb03a2e) : Colour (0xff2b2824));
        g.drawText (juce::String (nums[i]), Rectangle<float> (20.0f, 14.0f).withCentre (vp (46.0f, (float) i / 4.0f)).toNearestInt(), juce::Justification::centred, false);
    }

    // needle (with shadow)
    const auto tip = vp (60.0f, needle_);
    const auto pivot = juce::Point<float> (o.x + PX, o.y + PY);
    g.setColour (Colours::black.withAlpha (0.28f));
    g.drawLine (juce::Line<float> (pivot + juce::Point<float> (1.5f, 1.5f), tip + juce::Point<float> (1.5f, 1.5f)), 2.2f);
    g.setColour (ui::ink);
    g.drawLine (juce::Line<float> (pivot, tip), 1.6f);

    // furniture
    g.setColour (Colour (0xff7a2418));
    g.setFont (ui::mono (11.0f));
    g.drawText (juce::String::fromUTF8 ("\xe2\x88\x92") + juce::String (-grDb_, 1) + " dB", Rectangle<int> ((int) o.x + 7, (int) o.y + 3, 70, 14), juce::Justification::centredLeft, false);
    g.setColour (Colour (0xff5a4a22));
    g.setFont (ui::font (8.0f, true));
    g.drawText ("GAIN REDUCTION", Rectangle<int> ((int) o.x, (int) o.y + 17, 164, 10), juce::Justification::centred, false);
    g.drawText ("PEAK", Rectangle<int> ((int) o.x + 120, (int) o.y + 16, 40, 10), juce::Justification::centredRight, false);
    const bool peak = grDb_ < -12.0f;
    g.setColour (peak ? Colour (0xffff3b2a) : Colour (0xff5a1410));
    g.fillEllipse (Rectangle<float> (7.0f, 7.0f).withCentre ({ o.x + 150.0f, o.y + 10.0f }));
    g.setColour (Colour (0xff2b2824));
    g.drawEllipse (Rectangle<float> (7.0f, 7.0f).withCentre ({ o.x + 150.0f, o.y + 10.0f }), 1.0f);
    TubeLookAndFeel::drawScrew (g, { o.x + 152.0f, o.y + 62.0f }, 8.0f, 0.5f);

    // glass glare
    juce::Path glare;
    glare.addQuadrilateral (o.x, o.y, o.x + 96.0f, o.y, o.x + 50.0f, o.y + 74.0f, o.x, o.y + 74.0f);
    g.setColour (Colours::white.withAlpha (0.16f));
    g.fillPath (glare);
}

//==============================================================================
// Single glowing valve
void ValveIndicator::paint (Graphics& g)
{
    const float s = std::min (getWidth() / 44.0f, getHeight() / 58.0f);
    const auto o = getLocalBounds().toFloat().getCentre() - juce::Point<float> (22.0f * s, 29.0f * s);
    auto P = [&] (float x, float y) { return juce::Point<float> (o.x + x * s, o.y + y * s); };
    const float glow = glowing_ ? 1.0f : 0.0f;

    if (glowing_)
    {
        juce::ColourGradient halo (Colour (0xffffc878).withAlpha (0.55f), P (22.0f, 30.0f), Colour (0xffff8a2a).withAlpha (0.0f), P (22.0f, 30.0f + 27.0f), true);
        halo.addColour (0.5, Colour (0xffff8a2a).withAlpha (0.28f));
        g.setGradientFill (halo);
        g.fillEllipse (Rectangle<float> (54.0f * s, 54.0f * s).withCentre (P (22.0f, 28.0f)));
    }

    juce::Path env;
    env.startNewSubPath (P (7, 49));  env.lineTo (P (7, 20));
    env.cubicTo (P (7, 8), P (14, 3), P (22, 3));
    env.cubicTo (P (30, 3), P (37, 8), P (37, 20));
    env.lineTo (P (37, 49));          env.closeSubPath();
    g.setColour (Colour (0xff1a140e).withAlpha (0.5f));
    g.fillPath (env);
    g.setColour (Colour (0xff5a5d63));
    g.strokePath (env, juce::PathStrokeType (1.2f * s));

    g.setColour (Colour (0xffc7cad0).withAlpha (0.55f));                                   // getter flash
    g.fillEllipse (Rectangle<float> (16.0f * s, 9.0f * s).withCentre (P (22, 9)));
    g.setColour (Colour (0xffd8cfa8));                                                    // mica
    g.fillRoundedRectangle (Rectangle<float> (20.0f * s, 3.0f * s).withCentre (P (22, 14.5f)), 1.0f);
    g.setColour (Colour (0xff5e6168));                                                    // plate
    g.fillRoundedRectangle (Rectangle<float> (18.0f * s, 29.0f * s).withCentre (P (22, 30.5f)), 2.5f * s);
    g.setColour (Colour (0xff2f3237));
    g.drawRoundedRectangle (Rectangle<float> (18.0f * s, 29.0f * s).withCentre (P (22, 30.5f)), 2.5f * s, 1.0f);
    g.drawLine (P (17, 16).x, P (17, 16).y, P (17, 45).x, P (17, 45).y, 0.8f);
    g.drawLine (P (27, 16).x, P (27, 16).y, P (27, 45).x, P (27, 45).y, 0.8f);

    if (glowing_)                                                                          // filament
    {
        for (int i = 3; i >= 1; --i)
        {
            g.setColour (ui::amber.withAlpha (0.18f * glow));
            g.fillRoundedRectangle (Rectangle<float> ((6.0f + 2.0f * (float) i) * s, (23.0f + 2.0f * (float) i) * s).withCentre (P (22, 30.5f)), 3.0f * s);
        }
        g.setColour (Colour (0xffffe2a6));
        g.fillRoundedRectangle (Rectangle<float> (3.0f * s, 21.0f * s).withCentre (P (22, 30.5f)), 1.5f * s);
    }
    else
    {
        g.setColour (Colour (0xff6a4a28));
        g.fillRoundedRectangle (Rectangle<float> (3.0f * s, 21.0f * s).withCentre (P (22, 30.5f)), 1.5f * s);
    }

    g.setColour (Colour (0xff17140f).withAlpha (0.6f));                                    // grid wires
    for (float y : { 22.0f, 28.0f, 34.0f, 40.0f })
        g.drawLine (P (14, y).x, P (14, y).y, P (30, y).x, P (30, y).y, 0.8f);

    g.setColour (Colours::white.withAlpha (0.4f));                                         // glass highlights
    g.drawLine (P (10, 22).x, P (10, 22).y, P (10, 46).x, P (10, 46).y, 2.0f * s);
    juce::Path hi;
    hi.startNewSubPath (P (12, 10)); hi.cubicTo (P (14, 6), P (18, 4.5f), P (21, 4.5f));
    g.setColour (Colours::white.withAlpha (0.5f));
    g.strokePath (hi, juce::PathStrokeType (1.5f * s, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    auto strip = Rectangle<float> (34.0f * s, 3.0f * s).withCentre (P (22, 48.5f));          // base
    g.setGradientFill (chromeGradient (strip));
    g.fillRoundedRectangle (strip, 1.0f);
    g.setColour (Colour (0xff1f1c18));
    g.fillRoundedRectangle (Rectangle<float> (32.0f * s, 6.0f * s).withCentre (P (22, 53)), 2.0f * s);
    g.setColour (Colours::black);
    g.drawRoundedRectangle (Rectangle<float> (32.0f * s, 6.0f * s).withCentre (P (22, 53)), 2.0f * s, 1.0f);
    g.setColour (Colour (0xffa8adb4));
    for (float x : { 12.0f, 18.0f, 26.0f, 32.0f })
        g.drawLine (P (x, 56).x, P (x, 56).y, P (x, 58).x, P (x, 58).y, 2.0f * s);
}

//==============================================================================
// Limiter column
LimiterPanel::LimiterPanel (TheMaskAudioProcessor& p) : proc_ (p)
{
    onButton_.getProperties().set ("style", "small");
    onButton_.setClickingTogglesState (true);
    addAndMakeVisible (onButton_);
    onAttach_ = std::make_unique<juce::ParameterAttachment> (*proc_.apvts.getParameter (ids::limOn),
        [this] (float v) { onButton_.setToggleState (v > 0.5f, juce::dontSendNotification); onButton_.setButtonText (v > 0.5f ? "On" : "Off"); });
    onAttach_->sendInitialUpdate();
    onButton_.onClick = [this] { onAttach_->setValueAsCompleteGesture (onButton_.getToggleState() ? 1.0f : 0.0f); };

    const char* pid[]    = { ids::limGain, ids::limThresh, ids::limCeiling, ids::limRelease };
    const char* labels[] = { "Gain", "Thresh", "Ceiling", "Release" };
    for (size_t i = 0; i < 4; ++i)
    {
        auto& c = cells_[i];
        c.label = labels[i];
        c.chrome = (i == 2);
        c.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        c.slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        c.slider.setRotaryParameters (1.25f * pi, 2.75f * pi, true);
        c.slider.getProperties().set ("dark", true);
        c.slider.getProperties().set ("noNumerals", true);
        c.slider.getProperties().set ("chrome", c.chrome);
        c.attach = std::make_unique<juce::SliderParameterAttachment> (*proc_.apvts.getParameter (pid[i]), c.slider);
        c.slider.onValueChange = [this] { repaint(); };
        addAndMakeVisible (c.slider);
    }
    startTimerHz (30);
}

LimiterPanel::~LimiterPanel() = default;

void LimiterPanel::resized()
{
    onButton_.setBounds (getWidth() - 12 - 48, 12, 48, 26);
    for (size_t i = 0; i < 4; ++i)
        cells_[i].slider.setBounds (12 + (int) i * 48, getHeight() - 10 - 74, 48, 48);
}

void LimiterPanel::timerCallback()
{
    auto toDb = [] (float lin) { return 20.0f * std::log10 (std::max (lin, 1.0e-4f)); };
    const float in = toDb (proc_.getLimiter().getInputPeak());
    const float out = toDb (proc_.getLimiter().getOutputPeak());
    const float gr = proc_.getLimiter().getReductionDb();

    inLevel_  = std::max (in,  inLevel_  - 1.5f);   inHold_  = std::max (inLevel_,  inHold_  - 0.25f);
    outLevel_ = std::max (out, outLevel_ - 1.5f);   outHold_ = std::max (outLevel_, outHold_ - 0.25f);
    grDb_     = std::min (gr,  grDb_ + 1.0f);       grHold_  = std::min (grDb_,     grHold_  + 0.15f);
    repaint();
}

void LimiterPanel::paint (Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (Colours::white.withAlpha (0.7f));
    g.drawRoundedRectangle (r.translated (0.0f, 1.0f), 14.0f, 1.0f);
    g.setColour (ui::darkPanel);
    g.fillRoundedRectangle (r, 14.0f);
    g.setColour (Colours::black.withAlpha (0.55f));
    g.drawRoundedRectangle (r.reduced (1.5f), 13.0f, 3.0f);
    g.setColour (Colours::black);
    g.drawRoundedRectangle (r, 14.0f, 1.0f);

    g.setColour (ui::brassHi);
    g.setFont (ui::display (26.0f));
    g.drawText ("LIMITER", Rectangle<int> (12, 10, 130, 30), juce::Justification::centredLeft, false);

    const int barTop = 62, barH = 196;
    const int xs[3] = { 82, 112, 142 };
    g.setFont (ui::font (10.0f, true));
    g.setColour (ui::dim);
    const char* heads[3] = { "IN", "GR", "OUT" };
    for (int i = 0; i < 3; ++i)
        g.drawText (heads[i], Rectangle<int> (xs[i] - 4, 46, 32, 12), juce::Justification::centred, false);

    // dB scale
    g.setFont (ui::mono (10.0f));
    for (int d : { 0, -3, -6, -12, -18, -24 })
        g.drawText (juce::String::fromUTF8 (d == 0 ? "0" : (juce::String::fromUTF8 ("\xe2\x88\x92") + juce::String (-d)).toRawUTF8()),
                    Rectangle<int> (46, barTop + (int) (-(float) d / 24.0f * (float) barH) - 6, 30, 12), juce::Justification::centredRight, false);

    auto bar = [&] (int x, float frac, float hold, bool isGr)
    {
        auto b = Rectangle<float> ((float) x, (float) barTop, 24.0f, (float) barH);
        g.setColour (Colour (0xff0a0908));
        g.fillRoundedRectangle (b, 3.0f);
        {
            juce::Graphics::ScopedSaveState s (g);
            juce::Path clip; clip.addRoundedRectangle (b.reduced (1.0f), 2.0f);
            g.reduceClipRegion (clip);
            if (isGr)
            {
                g.setGradientFill (juce::ColourGradient (ui::redSoft, b.getX(), b.getY(), Colour (0xffc0392b), b.getX(), b.getBottom(), false));
                g.fillRect (b.withHeight (b.getHeight() * frac));
            }
            else
            {
                juce::ColourGradient lg (Colour (0xff4cc38a), b.getX(), b.getBottom(), ui::redHot, b.getX(), b.getY(), false);
                lg.addColour (0.6, Colour (0xff4cc38a));
                lg.addColour (0.8, ui::amber);
                g.setGradientFill (lg);
                g.fillRect (b.withTop (b.getBottom() - b.getHeight() * frac));
            }
            g.setColour (Colour (0xe60a0908));                      // LED ladder segments
            for (float y = b.getY(); y < b.getBottom(); y += 5.0f)
                g.drawHorizontalLine ((int) y, b.getX(), b.getRight());
            g.setColour (Colour (0xffece8de));
            const float hy = isGr ? b.getY() + b.getHeight() * hold : b.getBottom() - b.getHeight() * hold;
            g.fillRect (b.getX(), hy - 1.0f, b.getWidth(), 2.0f);
        }
        g.setColour (Colour (0xff2e2a24));
        g.drawRoundedRectangle (b, 3.0f, 1.0f);
    };
    auto fracIn = [] (float db) { return juce::jlimit (0.0f, 1.0f, (db + 24.0f) / 24.0f); };
    bar (xs[0], fracIn (inLevel_), fracIn (inHold_), false);
    bar (xs[1], juce::jlimit (0.0f, 1.0f, -grDb_ / 12.0f), juce::jlimit (0.0f, 1.0f, -grHold_ / 12.0f), true);
    bar (xs[2], fracIn (outLevel_), fracIn (outHold_), false);

    const auto minus = juce::String::fromUTF8 ("\xe2\x88\x92");
    g.setColour (ui::dim);
    g.setFont (ui::mono (10.0f));
    g.drawText ("GR", Rectangle<int> (14, 266, 60, 18), juce::Justification::centredLeft, false);
    g.setColour (ui::redSoft);
    g.setFont (ui::mono (14.0f));
    g.drawText (minus + juce::String (-grDb_, 1) + " dB", Rectangle<int> (80, 266, 124, 18), juce::Justification::centredRight, false);
    g.setColour (ui::dim);
    g.setFont (ui::mono (10.0f));
    g.drawText ("TRUE PEAK", Rectangle<int> (14, 288, 80, 16), juce::Justification::centredLeft, false);
    g.setColour (ui::green);
    g.setFont (ui::mono (12.0f));
    const float pk = outHold_;
    g.drawText ((pk < -0.05f ? minus + juce::String (-pk, 1) : juce::String (pk, 1)) + " dB", Rectangle<int> (94, 288, 110, 16), juce::Justification::centredRight, false);

    // dial captions
    for (size_t i = 0; i < 4; ++i)
    {
        const int x = 12 + (int) i * 48;
        g.setColour (ui::cream);
        g.setFont (ui::font (10.0f, true));
        g.drawText (cells_[i].label.toUpperCase(), Rectangle<int> (x - 2, getHeight() - 10 - 26, 52, 12), juce::Justification::centred, false);
        g.setColour (ui::dim);
        g.setFont (ui::mono (9.0f));
        auto* prm = proc_.apvts.getParameter ((i == 0 ? ids::limGain : i == 1 ? ids::limThresh : i == 2 ? ids::limCeiling : ids::limRelease));
        g.drawText (prm->getCurrentValueAsText(), Rectangle<int> (x - 4, getHeight() - 10 - 14, 56, 12), juce::Justification::centred, false);
    }
}

//==============================================================================
// Glue panel: one big dial that blends every suggestion
GluePanel::GluePanel (TheMaskAudioProcessor& p) : proc_ (p)
{
    dial_.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    dial_.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    dial_.setRotaryParameters (1.25f * pi, 2.75f * pi, true);
    dial_.getProperties().set ("dark", true);
    attach_ = std::make_unique<juce::SliderParameterAttachment> (*proc_.apvts.getParameter (ids::glue), dial_);
    dial_.onValueChange = [this] { repaint(); };
    addAndMakeVisible (dial_);

    for (auto* b : { &minus_, &plus_ })
    {
        b->getProperties().set ("style", "round");
        addAndMakeVisible (*b);
    }
    minus_.setButtonText (juce::String::fromUTF8 ("\xe2\x88\x92"));
    minus_.onClick = [this] { dial_.setValue (std::max (0.0, std::round ((dial_.getValue() - 0.1) * 10.0) / 10.0)); };
    plus_.onClick  = [this] { dial_.setValue (std::min (1.0, std::round ((dial_.getValue() + 0.1) * 10.0) / 10.0)); };

    for (size_t i = 0; i < toggles_.size(); ++i)
    {
        auto& t = toggles_[i];
        t.getProperties().set ("style", "pill");
        t.setClickingTogglesState (true);
        t.setToggleState (true, juce::dontSendNotification);
        t.setButtonText ("In");
        t.onClick = [this, i] { included_[i] = toggles_[i].getToggleState(); toggles_[i].setButtonText (included_[i] ? "In" : "Out"); repaint(); };
        addAndMakeVisible (t);
    }
}

GluePanel::~GluePanel() = default;

void GluePanel::resized()
{
    dial_.setBounds (276, 6, 116, 116);
    minus_.setBounds (258, 130, 26, 26);
    plus_.setBounds (384, 130, 26, 26);
    for (size_t i = 0; i < toggles_.size(); ++i)
        toggles_[i].setBounds (getWidth() - 16 - 52, 38 + (int) i * 29 + 2, 52, 24);
}

void GluePanel::paint (Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (Colours::white.withAlpha (0.7f));
    g.drawRoundedRectangle (r.translated (0.0f, 1.0f), 10.0f, 1.0f);
    g.setColour (ui::darkPanel);
    g.fillRoundedRectangle (r, 10.0f);
    g.setColour (Colours::black.withAlpha (0.55f));
    g.drawRoundedRectangle (r.reduced (1.5f), 9.0f, 3.0f);
    g.setColour (Colours::black);
    g.drawRoundedRectangle (r, 10.0f, 1.0f);

    const float amount = (float) dial_.getValue();
    int count = 0;
    for (bool b : included_) count += b ? 1 : 0;

    // left column
    g.setColour (ui::teal);
    g.setFont (ui::display (26.0f));
    g.drawText ("GLUE", Rectangle<int> (16, 10, 80, 28), juce::Justification::centredLeft, false);
    g.setColour (ui::dim);
    g.setFont (ui::font (11.0f));
    g.drawText ("fit to the main track", Rectangle<int> (92, 16, 130, 16), juce::Justification::centredLeft, false);

    auto ref = Rectangle<float> (16.0f, 46.0f, 210.0f, 36.0f);
    g.setColour (Colour (0xff0a0908));
    g.fillRoundedRectangle (ref, 8.0f);
    g.setColour (Colour (0xff3a3328));
    g.drawRoundedRectangle (ref, 8.0f, 1.0f);
    g.setColour (ui::dim);
    g.setFont (ui::font (9.0f, true));
    g.drawText ("REFERENCE", ref.toNearestInt().withTrimmedLeft (12).withHeight (18).withY (ref.toNearestInt().getY() + 2), juce::Justification::centredLeft, false);
    g.setColour (Colour (0xffece8de));
    g.setFont (ui::font (12.0f, true));
    g.drawText (juce::String::fromUTF8 ("Main Mix \xc2\xb7 Sidechain 1"), ref.toNearestInt().withTrimmedLeft (12).withTrimmedTop (16), juce::Justification::centredLeft, false);

    g.setColour (ui::green);
    g.fillEllipse (18.0f, 94.0f, 8.0f, 8.0f);
    g.setColour (ui::dim);
    g.setFont (ui::mono (11.0f));
    g.drawText ("Preview - analysis next", Rectangle<int> (32, 90, 190, 16), juce::Justification::centredLeft, false);

    g.setColour (ui::amber);
    g.setFont (ui::mono (30.0f));
    g.drawText (juce::String (74 + juce::roundToInt (16.0f * amount * (float) count / 4.0f)) + "%", Rectangle<int> (16, 118, 100, 34), juce::Justification::centredLeft, false);
    g.setColour (ui::dim);
    g.setFont (ui::font (11.0f));
    g.drawText ("GLUE FIT", Rectangle<int> (118, 128, 100, 16), juce::Justification::centredLeft, false);

    // dial readout
    g.setColour (ui::amber);
    g.setFont (ui::mono (14.0f));
    g.drawText (juce::String (juce::roundToInt (amount * 100.0f)) + "%", Rectangle<int> (288, 130, 94, 26), juce::Justification::centred, false);

    // suggestions
    const int sx = 442, sw = getWidth() - 16 - sx;
    g.setColour (ui::dim);
    g.setFont (ui::font (11.0f));
    g.drawText ("BLENDED BY THE GLUE DIAL", Rectangle<int> (sx, 12, 260, 22), juce::Justification::centredLeft, false);
    g.setColour (ui::teal);
    g.setFont (ui::mono (11.0f));
    g.drawText (juce::String (count) + " of 4 in " + juce::String::fromUTF8 ("\xc2\xb7") + " " + juce::String (juce::roundToInt (amount * 100.0f)) + "%",
                Rectangle<int> (sx + sw - 200, 12, 200, 22), juce::Justification::centredRight, false);

    const auto minus = juce::String::fromUTF8 ("\xe2\x88\x92");
    const juce::String titles[4] = {
        "Mask 2.8 kHz an extra " + minus + juce::String (3.0f * amount * (included_[0] ? 1.0f : 0.0f), 1) + " dB",
        "Dip 310 Hz by " + minus + juce::String (2.5f * amount * (included_[1] ? 1.0f : 0.0f), 1) + " dB",
        "Attack " + juce::String (juce::roundToInt (12.0f - 4.0f * amount * (included_[2] ? 1.0f : 0.0f))) + " ms (target 8 ms)",
        "Ease 7.8 kHz mask to " + minus + juce::String (8.0f - 4.0f * amount * (included_[3] ? 1.0f : 0.0f), 1) + " dB" };
    const char* reasons[4] = { "Reference lead sits here and this track is masking it", "Both build up here; the reference body is stronger",
                               "Reference transients are faster; this tightens the groove", "No clash here, so keep more of the top-end air" };
    const char* chips[4] = { "MASK", "DIP", "TIME", "EASE" };
    const Colour chipCol[4] = { ui::redSoft, ui::redSoft, Colour (0xffa9a1e0), ui::amber };

    for (int i = 0; i < 4; ++i)
    {
        const int y = 38 + i * 29;
        auto chip = Rectangle<float> (44.0f, 18.0f).withPosition ((float) sx, (float) y + 5.5f);
        g.setColour (chipCol[i]);
        g.drawRoundedRectangle (chip, 4.0f, 1.0f);
        g.setFont (ui::mono (10.0f));
        g.drawText (chips[i], chip.toNearestInt(), juce::Justification::centred, false);
        g.setColour (Colour (0xffece8de));
        g.setFont (ui::font (12.0f, true));
        g.drawText (titles[i], Rectangle<int> (sx + 54, y, sw - 54 - 62, 15), juce::Justification::centredLeft, true);
        g.setColour (ui::dim);
        g.setFont (ui::font (11.0f));
        g.drawText (reasons[i], Rectangle<int> (sx + 54, y + 14, sw - 54 - 62, 14), juce::Justification::centredLeft, true);
    }
}

//==============================================================================
// Editor
TheMaskAudioProcessorEditor::TheMaskAudioProcessorEditor (TheMaskAudioProcessor& p)
    : AudioProcessorEditor (&p), proc_ (p),
      spectrum_ (p.getEngine()), vu_ (p.getEngine()), limiter_ (p), glue_ (p)
{
    setLookAndFeel (&laf_);
    setSize (1040, 904);

    addAndMakeVisible (spectrum_);
    addAndMakeVisible (vu_);
    addAndMakeVisible (valve_);
    addAndMakeVisible (limiter_);
    addAndMakeVisible (glue_);

    // eight tube-amp dials
    struct Def { const char* id; const char* label; bool chrome; };
    const Def defs[] = { { ids::depth, "Depth", false }, { ids::sharpness, "Sharpness", false }, { ids::attack, "Attack", false },
                         { ids::release, "Release", false }, { ids::transient, "Transient", true }, { ids::lowHz, "Low", false },
                         { ids::highHz, "High", false }, { ids::mix, "Mix", false } };
    for (size_t i = 0; i < knobs_.size(); ++i)
    {
        auto& k = knobs_[i];
        k.label = defs[i].label;
        k.chrome = defs[i].chrome;
        k.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        k.slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        k.slider.setRotaryParameters (1.25f * pi, 2.75f * pi, true);
        k.slider.getProperties().set ("chrome", k.chrome);
        k.attach = std::make_unique<juce::SliderParameterAttachment> (*proc_.apvts.getParameter (defs[i].id), k.slider);
        k.slider.onValueChange = [this] { repaint (0, 536, getWidth(), 120); };
        addAndMakeVisible (k.slider);
    }

    // detection mode
    const char* modeNames[] = { "Live", "Learn", "Both" };
    for (size_t i = 0; i < modeButtons_.size(); ++i)
    {
        auto& b = modeButtons_[i];
        b.setButtonText (modeNames[i]);
        b.getProperties().set ("style", "mode");
        b.setClickingTogglesState (false);
        b.onClick = [this, i] { modeAttach_->setValueAsCompleteGesture ((float) i); };
        addAndMakeVisible (b);
    }
    modeAttach_ = std::make_unique<juce::ParameterAttachment> (*proc_.apvts.getParameter (ids::mode),
        [this] (float v) { for (size_t i = 0; i < modeButtons_.size(); ++i) modeButtons_[i].setToggleState ((int) std::lround (v) == (int) i, juce::dontSendNotification); });
    modeAttach_->sendInitialUpdate();

    // preset drop-down
    presetButton_.getProperties().set ("style", "preset");
    presetButton_.setButtonText (proc_.currentPresetName);
    presetButton_.onClick = [this] { showPresetMenu(); };
    addAndMakeVisible (presetButton_);

    // bypass jewel
    bypassButton_.getProperties().set ("style", "jewel");
    bypassButton_.setClickingTogglesState (true);
    bypassButton_.onClick = [this] { bypassAttach_->setValueAsCompleteGesture (bypassButton_.getToggleState() ? 1.0f : 0.0f); };
    bypassAttach_ = std::make_unique<juce::ParameterAttachment> (*proc_.apvts.getParameter (ids::bypass),
        [this] (float v) { bypassButton_.setToggleState (v > 0.5f, juce::dontSendNotification); valve_.setGlowing (v < 0.5f); });
    bypassAttach_->sendInitialUpdate();
    addAndMakeVisible (bypassButton_);

    // learn
    learnButton_.getProperties().set ("style", "learn");
    learnButton_.onClick = [this]
    {
        auto& e = proc_.getEngine();
        if (e.isLearning()) e.stopLearning(); else e.startLearning();
    };
    addAndMakeVisible (learnButton_);

    // delta
    deltaButton_.getProperties().set ("style", "small");
    deltaButton_.setClickingTogglesState (true);
    deltaButton_.onClick = [this] { deltaAttach_->setValueAsCompleteGesture (deltaButton_.getToggleState() ? 1.0f : 0.0f); };
    deltaAttach_ = std::make_unique<juce::ParameterAttachment> (*proc_.apvts.getParameter (ids::delta),
        [this] (float v) { deltaButton_.setToggleState (v > 0.5f, juce::dontSendNotification); });
    deltaAttach_->sendInitialUpdate();
    addAndMakeVisible (deltaButton_);

    buildBackground();
    startTimerHz (10);
}

TheMaskAudioProcessorEditor::~TheMaskAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void TheMaskAudioProcessorEditor::timerCallback()
{
    auto& e = proc_.getEngine();
    juce::String s;
    if (e.isLearning())
        s = "Listening... " + juce::String ((double) e.getLearnedFrames() * themask::MaskEngine::kHop / std::max (1.0, e.getSampleRate()), 1) + " s";
    else if (e.hasLearnedCurve())
        s = juce::String (e.getLearnedResonances()) + " resonance" + (e.getLearnedResonances() == 1 ? "" : "s") + " learned";
    else
        s = "No curve learned yet";

    learnButton_.setButtonText (e.isLearning() ? "Stop" : "Learn");
    learnButton_.setToggleState (e.isLearning(), juce::dontSendNotification);
    if (s != learnStatus_) { learnStatus_ = s; repaint (48, 600, 140, 52); }
    if (presetButton_.getButtonText() != proc_.currentPresetName) presetButton_.setButtonText (proc_.currentPresetName);
}

void TheMaskAudioProcessorEditor::showPresetMenu()
{
    juce::PopupMenu menu;
    const auto& presets = TheMaskAudioProcessor::getPresets();
    juce::String last;
    juce::PopupMenu sub;
    auto flush = [&] { if (last.isNotEmpty()) menu.addSubMenu (last, sub); sub = juce::PopupMenu(); };
    for (size_t i = 0; i < presets.size(); ++i)
    {
        if (last != presets[i].category) { flush(); last = presets[i].category; }
        const bool ticked = proc_.currentPresetName == juce::String (presets[i].category) + "  /  " + presets[i].name;
        sub.addItem ((int) i + 1, presets[i].name, true, ticked);
    }
    flush();

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&presetButton_).withMinimumWidth (presetButton_.getWidth()),
        [this] (int result)
        {
            if (result > 0)
            {
                proc_.applyPreset (TheMaskAudioProcessor::getPresets()[(size_t) result - 1]);
                presetButton_.setButtonText (proc_.currentPresetName);
            }
        });
}

//==============================================================================
void TheMaskAudioProcessorEditor::resized()
{
    valve_.setBounds (340, 28, 42, 56);
    for (size_t i = 0; i < modeButtons_.size(); ++i)
        modeButtons_[i].setBounds (427 + (int) i * 68, 39, 66, 34);
    presetButton_.setBounds (644, 36, 296, 40);
    bypassButton_.setBounds (940, 34, 44, 44);

    spectrum_.setBounds (55, 101, 702, 414);
    limiter_.setBounds (776, 94, 216, 430);

    learnButton_.setBounds (52, 560, 124, 46);
    for (size_t i = 0; i < knobs_.size(); ++i)
        knobs_[i].slider.setBounds (200 + (int) i * 74, 540, 72, 72);
    vu_.setBounds (812, 536, 176, 86);
    deltaButton_.setBounds (812, 624, 176, 28);

    glue_.setBounds (48, 664, 944, 168);
}

//==============================================================================
void TheMaskAudioProcessorEditor::buildBackground()
{
    const float scale = 2.0f;                                   // render the static art at 2x for crisp edges
    background_ = juce::Image (juce::Image::ARGB, (int) (1040 * scale), (int) (904 * scale), true);
    Graphics g (background_);
    g.addTransform (juce::AffineTransform::scale (scale));

    // ---- walnut cabinet
    {
        juce::ColourGradient wood (Colour (0xff6d3d20), 0.0f, 0.0f, Colour (0xff5f3419), 0.0f, 904.0f, false);
        wood.addColour (0.55, Colour (0xff4a2713));
        g.setGradientFill (wood);
        g.fillRect (0, 0, 1040, 904);
        juce::Random rnd (42);
        for (float x = 0.0f; x < 1040.0f; x += 1.0f)
        {
            const float a = rnd.nextFloat();
            if (a < 0.55f) { g.setColour (Colours::black.withAlpha (0.05f + 0.10f * rnd.nextFloat())); g.drawLine (x, 0.0f, x + 3.0f * (rnd.nextFloat() - 0.5f), 904.0f, 1.0f); }
            else if (a > 0.9f) { g.setColour (Colour (0xffffc88c).withAlpha (0.05f)); g.drawLine (x, 0.0f, x + 2.0f * (rnd.nextFloat() - 0.5f), 904.0f, 1.0f); }
        }
        juce::ColourGradient vignette (Colours::transparentBlack, 520.0f, 452.0f, Colours::black.withAlpha (0.55f), 0.0f, 0.0f, true);
        vignette.addColour (0.62, Colours::transparentBlack);
        g.setGradientFill (vignette);
        g.fillRect (0, 0, 1040, 904);
    }

    // ---- polished silver faceplate
    const auto plate = Rectangle<float> (30.0f, 14.0f, 980.0f, 876.0f);
    {
        g.setColour (Colours::black.withAlpha (0.55f));
        g.fillRoundedRectangle (plate.translated (0.0f, 3.0f).expanded (2.0f), 8.0f);
        juce::ColourGradient silver (Colour (0xfffdfdfe), plate.getX(), plate.getY(), Colour (0xffa6abb3), plate.getX(), plate.getBottom(), false);
        silver.addColour (0.18, Colour (0xffdcdfe4));
        silver.addColour (0.44, Colour (0xffaeb3bb));
        silver.addColour (0.60, Colour (0xffedeff2));
        silver.addColour (0.82, Colour (0xffc4c8cf));
        g.setGradientFill (silver);
        g.fillRoundedRectangle (plate, 6.0f);

        // diagonal chrome highlights
        juce::Graphics::ScopedSaveState s (g);
        juce::Path clip; clip.addRoundedRectangle (plate, 6.0f);
        g.reduceClipRegion (clip);
        const float ang = 105.0f * pi / 180.0f;
        const juce::Point<float> c = plate.getCentre();
        const juce::Point<float> dirv (std::cos (ang - pi / 2.0f), std::sin (ang - pi / 2.0f));
        const auto a = c - dirv * 520.0f, b = c + dirv * 520.0f;
        juce::ColourGradient hl (Colours::transparentWhite, a, Colours::transparentWhite, b, false);
        hl.addColour (0.11, Colours::white.withAlpha (0.75f));
        hl.addColour (0.21, Colours::transparentWhite);
        hl.addColour (0.49, Colours::white.withAlpha (0.6f));
        hl.addColour (0.60, Colours::transparentWhite);
        hl.addColour (0.87, Colours::white.withAlpha (0.5f));
        hl.addColour (0.96, Colours::transparentWhite);
        g.setGradientFill (hl);
        g.fillRect (plate);
    }
    g.setColour (Colour (0xff6f737a));
    g.drawRoundedRectangle (plate, 6.0f, 1.0f);
    g.setColour (Colours::white.withAlpha (0.8f));
    g.drawLine (plate.getX() + 6.0f, plate.getY() + 2.0f, plate.getRight() - 6.0f, plate.getY() + 2.0f, 1.5f);
    g.setColour (Colours::black.withAlpha (0.25f));
    g.drawLine (plate.getX() + 6.0f, plate.getBottom() - 2.0f, plate.getRight() - 6.0f, plate.getBottom() - 2.0f, 1.5f);

    TubeLookAndFeel::drawScrew (g, { 44.0f, 28.0f }, 12.0f, 0.6f);
    TubeLookAndFeel::drawScrew (g, { 996.0f, 28.0f }, 12.0f, 2.1f);
    TubeLookAndFeel::drawScrew (g, { 44.0f, 876.0f }, 12.0f, 0.9f);
    TubeLookAndFeel::drawScrew (g, { 996.0f, 876.0f }, 12.0f, 0.0f);

    // ---- header: emblem + title
    {
        const auto emblem = Rectangle<float> (56.0f, 30.0f, 52.0f, 52.0f);
        g.setColour (Colour (0xff17140f));
        g.fillRoundedRectangle (emblem, 9.0f);
        g.setColour (Colour (0xff6f737a));
        g.drawRoundedRectangle (emblem, 9.0f, 1.5f);
        auto mask = juce::Drawable::parseSVGPath ("M6 22 C10 13 19 14 24 18 C29 14 38 13 42 22 C42 31 34 35 28.5 29.5 C26.5 27.5 21.5 27.5 19.5 29.5 C14 35 6 31 6 22 Z "
                                                  "M11 22.5 C14 19.5 18.5 20 20.5 23.5 C17.5 26.5 13 26.5 11 22.5 Z M37 22.5 C34 19.5 29.5 20 27.5 23.5 C30.5 26.5 35 26.5 37 22.5 Z");
        mask.setUsingNonZeroWinding (false);
        mask.applyTransform (juce::AffineTransform::scale (52.0f / 48.0f).translated (56.0f, 30.0f));
        g.setGradientFill (brassGradient (emblem));
        g.fillPath (mask);
        g.setColour (Colour (0xffe0301f));
        g.drawLine (56.0f + 33.0f * 52.0f / 48.0f, 30.0f + 8.0f * 52.0f / 48.0f, 56.0f + 40.0f * 52.0f / 48.0f, 30.0f + 5.0f * 52.0f / 48.0f, 2.0f);
    }
    engraved (g, "THE MASK", { 124, 28, 210, 44 }, ui::display (40.0f), juce::Justification::centredLeft);
    engraved (g, "INTELLIGENT RESONANCE SUPPRESSOR", { 125, 68, 280, 12 }, ui::font (10.0f, true).withExtraKerningFactor (0.12f), juce::Justification::centredLeft);

    // mode switch well
    {
        const auto well = Rectangle<float> (424.0f, 36.0f, 206.0f, 40.0f);
        g.setColour (Colours::white.withAlpha (0.7f));
        g.drawRoundedRectangle (well.translated (0.0f, 1.0f), 9.0f, 1.0f);
        g.setColour (ui::darkPanel);
        g.fillRoundedRectangle (well, 9.0f);
        g.setColour (Colours::black);
        g.drawRoundedRectangle (well, 9.0f, 1.0f);
    }

    // ---- display bezel (chrome)
    TubeLookAndFeel::drawChromeFrame (g, { 48.0f, 94.0f, 716.0f, 430.0f }, 14.0f);
    g.setColour (Colour (0xff0f0e0b));
    g.fillRoundedRectangle (55.0f, 101.0f, 702.0f, 414.0f, 8.0f);

    // ---- control row furniture
    for (int x : { 188, 802 })
    {
        g.setColour (Colour (0xff8e929a));
        g.drawLine ((float) x, 538.0f, (float) x, 650.0f, 1.0f);
        g.setColour (Colours::white.withAlpha (0.55f));
        g.drawLine ((float) x + 1.0f, 538.0f, (float) x + 1.0f, 650.0f, 1.0f);
    }
    engraved (g, "LEARN", { 52, 538, 100, 16 }, ui::font (11.0f, true).withExtraKerningFactor (0.16f), juce::Justification::centredLeft);

    // learn / delta button wells
    for (auto r : { Rectangle<float> (50.0f, 558.0f, 128.0f, 50.0f), Rectangle<float> (810.0f, 622.0f, 180.0f, 32.0f) })
    {
        g.setColour (Colours::black.withAlpha (0.22f));
        g.fillRoundedRectangle (r, 11.0f);
        g.setColour (Colours::white.withAlpha (0.6f));
        g.drawRoundedRectangle (r.translated (0.0f, 1.0f), 11.0f, 1.0f);
    }
}

void TheMaskAudioProcessorEditor::paint (Graphics& g)
{
    g.drawImage (background_, getLocalBounds().toFloat());

    // dial captions and values
    for (size_t i = 0; i < knobs_.size(); ++i)
    {
        const int x = 200 + (int) i * 74;
        engraved (g, knobs_[i].label.toUpperCase(), { x - 4, 612, 80, 13 }, ui::font (10.0f, true).withExtraKerningFactor (0.05f), juce::Justification::centred);
        g.setColour (juce::Colour (0xff3c3830));
        g.setFont (ui::mono (11.0f));
        const char* pid[] = { ids::depth, ids::sharpness, ids::attack, ids::release, ids::transient, ids::lowHz, ids::highHz, ids::mix };
        g.drawText (proc_.apvts.getParameter (pid[i])->getCurrentValueAsText(), juce::Rectangle<int> (x - 4, 625, 80, 14), juce::Justification::centred, false);
    }

    g.setColour (juce::Colour (0xff3c3830));
    g.setFont (ui::mono (11.0f));
    g.drawText (learnStatus_, juce::Rectangle<int> (52, 612, 130, 30), juce::Justification::topLeft, true);

    // footer
    const double sr = proc_.getSampleRate() > 0.0 ? proc_.getSampleRate() : 48000.0;
    const double ms = 1000.0 * themask::MaskEngine::kLatency / sr;
    g.setColour (juce::Colour (0xff3c3830));
    g.setFont (ui::mono (11.0f));
    g.drawText (juce::String ("VST3 ") + juce::String::fromUTF8 ("\xc2\xb7") + " Latency " + juce::String (themask::MaskEngine::kLatency) + " samples ("
                    + juce::String (ms, 1) + " ms @ " + juce::String (sr / 1000.0, 1) + " kHz)",
                juce::Rectangle<int> (52, 844, 400, 16), juce::Justification::centredLeft, false);
    g.drawText (juce::String ("FFT ") + juce::String (themask::MaskEngine::kFftSize) + " " + juce::String::fromUTF8 ("\xc2\xb7") + " Overlap 75%",
                juce::Rectangle<int> (420, 844, 200, 16), juce::Justification::centred, false);
}
