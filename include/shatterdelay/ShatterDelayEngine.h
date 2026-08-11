#pragma once

#include "shatterdelay/ShatterDelayDspPrimitives.h"

#include <array>
#include <memory>

namespace shatterdelay
{

struct ShatterDelayParameters
{
    float time = 0.42f;
    float feedback = 0.54f;
    float shatter = 0.36f;
    float damping = 0.48f;
    float drift = 0.22f;
    float mix = 0.55f;
    float output = 0.86f;
};

class ShatterDelayEngine
{
public:
    ShatterDelayEngine();

    void prepare (double sampleRate) noexcept;
    void reset() noexcept;
    void setParameters (const ShatterDelayParameters& parameters) noexcept;
    [[nodiscard]] StereoFrame processSample (float inputLeft, float inputRight) noexcept;
    void process (float* left, float* right, int numSamples) noexcept;

private:
    static constexpr int maxDelaySamples = 192000;
    using DelayBuffer = std::array<float, maxDelaySamples>;

    struct ClampedParameters
    {
        float time = 0.42f;
        float feedback = 0.54f;
        float shatter = 0.36f;
        float damping = 0.48f;
        float drift = 0.22f;
        float mix = 0.55f;
        float output = 0.86f;
    };

    [[nodiscard]] float readDelay (const DelayBuffer& buffer, float delaySamples) const noexcept;
    [[nodiscard]] float fold (float input) const noexcept;
    [[nodiscard]] float sanitizeAudio (float value) const noexcept;
    [[nodiscard]] StereoFrame sanitizeFrame (float left, float right) const noexcept;

    ClampedParameters params;
    double sampleRate = 44100.0;
    std::unique_ptr<DelayBuffer> delayLeft;
    std::unique_ptr<DelayBuffer> delayRight;
    int writeIndex = 0;
    float dampLeft = 0.0f;
    float dampRight = 0.0f;
    float driftPhase = 0.0f;
    DeterministicNoise crackleNoise;
};

} // namespace shatterdelay
