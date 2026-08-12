#pragma once

#include <yup_audio_processors/yup_audio_processors.h>
#include <yup_gui/yup_gui.h>

#include <cstdint>
#include <memory>
#include <vector>

namespace shatterdelay::plugin
{

class ShatterDelayPlugin;

/** Reusable parameter-grid shell; product DSP and parameter semantics stay processor-owned. */
class ParameterGridEditor final
    : public yup::AudioProcessorEditor
    , private yup::Timer
{
public:
    ParameterGridEditor (yup::AudioProcessor& processor,
                         yup::StringRef title,
                         yup::StringRef warning,
                         std::uint32_t accentColor);
    ~ParameterGridEditor() override;

    bool isResizable() const override;
    bool shouldPreserveAspectRatio() const override;
    yup::Size<int> getPreferredSize() const override;
    void paint (yup::Graphics& graphics) override;
    void resized() override;
    void focusLost() override;
    void keyDown (const yup::KeyPress& key, const yup::Point<float>& position) override;
    void keyUp (const yup::KeyPress& key, const yup::Point<float>& position) override;

private:
    void timerCallback() override;
    void syncAuditionControls();

    yup::String title;
    yup::String warning;
    std::unique_ptr<yup::Label> titleLabel;
    std::unique_ptr<yup::Label> warningLabel;
    std::vector<yup::AudioParameter::Ptr> parameters;
    std::vector<std::unique_ptr<yup::Label>> labels;
    std::vector<std::unique_ptr<yup::Slider>> sliders;
    std::vector<std::unique_ptr<yup::Label>> valueLabels;
#if defined(YUP_AUDIO_PLUGIN_ENABLE_STANDALONE)
    std::unique_ptr<yup::TextButton> auditionButton;
    std::unique_ptr<yup::TextButton> auditionTypeButton;
    std::unique_ptr<yup::Label> inputMeterLabel;
    std::unique_ptr<yup::Label> outputMeterLabel;
    std::unique_ptr<yup::Component> inputMeter;
    std::unique_ptr<yup::Component> outputMeter;
#endif
    ShatterDelayPlugin* shatterdelayProcessor = nullptr;
    float displayedInputPeak = 0.0f;
    float displayedPeak = 0.0f;
};

} // namespace shatterdelay::plugin
