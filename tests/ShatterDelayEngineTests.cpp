#include "shatterdelay/ShatterDelayEngine.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include <vector>

using shatterdelay::ShatterDelayEngine;
using shatterdelay::ShatterDelayParameters;

namespace
{
std::vector<float> renderImpulse (ShatterDelayParameters params, int samples)
{
    ShatterDelayEngine engine;
    engine.prepare (48000.0);
    engine.setParameters (params);
    engine.reset();

    std::vector<float> output;
    output.reserve (static_cast<std::size_t> (samples));
    for (int i = 0; i < samples; ++i)
    {
        const auto input = i == 0 ? 0.85f : 0.0f;
        output.push_back (engine.processSample (input, input * 0.4f).left);
    }
    return output;
}

float energyAfter (const std::vector<float>& samples, int start)
{
    float energy = 0.0f;
    for (std::size_t i = static_cast<std::size_t> (start); i < samples.size(); ++i)
        energy += samples[i] * samples[i];
    return energy;
}

float energyBetween (const std::vector<float>& samples, int start, int end)
{
    float energy = 0.0f;
    const auto safeStart = std::max (0, start);
    const auto safeEnd = std::min (end, static_cast<int> (samples.size()));
    for (int i = safeStart; i < safeEnd; ++i)
        energy += samples[static_cast<std::size_t> (i)] * samples[static_cast<std::size_t> (i)];
    return energy;
}

int nonZeroAfter (const std::vector<float>& samples, int start)
{
    int count = 0;
    for (std::size_t i = static_cast<std::size_t> (start); i < samples.size(); ++i)
        count += std::fabs (samples[i]) > 1.0e-5f ? 1 : 0;
    return count;
}

void testSilenceStaysSilent()
{
    ShatterDelayEngine engine;
    engine.prepare (48000.0);
    engine.reset();

    for (int i = 0; i < 8192; ++i)
    {
        const auto frame = engine.processSample (0.0f, 0.0f);
        assert (std::fabs (frame.left) <= 1.0e-7f);
        assert (std::fabs (frame.right) <= 1.0e-7f);
    }
}

void testTimeMovesEchoPosition()
{
    ShatterDelayParameters shortDelay;
    shortDelay.time = 0.05f;
    shortDelay.feedback = 0.0f;
    shortDelay.mix = 1.0f;

    ShatterDelayParameters longDelay = shortDelay;
    longDelay.time = 0.48f;

    const auto shortOutput = renderImpulse (shortDelay, 18000);
    const auto longOutput = renderImpulse (longDelay, 18000);

    // The short echo lands near 0.008 + 0.05^2 * 0.94 ~= 10.35 ms,
    // while the long echo lands near 224.6 ms. Compare disjoint arrival
    // windows rather than tail integrals that contain both echoes.
    assert (energyBetween (shortOutput, 430, 620) > energyBetween (longOutput, 430, 620) * 8.0f + 1.0e-4f);
    assert (energyBetween (longOutput, 10600, 11050) > energyBetween (shortOutput, 10600, 11050) * 8.0f + 1.0e-4f);
}

void testFeedbackCreatesTailEnergy()
{
    ShatterDelayParameters dry;
    dry.time = 0.08f;
    dry.feedback = 0.0f;
    dry.mix = 1.0f;

    ShatterDelayParameters ringing = dry;
    ringing.feedback = 0.84f;

    const auto dryOutput = renderImpulse (dry, 18000);
    const auto ringingOutput = renderImpulse (ringing, 18000);

    assert (energyAfter (ringingOutput, 8000) > energyAfter (dryOutput, 8000) + 1.0e-4f);
}

void testShatterAddsSparseActivity()
{
    ShatterDelayParameters smooth;
    smooth.time = 0.07f;
    smooth.feedback = 0.72f;
    smooth.shatter = 0.0f;
    smooth.mix = 1.0f;

    ShatterDelayParameters broken = smooth;
    broken.shatter = 1.0f;

    const auto smoothOutput = renderImpulse (smooth, 16000);
    const auto brokenOutput = renderImpulse (broken, 16000);
    assert (std::fabs (energyAfter (brokenOutput, 2000) - energyAfter (smoothOutput, 2000)) > 1.0e-3f);
    assert (nonZeroAfter (brokenOutput, 2000) > nonZeroAfter (smoothOutput, 2000) / 2);
}

void testDeterministic()
{
    ShatterDelayParameters params;
    params.time = 0.23f;
    params.feedback = 0.79f;
    params.shatter = 0.67f;
    params.drift = 0.31f;

    const auto a = renderImpulse (params, 12000);
    const auto b = renderImpulse (params, 12000);
    assert (a.size() == b.size());
    for (std::size_t i = 0; i < a.size(); ++i)
        assert (std::fabs (a[i] - b[i]) <= 1.0e-6f);
}

void testFiniteBoundedExtremeParameters()
{
    ShatterDelayParameters params;
    params.time = 1000.0f;
    params.feedback = 1000.0f;
    params.shatter = 1000.0f;
    params.damping = std::numeric_limits<float>::infinity();
    params.drift = 1000.0f;
    params.mix = 1000.0f;
    params.output = 1000.0f;

    ShatterDelayEngine engine;
    engine.prepare (0.0);
    engine.setParameters (params);
    engine.reset();

    for (int i = 0; i < 8192; ++i)
    {
        const auto frame = engine.processSample (1000.0f, -1000.0f);
        assert (std::isfinite (frame.left));
        assert (std::isfinite (frame.right));
        assert (frame.left >= -0.9801f && frame.left <= 0.9801f);
        assert (frame.right >= -0.9801f && frame.right <= 0.9801f);
    }
}

void testDenormalInputDoesNotLeak()
{
    ShatterDelayEngine engine;
    engine.prepare (48000.0);
    engine.reset();

    for (int i = 0; i < 1024; ++i)
    {
        const auto frame = engine.processSample (1.0e-30f, -1.0e-30f);
        assert (std::fabs (frame.left) <= 1.0e-7f);
        assert (std::fabs (frame.right) <= 1.0e-7f);
    }
}
} // namespace

int main()
{
    testSilenceStaysSilent();
    testTimeMovesEchoPosition();
    testFeedbackCreatesTailEnergy();
    testShatterAddsSparseActivity();
    testDeterministic();
    testFiniteBoundedExtremeParameters();
    testDenormalInputDoesNotLeak();

    std::cout << "ShatterDelayEngineTests passed\n";
    return 0;
}
