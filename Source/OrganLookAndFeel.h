#pragma once

#include <JuceHeader.h>

//==============================================================================
/** 1980s home electronic organ styling: wood, ivory, black plastic, amber LEDs. */
class OrganLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    // Cabinet palette
    static juce::Colour woodDark()     { return juce::Colour (0xff3e2414); }
    static juce::Colour woodMid()      { return juce::Colour (0xff6b3f22); }
    static juce::Colour woodLight()    { return juce::Colour (0xff8b5a2b); }
    static juce::Colour panelBlack()   { return juce::Colour (0xff1a1512); }
    static juce::Colour panelInset()   { return juce::Colour (0xff0f0c0a); }
    static juce::Colour ivory()        { return juce::Colour (0xfff3e6c8); }
    static juce::Colour ivoryDark()    { return juce::Colour (0xffd4c4a0); }
    static juce::Colour gold()         { return juce::Colour (0xffd4af37); }
    static juce::Colour goldDim()      { return juce::Colour (0xffa08830); }
    static juce::Colour amber()        { return juce::Colour (0xffffb000); }
    static juce::Colour amberGlow()    { return juce::Colour (0xffffcc44); }
    static juce::Colour creamText()    { return juce::Colour (0xfff0e6d0); }
    static juce::Colour silkScreen()   { return juce::Colour (0xffc8b896); }

    OrganLookAndFeel()
    {
        setColour (juce::ResizableWindow::backgroundColourId, woodDark());
        setColour (juce::Label::textColourId, creamText());
        setColour (juce::ComboBox::backgroundColourId, panelInset());
        setColour (juce::ComboBox::textColourId, amberGlow());
        setColour (juce::ComboBox::outlineColourId, goldDim());
        setColour (juce::ComboBox::arrowColourId, gold());
        setColour (juce::PopupMenu::backgroundColourId, panelBlack());
        setColour (juce::PopupMenu::textColourId, creamText());
        setColour (juce::PopupMenu::highlightedBackgroundColourId, woodMid());
        setColour (juce::PopupMenu::highlightedTextColourId, ivory());
        setColour (juce::TextButton::buttonColourId, ivoryDark());
        setColour (juce::TextButton::buttonOnColourId, amber());
        setColour (juce::TextButton::textColourOffId, juce::Colour (0xff2a2010));
        setColour (juce::TextButton::textColourOnId, juce::Colour (0xff1a1000));
        setColour (juce::ToggleButton::textColourId, creamText());
        setColour (juce::ToggleButton::tickColourId, amber());
        setColour (juce::ToggleButton::tickDisabledColourId, goldDim());
        setColour (juce::Slider::textBoxTextColourId, amberGlow());
        setColour (juce::Slider::textBoxBackgroundColourId, panelInset());
        setColour (juce::Slider::textBoxOutlineColourId, goldDim());
        setColour (juce::TextEditor::backgroundColourId, panelInset());
        setColour (juce::TextEditor::textColourId, amberGlow());
        setColour (juce::TextEditor::outlineColourId, goldDim());
        setColour (juce::CaretComponent::caretColourId, amber());
        setColour (juce::AlertWindow::backgroundColourId, woodMid());
        setColour (juce::AlertWindow::textColourId, creamText());
        setColour (juce::AlertWindow::outlineColourId, gold());
    }

    //==============================================================================
    juce::Font getLabelFont (juce::Label& label) override
    {
        const auto h = (float) label.getHeight();
        if (h > 22.0f)
            return juce::Font (juce::FontOptions (juce::jmin (18.0f, h * 0.7f)).withStyle ("Bold"));
        return juce::Font (juce::FontOptions (juce::jmax (11.0f, h * 0.55f)));
    }

    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override
    {
        return juce::Font (juce::FontOptions ((float) juce::jmin (14, buttonHeight - 8)).withStyle ("Bold"));
    }

    juce::Font getComboBoxFont (juce::ComboBox&) override
    {
        return juce::Font (juce::FontOptions (13.0f));
    }

    //==============================================================================
    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider& slider) override
    {
        auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height)
                          .reduced (4.0f);
        const auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
        const auto centre = bounds.getCentre();
        const auto toAngle = rotaryStartAngle
                           + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

        // Shadow
        g.setColour (juce::Colours::black.withAlpha (0.45f));
        g.fillEllipse (centre.x - radius + 2.0f, centre.y - radius + 3.0f,
                       radius * 2.0f, radius * 2.0f);

        // Chrome outer ring
        juce::ColourGradient chrome (juce::Colour (0xffe8e0c8), centre.x - radius, centre.y - radius,
                                     juce::Colour (0xff6a6048), centre.x + radius, centre.y + radius, true);
        g.setGradientFill (chrome);
        g.fillEllipse (centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);

        // Black plastic body
        const auto bodyR = radius * 0.86f;
        juce::ColourGradient body (juce::Colour (0xff3a3530), centre.x, centre.y - bodyR,
                                   juce::Colour (0xff0a0806), centre.x, centre.y + bodyR, false);
        g.setGradientFill (body);
        g.fillEllipse (centre.x - bodyR, centre.y - bodyR, bodyR * 2.0f, bodyR * 2.0f);

        // Highlight arc
        g.setColour (juce::Colours::white.withAlpha (0.12f));
        g.drawEllipse (centre.x - bodyR * 0.7f, centre.y - bodyR * 0.7f,
                       bodyR * 1.4f, bodyR * 0.9f, 1.5f);

        // Pointer (ivory)
        juce::Path pointer;
        const auto pointerLen = bodyR * 0.72f;
        const auto pointerW = juce::jmax (2.5f, bodyR * 0.12f);
        pointer.addRoundedRectangle (-pointerW * 0.5f, -pointerLen, pointerW, pointerLen * 0.72f, 1.0f);
        g.setColour (ivory());
        g.fillPath (pointer, juce::AffineTransform::rotation (toAngle).translated (centre.x, centre.y));

        // Centre cap
        const auto capR = bodyR * 0.18f;
        g.setColour (juce::Colour (0xff2a2520));
        g.fillEllipse (centre.x - capR, centre.y - capR, capR * 2.0f, capR * 2.0f);
        g.setColour (goldDim().withAlpha (0.6f));
        g.drawEllipse (centre.x - capR, centre.y - capR, capR * 2.0f, capR * 2.0f, 1.0f);

        juce::ignoreUnused (slider);
    }

    //==============================================================================
    void drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           juce::Slider::SliderStyle style, juce::Slider& slider) override
    {
        if (style == juce::Slider::LinearVertical)
        {
            // Drawbar-style organ stop
            auto track = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height)
                             .reduced ((float) width * 0.28f, 2.0f);

            // Slot
            g.setColour (panelInset());
            g.fillRoundedRectangle (track, 3.0f);
            g.setColour (goldDim().withAlpha (0.5f));
            g.drawRoundedRectangle (track, 3.0f, 1.0f);

            // Coloured drawbar (grows from bottom like a pulled bar)
            const auto barTop = sliderPos;
            auto bar = track.withTop (barTop).withBottom (track.getBottom()).reduced (2.0f, 0.0f);

            // Classic organ drawbar colours cycle by component order heuristic
            const auto hue = 0.08f + 0.12f * (float) (slider.getName().getIntValue() % 5);
            juce::Colour barColour = juce::Colour::fromHSV (hue, 0.65f, 0.75f, 1.0f);
            if (slider.getName().containsIgnoreCase ("1"))
                barColour = juce::Colour (0xffc0392b); // brown/red
            else if (slider.getName().containsIgnoreCase ("2"))
                barColour = juce::Colour (0xfff1c40f); // white/yellow
            else if (slider.getName().containsIgnoreCase ("3"))
                barColour = juce::Colour (0xff2980b9); // black/blue-ish
            else
                barColour = amber();

            juce::ColourGradient grad (barColour.brighter (0.25f), bar.getX(), bar.getY(),
                                       barColour.darker (0.35f), bar.getRight(), bar.getBottom(), false);
            g.setGradientFill (grad);
            g.fillRoundedRectangle (bar, 2.0f);

            // Grip lines on the "handle" at top of bar
            g.setColour (juce::Colours::black.withAlpha (0.35f));
            for (int i = 0; i < 3; ++i)
            {
                const float gy = bar.getY() + 4.0f + (float) i * 4.0f;
                if (gy < bar.getBottom() - 2.0f)
                    g.drawLine (bar.getX() + 3.0f, gy, bar.getRight() - 3.0f, gy, 1.0f);
            }

            juce::ignoreUnused (minSliderPos, maxSliderPos);
            return;
        }

        LookAndFeel_V4::drawLinearSlider (g, x, y, width, height, sliderPos,
                                          minSliderPos, maxSliderPos, style, slider);
    }

    //==============================================================================
    void drawButtonBackground (juce::Graphics& g, juce::Button& button,
                               const juce::Colour& backgroundColour,
                               bool shouldDrawButtonAsHighlighted,
                               bool shouldDrawButtonAsDown) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced (1.0f);
        auto base = backgroundColour;

        if (shouldDrawButtonAsDown)
            base = base.darker (0.15f);
        else if (shouldDrawButtonAsHighlighted)
            base = base.brighter (0.08f);

        // Plastic bevel
        g.setColour (juce::Colours::black.withAlpha (0.35f));
        g.fillRoundedRectangle (bounds.translated (0.0f, 1.5f), 4.0f);

        juce::ColourGradient plastic (base.brighter (0.2f), bounds.getX(), bounds.getY(),
                                      base.darker (0.15f), bounds.getX(), bounds.getBottom(), false);
        g.setGradientFill (plastic);
        g.fillRoundedRectangle (bounds, 4.0f);

        g.setColour (goldDim().withAlpha (0.55f));
        g.drawRoundedRectangle (bounds, 4.0f, 1.0f);

        if (shouldDrawButtonAsDown)
        {
            g.setColour (juce::Colours::black.withAlpha (0.15f));
            g.fillRoundedRectangle (bounds, 4.0f);
        }
    }

    //==============================================================================
    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                           bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        juce::ignoreUnused (shouldDrawButtonAsDown);

        auto bounds = button.getLocalBounds().toFloat();
        const auto tick = bounds.removeFromLeft (22.0f).withSizeKeepingCentre (18.0f, 18.0f);

        // Rockers / pilot lamp style
        g.setColour (panelInset());
        g.fillEllipse (tick);
        g.setColour (goldDim());
        g.drawEllipse (tick, 1.2f);

        if (button.getToggleState())
        {
            g.setColour (amberGlow().withAlpha (0.35f));
            g.fillEllipse (tick.expanded (3.0f));
            g.setColour (amber());
            g.fillEllipse (tick.reduced (3.0f));
        }
        else
        {
            g.setColour (juce::Colour (0xff3a3020));
            g.fillEllipse (tick.reduced (3.0f));
        }

        g.setColour (button.findColour (juce::ToggleButton::textColourId));
        g.setFont (juce::Font (juce::FontOptions (13.0f)));
        g.drawFittedText (button.getButtonText(),
                          button.getLocalBounds().withTrimmedLeft (26),
                          juce::Justification::centredLeft, 1);

        juce::ignoreUnused (shouldDrawButtonAsHighlighted);
    }

    //==============================================================================
    void drawComboBox (juce::Graphics& g, int width, int height, bool isButtonDown,
                       int buttonX, int buttonY, int buttonW, int buttonH,
                       juce::ComboBox& box) override
    {
        juce::ignoreUnused (isButtonDown, buttonX, buttonY, buttonW, buttonH, box);

        auto bounds = juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height);

        g.setColour (panelInset());
        g.fillRoundedRectangle (bounds, 3.0f);

        // Amber LED readout feel
        g.setColour (juce::Colour (0xff1a1208));
        g.fillRoundedRectangle (bounds.reduced (2.0f), 2.0f);

        g.setColour (goldDim().withAlpha (0.7f));
        g.drawRoundedRectangle (bounds, 3.0f, 1.0f);

        juce::Path arrow;
        const float cx = (float) width - 14.0f;
        const float cy = (float) height * 0.5f;
        arrow.addTriangle (cx - 4.0f, cy - 2.0f, cx + 4.0f, cy - 2.0f, cx, cy + 4.0f);
        g.setColour (gold());
        g.fillPath (arrow);
    }
};
