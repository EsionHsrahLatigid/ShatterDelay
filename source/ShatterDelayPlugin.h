#pragma once

#include "shatterdelay/ShatterDelayEngine.h"

#include <yup_audio_processors/yup_audio_processors.h>

#include <array>
#include <atomic>
#include <cstdint>

namespace shatterdelay::plugin
{

class ShatterDelayPlugin final : public yup::AudioProcessor
{
public:
    ShatterDelayPlugin();

    void prepareToPlay (const yup::AudioSpec& spec) override;
    void releaseResources() override;
    void processBlock (yup::AudioProcessContext<float>& context) override;
    void flush() override;

    bool acceptsMidi() const noexcept override;
    bool producesMidi() const noexcept override;

    int getCurrentPreset() const noexcept override;
    void setCurrentPreset (int index) noexcept override;
    int getNumPresets() const override;
    yup::String getPresetName (int index) const override;
    void setPresetName (int index, yup::StringRef newName) override;

    yup::Result loadStateFromMemory (const yup::MemoryBlock& data) override;
    yup::Result saveStateIntoMemory (yup::MemoryBlock& data) override;

    bool hasEditor() const override;
    yup::AudioProcessorEditor* createEditor() override;

    [[nodiscard]] float getInputPeakLevel() const noexcept;
    [[nodiscard]] float getOutputPeakLevel() const noexcept;

#if defined(YUP_AUDIO_PLUGIN_ENABLE_STANDALONE)
    void setAuditionEnabled (bool shouldBeEnabled) noexcept;
    [[nodiscard]] bool isAuditionEnabled() const noexcept;
    void setAuditionType (int type) noexcept;
    [[nodiscard]] int getAuditionType() const noexcept;
#endif

private:
    enum ParameterIndex
    {
        time,
        feedback,
        shatter,
        damping,
        drift,
        mix,
        output,
        parameterCount
    };

    static constexpr int parameterUpdateCadenceSamples = 16;

    void advanceParameterHandles (int samplePosition) noexcept;
    void syncParameterValuesFromParameters() noexcept;
    void updateEngineParameters() noexcept;
#if defined(YUP_AUDIO_PLUGIN_ENABLE_STANDALONE)
    [[nodiscard]] StereoFrame renderAuditionFrame() noexcept;
#endif

    std::array<yup::AudioParameter::Ptr, parameterCount> parameters;
    std::array<yup::AudioParameterHandle, parameterCount> parameterHandles;
    std::array<float, parameterCount> currentParameterValues {};
    shatterdelay::ShatterDelayEngine engine;

    int controlUpdateCountdown = 0;
    std::atomic<int> inputPeakMilli { 0 };
    std::atomic<int> outputPeakMilli { 0 };
    std::atomic<int> currentPreset { 0 };

#if defined(YUP_AUDIO_PLUGIN_ENABLE_STANDALONE)
    std::atomic<int> auditionEnabled { 1 };
    std::atomic<int> auditionType { 0 };
    double auditionSampleRate = 44100.0;
    float auditionPhase = 0.0f;
    std::uint32_t auditionNoise = 0x6d2b79f5u;
#endif

    std::array<yup::String, 4> presetNames {
        "Glass Alley",
        "Sparse Fault",
        "Bent Slap",
        "Terminal Drift"
    };
};

} // namespace shatterdelay::plugin
