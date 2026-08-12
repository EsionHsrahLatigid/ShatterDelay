#include "ParameterGridEditor.h"

#include <ehl/yup_plugin_ui/EhlPluginTheme.h>
#include "ShatterDelayPlugin.h"

#include <algorithm>

namespace shatterdelay::plugin
{
ParameterGridEditor::ParameterGridEditor (yup::AudioProcessor& processor,
                                          yup::StringRef newTitle,
                                          yup::StringRef newWarning,
                                          std::uint32_t newAccentColor)
    : title (newTitle)
    , warning (newWarning)
{
    (void) newAccentColor;
    shatterdelayProcessor = dynamic_cast<ShatterDelayPlugin*> (&processor);

    const auto processorParameters = processor.getParameters();
    parameters.assign (processorParameters.begin(), processorParameters.end());

    titleLabel = std::make_unique<yup::Label>();
    titleLabel->setText (title, yup::dontSendNotification);
    titleLabel->setJustification (yup::Justification::centerLeft);
    ehl::ui::styleLabel (*titleLabel, ehl::ui::TextRole::primary);
    addAndMakeVisible (*titleLabel);

    warningLabel = std::make_unique<yup::Label>();
    warningLabel->setText (warning, yup::dontSendNotification);
    warningLabel->setJustification (yup::Justification::centerLeft);
    ehl::ui::styleLabel (*warningLabel, ehl::ui::TextRole::secondary);
    addAndMakeVisible (*warningLabel);

    labels.reserve (parameters.size());
    sliders.reserve (parameters.size());
    valueLabels.reserve (parameters.size());

    for (const auto& parameter : parameters)
    {
        auto label = std::make_unique<yup::Label>();
        label->setText (parameter->getName(), yup::dontSendNotification);
        label->setJustification (yup::Justification::center);
        ehl::ui::styleLabel (*label, ehl::ui::TextRole::secondary);
        addAndMakeVisible (*label);
        labels.push_back (std::move (label));

        auto slider = std::make_unique<ehl::ui::PixelSlider> (yup::Slider::RotaryVerticalDrag);
        slider->setRange (parameter->getMinimumValue(),
                          parameter->getMaximumValue(),
                          parameter->isStepped() ? 1.0 : 0.0);
        slider->setDefaultValue (parameter->getDefaultValue());
        slider->setValue (parameter->getValue(), yup::dontSendNotification);
        slider->setTextBoxStyle (yup::Slider::NoTextBox);
        slider->setPopupDisplayEnabled (false);
        slider->setMouseCursor (yup::MouseCursor::Hand);
        slider->setClickingGrabFocus (false);
        slider->onDragStart = [parameter] (const yup::MouseEvent&) { parameter->beginChangeGesture(); };
        slider->onValueChanged = [parameter] (double value)
        {
            parameter->setValueNotifyingHost (static_cast<float> (value));
        };
        slider->onDragEnd = [parameter] (const yup::MouseEvent&) { parameter->endChangeGesture(); };
        addAndMakeVisible (*slider);
        sliders.push_back (std::move (slider));

        auto valueLabel = std::make_unique<yup::Label>();
        valueLabel->setText (parameter->toString(), yup::dontSendNotification);
        valueLabel->setJustification (yup::Justification::center);
        ehl::ui::styleLabel (*valueLabel, ehl::ui::TextRole::primary);
        addAndMakeVisible (*valueLabel);
        valueLabels.push_back (std::move (valueLabel));
    }

#if defined(YUP_AUDIO_PLUGIN_ENABLE_STANDALONE)
    if (shatterdelayProcessor != nullptr)
    {
        auditionButton = std::make_unique<ehl::ui::CommandButton>();
        auditionButton->setMouseCursor (yup::MouseCursor::Hand);
        auditionButton->setClickingGrabFocus (false);
        auditionButton->onClick = [this]
        {
            shatterdelayProcessor->setAuditionEnabled (! shatterdelayProcessor->isAuditionEnabled());
            syncAuditionControls();
        };
        addAndMakeVisible (*auditionButton);

        auditionTypeButton = std::make_unique<ehl::ui::CommandButton>();
        auditionTypeButton->setMouseCursor (yup::MouseCursor::Hand);
        auditionTypeButton->setClickingGrabFocus (false);
        auditionTypeButton->onClick = [this]
        {
            shatterdelayProcessor->setAuditionType (1 - shatterdelayProcessor->getAuditionType());
            syncAuditionControls();
        };
        addAndMakeVisible (*auditionTypeButton);

        inputMeterLabel = std::make_unique<yup::Label>();
        inputMeterLabel->setText ("In", yup::dontSendNotification);
        inputMeterLabel->setJustification (yup::Justification::centerLeft);
        ehl::ui::styleLabel (*inputMeterLabel, ehl::ui::TextRole::secondary);
        addAndMakeVisible (*inputMeterLabel);

        outputMeterLabel = std::make_unique<yup::Label>();
        outputMeterLabel->setText ("Out", yup::dontSendNotification);
        outputMeterLabel->setJustification (yup::Justification::centerLeft);
        ehl::ui::styleLabel (*outputMeterLabel, ehl::ui::TextRole::secondary);
        addAndMakeVisible (*outputMeterLabel);

        inputMeter = std::make_unique<ehl::ui::StripMeter> (ehl::ui::mid);
        outputMeter = std::make_unique<ehl::ui::StripMeter> (ehl::ui::paper);
        addAndMakeVisible (*inputMeter);
        addAndMakeVisible (*outputMeter);
        syncAuditionControls();
    }
#endif

    setSize (getPreferredSize().to<float>());
    startTimerHz (30);
}

ParameterGridEditor::~ParameterGridEditor()
{
}

bool ParameterGridEditor::isResizable() const
{
    return true;
}

bool ParameterGridEditor::shouldPreserveAspectRatio() const
{
    return true;
}

yup::Size<int> ParameterGridEditor::getPreferredSize() const
{
    return ehl::ui::preferredSize;
}

void ParameterGridEditor::paint (yup::Graphics& graphics)
{
    ehl::ui::paintEditorBackground (graphics, getWidth(), getHeight());
}

void ParameterGridEditor::resized()
{
    constexpr int columns = 7;
    constexpr float margin = 16.0f;
    constexpr float top = 128.0f;
    constexpr float gap = 8.0f;
    constexpr float labelHeight = 24.0f;
    constexpr float valueHeight = 24.0f;
    constexpr float controlSize = 72.0f;

    const auto bounds = getLocalBounds();
    const auto cellWidth = (bounds.getWidth() - 2.0f * margin - gap * (columns - 1)) / columns;
    const auto rows = 1;
    const auto availableHeight = bounds.getHeight() - top - margin;
    const auto cellHeight = (availableHeight - gap * (rows - 1)) / rows;

    titleLabel->setBounds (20.0f, 8.0f, bounds.getWidth() - 40.0f, 28.0f);
    warningLabel->setBounds (20.0f, 36.0f, bounds.getWidth() - 40.0f, 20.0f);

#if defined(YUP_AUDIO_PLUGIN_ENABLE_STANDALONE)
    if (auditionButton != nullptr && auditionTypeButton != nullptr && inputMeter != nullptr && outputMeter != nullptr)
    {
        constexpr float buttonWidth = 104.0f;
        constexpr float typeWidth = 80.0f;
        constexpr float controlHeight = 28.0f;
        const auto meterX = margin + buttonWidth + typeWidth + gap * 2.0f;
        const auto meterWidth = std::max (90.0f, (bounds.getWidth() - margin - meterX - gap) * 0.5f);

        auditionButton->setBounds (margin, 72.0f, buttonWidth, controlHeight);
        auditionTypeButton->setBounds (margin + buttonWidth + gap, 72.0f, typeWidth, controlHeight);
        inputMeterLabel->setBounds (meterX, 68.0f, 28.0f, 16.0f);
        inputMeter->setBounds (meterX + 28.0f, 76.0f, meterWidth - 28.0f, 12.0f);
        outputMeterLabel->setBounds (meterX + meterWidth + gap, 68.0f, 32.0f, 16.0f);
        outputMeter->setBounds (meterX + meterWidth + gap + 32.0f, 76.0f, meterWidth - 32.0f, 12.0f);
    }
#endif

    for (std::size_t i = 0; i < sliders.size(); ++i)
    {
        const auto column = static_cast<int> (i) % columns;
        const auto row = static_cast<int> (i) / columns;
        const auto x = margin + column * (cellWidth + gap);
        const auto y = top + row * (cellHeight + gap);
        const auto fittedControlSize = std::min (controlSize, cellWidth - 8.0f);
        const auto controlX = x + 0.5f * (cellWidth - fittedControlSize);
        const auto controlY = y + 52.0f;

        labels[i]->setBounds (x + 2.0f, y + 12.0f, cellWidth - 4.0f, labelHeight);
        sliders[i]->setBounds (controlX, controlY, fittedControlSize, fittedControlSize);
        valueLabels[i]->setBounds (x + 2.0f, y + cellHeight - valueHeight - 28.0f, cellWidth - 4.0f, valueHeight);
    }
}

void ParameterGridEditor::focusLost()
{
    yup::AudioProcessorEditor::focusLost();
}

void ParameterGridEditor::keyDown (const yup::KeyPress& key, const yup::Point<float>& position)
{
    yup::AudioProcessorEditor::keyDown (key, position);

}

void ParameterGridEditor::keyUp (const yup::KeyPress& key, const yup::Point<float>& position)
{
    yup::AudioProcessorEditor::keyUp (key, position);

}

void ParameterGridEditor::timerCallback()
{
    for (std::size_t i = 0; i < sliders.size(); ++i)
    {
        if (! sliders[i]->isCurrentlyBeingDragged())
            sliders[i]->setValue (parameters[i]->getValue(), yup::dontSendNotification);
        valueLabels[i]->setText (parameters[i]->toString(), yup::dontSendNotification);
    }

#if defined(YUP_AUDIO_PLUGIN_ENABLE_STANDALONE)
    if (shatterdelayProcessor != nullptr && inputMeter != nullptr && outputMeter != nullptr)
    {
        const auto latestInputPeak = shatterdelayProcessor->getInputPeakLevel();
        const auto latestPeak = shatterdelayProcessor->getOutputPeakLevel();
        displayedInputPeak = std::max (latestInputPeak, displayedInputPeak * 0.82f);
        displayedPeak = std::max (latestPeak, displayedPeak * 0.82f);
        static_cast<ehl::ui::StripMeter*> (inputMeter.get())->setLevel (displayedInputPeak);
        static_cast<ehl::ui::StripMeter*> (outputMeter.get())->setLevel (displayedPeak);
    }
#endif
}

void ParameterGridEditor::syncAuditionControls()
{
#if defined(YUP_AUDIO_PLUGIN_ENABLE_STANDALONE)
    if (shatterdelayProcessor == nullptr || auditionButton == nullptr || auditionTypeButton == nullptr)
        return;

    auditionButton->setButtonText (shatterdelayProcessor->isAuditionEnabled() ? "Audition On" : "Audition Off");
    auditionTypeButton->setButtonText (shatterdelayProcessor->getAuditionType() == 0 ? "Saw" : "Pulse");
    static_cast<ehl::ui::CommandButton*> (auditionButton.get())->setSelected (shatterdelayProcessor->isAuditionEnabled());
    static_cast<ehl::ui::CommandButton*> (auditionTypeButton.get())->setSelected (shatterdelayProcessor->getAuditionType() != 0);
#endif
}

} // namespace shatterdelay::plugin
