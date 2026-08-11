#include "shatterdelay/ShatterDelayEngine.h"

#include <algorithm>
#include <cmath>

namespace shatterdelay
{
namespace
{
constexpr float ceiling = 0.98f;
constexpr float twoPi = 6.2831853071795864769f;
}

ShatterDelayEngine::ShatterDelayEngine()
    : delayLeft (std::make_unique<DelayBuffer>()),
      delayRight (std::make_unique<DelayBuffer>())
{
    prepare (44100.0);
    reset();
}

void ShatterDelayEngine::prepare (double newSampleRate) noexcept
{
    sampleRate = std::isfinite (newSampleRate) && newSampleRate > 1.0 ? newSampleRate : 44100.0;
    reset();
}

void ShatterDelayEngine::reset() noexcept
{
    delayLeft->fill (0.0f);
    delayRight->fill (0.0f);
    writeIndex = 0;
    dampLeft = 0.0f;
    dampRight = 0.0f;
    driftPhase = 0.0f;
    crackleNoise.reset (0x51f27e4du);
}

void ShatterDelayEngine::setParameters (const ShatterDelayParameters& parameters) noexcept
{
    params.time = clampFinite (parameters.time, 0.0f, 1.0f, ShatterDelayParameters {}.time);
    params.feedback = clampFinite (parameters.feedback, 0.0f, 1.0f, ShatterDelayParameters {}.feedback);
    params.shatter = clampFinite (parameters.shatter, 0.0f, 1.0f, ShatterDelayParameters {}.shatter);
    params.damping = clampFinite (parameters.damping, 0.0f, 1.0f, ShatterDelayParameters {}.damping);
    params.drift = clampFinite (parameters.drift, 0.0f, 1.0f, ShatterDelayParameters {}.drift);
    params.mix = clampFinite (parameters.mix, 0.0f, 1.0f, ShatterDelayParameters {}.mix);
    params.output = clampFinite (parameters.output, 0.0f, 2.0f, ShatterDelayParameters {}.output);
}

StereoFrame ShatterDelayEngine::processSample (float inputLeft, float inputRight) noexcept
{
    const auto dryLeft = sanitizeAudio (inputLeft);
    const auto dryRight = sanitizeAudio (inputRight);

    driftPhase += (0.035f + params.drift * 0.19f) / static_cast<float> (sampleRate);
    if (driftPhase >= 1.0f)
        driftPhase -= 1.0f;

    const auto baseDelay = (0.008f + params.time * params.time * 0.94f) * static_cast<float> (sampleRate);
    const auto driftSamples = std::sin (driftPhase * twoPi) * params.drift * (3.0f + 21.0f * params.time);
    const auto leftDelay = std::clamp (baseDelay + driftSamples, 2.0f, static_cast<float> (maxDelaySamples - 4));
    const auto rightDelay = std::clamp (baseDelay * 1.137f - driftSamples * 0.71f, 2.0f, static_cast<float> (maxDelaySamples - 4));

    auto delayedLeft = readDelay (*delayLeft, leftDelay);
    auto delayedRight = readDelay (*delayRight, rightDelay);

    const auto dampAmount = 0.08f + params.damping * 0.88f;
    dampLeft += dampAmount * (delayedLeft - dampLeft);
    dampRight += dampAmount * (delayedRight - dampRight);
    delayedLeft = dampLeft;
    delayedRight = dampRight;

    const auto activity = std::max ({ std::fabs (dryLeft), std::fabs (dryRight), std::fabs (delayedLeft), std::fabs (delayedRight) });
    const auto sparseHit = (crackleNoise.nextWord() & 4095u) < static_cast<std::uint32_t> (2u + params.shatter * 38.0f);
    const auto crackle = activity > 1.0e-5f && sparseHit ? crackleNoise.nextBinary() * params.shatter * activity * 0.13f : 0.0f;

    const auto foldedLeft = fold (delayedLeft + crackle);
    const auto foldedRight = fold (delayedRight - crackle * 0.73f);
    const auto feedback = params.feedback * (0.12f + 0.80f * (1.0f - params.shatter * 0.18f));
    (*delayLeft)[static_cast<std::size_t> (writeIndex)] = sanitizeAudio (dryLeft + foldedRight * feedback);
    (*delayRight)[static_cast<std::size_t> (writeIndex)] = sanitizeAudio (dryRight + foldedLeft * feedback);
    writeIndex = (writeIndex + 1) % maxDelaySamples;

    const auto wetLeft = fold (foldedLeft + delayedRight * params.shatter * 0.16f);
    const auto wetRight = fold (foldedRight + delayedLeft * params.shatter * 0.16f);
    const auto dry = 1.0f - params.mix;
    return sanitizeFrame ((dryLeft * dry + wetLeft * params.mix) * params.output,
                          (dryRight * dry + wetRight * params.mix) * params.output);
}

void ShatterDelayEngine::process (float* left, float* right, int numSamples) noexcept
{
    if (left == nullptr || right == nullptr || numSamples <= 0)
        return;

    for (int i = 0; i < numSamples; ++i)
    {
        const auto frame = processSample (left[i], right[i]);
        left[i] = frame.left;
        right[i] = frame.right;
    }
}

float ShatterDelayEngine::readDelay (const DelayBuffer& buffer, float delaySamples) const noexcept
{
    const auto integral = static_cast<int> (delaySamples);
    const auto fraction = delaySamples - static_cast<float> (integral);
    auto indexA = writeIndex - integral;
    while (indexA < 0)
        indexA += maxDelaySamples;
    const auto indexB = (indexA - 1 + maxDelaySamples) % maxDelaySamples;
    return buffer[static_cast<std::size_t> (indexA)] * (1.0f - fraction)
         + buffer[static_cast<std::size_t> (indexB)] * fraction;
}

float ShatterDelayEngine::fold (float input) const noexcept
{
    const auto limit = 0.55f + (1.0f - params.shatter) * 0.65f;
    auto value = sanitizeAudio (input);
    if (std::fabs (value) > limit)
    {
        const auto sign = value < 0.0f ? -1.0f : 1.0f;
        value = sign * (limit - std::fmod (std::fabs (value) - limit, limit * 0.72f + 0.05f));
    }
    return boundedDrive (value, 1.0f + params.shatter * 2.4f);
}

float ShatterDelayEngine::sanitizeAudio (float value) const noexcept
{
    return clampFinite (value, -8.0f, 8.0f, 0.0f);
}

StereoFrame ShatterDelayEngine::sanitizeFrame (float left, float right) const noexcept
{
    auto safeLeft = boundedDrive (left, 1.08f + params.shatter * 0.58f);
    auto safeRight = boundedDrive (right, 1.08f + params.shatter * 0.58f);
    if (std::fabs (safeLeft) < 1.0e-20f)
        safeLeft = 0.0f;
    if (std::fabs (safeRight) < 1.0e-20f)
        safeRight = 0.0f;
    return { std::clamp (safeLeft, -ceiling, ceiling),
             std::clamp (safeRight, -ceiling, ceiling) };
}

} // namespace shatterdelay
