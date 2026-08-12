#pragma once

#include <yup_gui/yup_gui.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace ehl::ui
{

inline constexpr std::uint32_t ink = 0xff050505u;
inline constexpr std::uint32_t low = 0xff2a2a2au;
inline constexpr std::uint32_t mid = 0xff8a8a86u;
inline constexpr std::uint32_t paper = 0xfff2f2f0u;
inline constexpr std::uint32_t transparent = 0x00000000u;

inline constexpr yup::Size<int> preferredSize { 640, 360 };
inline constexpr float grid = 4.0f;

enum class TextRole
{
    primary,
    secondary
};

inline void styleLabel (yup::Label& label, TextRole role)
{
    label.setColor (yup::Label::Style::textFillColorId,
                    yup::Color (role == TextRole::primary ? paper : mid));
    label.setColor (yup::Label::Style::textStrokeColorId, yup::Color (transparent));
    label.setColor (yup::Label::Style::backgroundColorId, yup::Color (transparent));
    label.setColor (yup::Label::Style::outlineColorId, yup::Color (transparent));
}

class PixelSlider final : public yup::Slider
{
public:
    explicit PixelSlider (SliderType sliderType)
        : yup::Slider (sliderType)
    {
    }

    void paint (yup::Graphics& graphics) override
    {
        const auto bounds = getLocalBounds().to<float>().reduced (grid);
        const auto cell = std::max (
            grid,
            std::floor (std::min (bounds.getWidth(), bounds.getHeight()) / (grid * 7.0f)) * grid);
        const auto side = cell * 7.0f;
        const auto frame = yup::Rectangle<float> {
            bounds.getCenterX() - side * 0.5f,
            bounds.getCenterY() - side * 0.5f,
            side,
            side
        };

        graphics.setFillColor (low);
        graphics.fillRect (frame);
        graphics.setStrokeColor (hasKeyboardFocus() || isMouseOver() ? paper : mid);
        graphics.setStrokeWidth (hasKeyboardFocus() ? 2.0f : 1.0f);
        graphics.strokeRect (frame.reduced (1.0f));

        constexpr std::array<int, 16> ringX { 0, 0, 0, 0, 0, 1, 2, 3, 4, 4, 4, 4, 4, 3, 2, 1 };
        constexpr std::array<int, 16> ringY { 4, 3, 2, 1, 0, 0, 0, 0, 0, 1, 2, 3, 4, 4, 4, 4 };
        constexpr int segmentCount = static_cast<int> (ringX.size());
        const auto activeSegments = std::clamp (
            static_cast<int> (std::round (getValueNormalised() * segmentCount)), 0, segmentCount);
        const auto originX = frame.getX() + cell;
        const auto originY = frame.getY() + cell;
        const auto block = std::max (2.0f, cell - 2.0f);

        for (int index = 0; index < segmentCount; ++index)
        {
            graphics.setFillColor (index < activeSegments ? paper : ink);
            graphics.fillRect (originX + ringX[static_cast<std::size_t> (index)] * cell + 1.0f,
                               originY + ringY[static_cast<std::size_t> (index)] * cell + 1.0f,
                               block,
                               block);
        }

        graphics.setFillColor (isCurrentlyBeingDragged() ? paper : mid);
        graphics.fillRect (frame.getCenterX() - cell * 0.5f + 1.0f,
                           frame.getCenterY() - cell * 0.5f + 1.0f,
                           block,
                           block);
    }
};

class CommandButton final : public yup::TextButton
{
public:
    using yup::TextButton::TextButton;

    void setSelected (bool shouldBeSelected)
    {
        if (selected == shouldBeSelected)
            return;

        selected = shouldBeSelected;
        repaint();
    }

    void paintButton (yup::Graphics& graphics) override
    {
        const auto bounds = getLocalBounds().to<float>();
        const auto active = selected || isButtonDown();
        const auto over = isButtonOver();
        const auto enabled = isEnabled();

        graphics.setFillColor (active ? paper : (over ? mid : low));
        graphics.fillRect (bounds);
        graphics.setStrokeColor (enabled ? (hasKeyboardFocus() ? paper : mid) : low);
        graphics.setStrokeWidth (hasKeyboardFocus() ? 2.0f : 1.0f);
        graphics.strokeRect (bounds.reduced (1.0f));

        graphics.setFillColor (enabled ? (active || over ? ink : paper) : mid);
        graphics.fillFittedText (getStyledText(), getTextBounds());
    }

private:
    bool selected = false;
};

class StripMeter final : public yup::Component
{
public:
    explicit StripMeter (std::uint32_t activeColor)
        : color (activeColor)
    {
    }

    void setLevel (float newLevel)
    {
        level = std::clamp (newLevel, 0.0f, 1.0f);
        repaint();
    }

    void paint (yup::Graphics& graphics) override
    {
        const auto bounds = getLocalBounds().to<float>();
        graphics.setFillColor (ink);
        graphics.fillRect (bounds);

        constexpr int segmentCount = 24;
        const auto activeSegments = std::clamp (
            static_cast<int> (std::round (level * segmentCount)), 0, segmentCount);
        const auto segmentWidth = bounds.getWidth() / static_cast<float> (segmentCount);

        for (int index = 0; index < segmentCount; ++index)
        {
            graphics.setFillColor (index < activeSegments ? color : low);
            graphics.fillRect (bounds.getX() + index * segmentWidth,
                               bounds.getY(),
                               std::max (1.0f, segmentWidth - 2.0f),
                               bounds.getHeight());
        }
    }

private:
    std::uint32_t color = paper;
    float level = 0.0f;
};

inline void paintEditorBackground (yup::Graphics& graphics, float width, float height)
{
    graphics.setFillColor (ink);
    graphics.fillAll();

    graphics.setFillColor (low);
    for (int y = 64; y < static_cast<int> (height); y += 16)
        graphics.fillRect (0.0f, static_cast<float> (y), width, 1.0f);

    graphics.setFillColor (paper);
    graphics.fillRect (0.0f, 0.0f, width, grid);

    graphics.setFillColor (low);
    graphics.fillRect (0.0f, 64.0f, width, 48.0f);
    graphics.setFillColor (paper);
    graphics.fillRect (0.0f, 108.0f, width, grid);

    constexpr int columns = 7;
    constexpr float margin = 16.0f;
    constexpr float gap = 8.0f;
    constexpr float top = 128.0f;
    constexpr float bottom = 16.0f;
    const auto cellWidth = (width - 2.0f * margin - gap * (columns - 1)) / columns;
    const auto cellHeight = height - top - bottom;

    graphics.setStrokeColor (low);
    graphics.setStrokeWidth (1.0f);
    for (int column = 0; column < columns; ++column)
    {
        const auto x = margin + column * (cellWidth + gap);
        graphics.strokeRect (x, top, cellWidth, cellHeight);

        graphics.setFillColor (column % 2 == 0 ? mid : low);
        for (int bit = 0; bit <= column; ++bit)
            graphics.fillRect (x + grid + bit * (grid * 2.0f), height - 24.0f, grid, grid);
    }
}

} // namespace ehl::ui
