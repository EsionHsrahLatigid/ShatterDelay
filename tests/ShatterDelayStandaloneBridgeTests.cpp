#include "ShatterDelayPlugin.h"

#include <yup_audio_processors/yup_audio_processors.h>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>

namespace
{
constexpr int numChannels = 2;
constexpr int blockSamples = 4096;

class PluginHarness
{
public:
    PluginHarness()
        : audio (numChannels, blockSamples)
        , context { audio, midi, automation, nullptr, {}, {} }
    {
        plugin.prepareToPlay (yup::AudioSpec (48000.0f, blockSamples, numChannels));
    }

    float processSilenceAndReturnRms()
    {
        audio.clear();
        plugin.processBlock (context);

        float energy = 0.0f;
        for (int channel = 0; channel < audio.getNumChannels(); ++channel)
        {
            const auto* samples = audio.getReadPointer (channel);
            for (int sample = 0; sample < audio.getNumSamples(); ++sample)
                energy += samples[sample] * samples[sample];
        }
        return std::sqrt (energy / static_cast<float> (audio.getNumChannels() * audio.getNumSamples()));
    }

    shatterdelay::plugin::ShatterDelayPlugin plugin;

private:
    yup::AudioBuffer<float> audio;
    yup::MidiBuffer midi;
    yup::ParameterChangeBuffer automation;
    yup::AudioProcessContext<float> context;
};

void testDefaultStandaloneAuditionRendersAndMeters()
{
    PluginHarness harness;
    assert (harness.plugin.isAuditionEnabled());

    const auto rms = harness.processSilenceAndReturnRms();

    assert (rms >= 1.0e-4f);
    assert (harness.plugin.getInputPeakLevel() > 0.0f);
    assert (harness.plugin.getOutputPeakLevel() > 0.0f);
}

void testStandaloneAuditionCanBeDisabled()
{
    PluginHarness harness;
    harness.plugin.setAuditionEnabled (false);
    assert (! harness.plugin.isAuditionEnabled());

    const auto rms = harness.processSilenceAndReturnRms();
    assert (rms <= 1.0e-7f);
}

void testStandaloneAuditionTypeChangesSignal()
{
    PluginHarness saw;
    saw.plugin.setAuditionType (0);
    const auto sawRms = saw.processSilenceAndReturnRms();

    PluginHarness pulse;
    pulse.plugin.setAuditionType (1);
    const auto pulseRms = pulse.processSilenceAndReturnRms();

    assert (std::fabs (sawRms - pulseRms) > 1.0e-4f);
}

void testAuditionStateIsNotSerialized()
{
    PluginHarness source;
    source.plugin.setAuditionEnabled (false);
    source.plugin.setAuditionType (1);
    yup::MemoryBlock data;
    assert (source.plugin.saveStateIntoMemory (data).wasOk());

    PluginHarness target;
    assert (target.plugin.loadStateFromMemory (data).wasOk());
    assert (target.plugin.isAuditionEnabled());
    assert (target.plugin.getAuditionType() == 0);
}

} // namespace

int main()
{
    testDefaultStandaloneAuditionRendersAndMeters();
    testStandaloneAuditionCanBeDisabled();
    testStandaloneAuditionTypeChangesSignal();
    testAuditionStateIsNotSerialized();

    std::cout << "ShatterDelayStandaloneBridgeTests passed\n";
    return 0;
}
