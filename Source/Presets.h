#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>
#include <utility>

struct FactoryPreset
{
    juce::String name;
    // parameter id -> value in real units (choice/int/bool use their index/value)
    std::vector<std::pair<juce::String, float>> values;
    // optional embedded sample (resource name, e.g. "Live_Bass_1_flac")
    juce::String sample {};
};

const std::vector<FactoryPreset>& getFactoryPresets();

// Resets every parameter to its default and then applies the preset values.
void applyPreset (juce::AudioProcessorValueTreeState& apvts, const FactoryPreset& preset);
