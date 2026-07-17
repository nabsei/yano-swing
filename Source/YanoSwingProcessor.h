#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <map>
#include <vector>

// One-knob "swing" processor for amapiano production -- the fourth piece of
// the Yano line, after Yano Log, Yano Finish, and Yano Space. A completely
// different kind of tool from the other three: not an audio effect, but a
// MIDI groove processor. Amapiano's laid-back feel comes from pushing the
// off-grid 16th notes (the "and" of each 8th) noticeably late relative to
// the beat, rather than the phonk/trap chain's audio-domain drive/width/
// space treatment -- so this has to work on note timing, not the waveform.
//
// Every incoming note-on is classified against a 16th-note grid derived
// from the host's ppq position: on-grid (even) 16ths pass straight through,
// off-grid (odd) 16ths get delayed by an amount proportional to the single
// Amount macro, up to a fraction of a 16th note. Matching note-offs are
// delayed by the exact same amount as their note-on so note durations are
// preserved. Non-note messages (CC, pitch bend, etc.) pass through
// immediately, unswung -- only note timing is groove-quantized in v0.1.
class YanoSwingProcessor : public juce::AudioProcessor
{
public:
    YanoSwingProcessor();
    ~YanoSwingProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout&) const override { return true; }
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Yano Swing"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return true; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;

    // Exposed for the dev-only test tool: computes the swing delay (in
    // samples) that would be applied to a note-on at the given absolute
    // ppq position, given the current Amount value. Pulled out of
    // processBlock so tests can check the grid math directly without
    // needing a real playhead/block loop.
    static double computeSwingDelaySamples(double ppqPosition, double bpm, double sampleRate, float amount01);

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    double lastSampleRate = 44100.0;
    juce::int64 blockStartAbsoluteSample = 0; // running clock, advances every processBlock call
    juce::int64 freeRunningSampleCounter = 0; // fallback timeline when no host playhead is available

    struct PendingEvent
    {
        juce::MidiMessage message;
        juce::int64 targetAbsoluteSample;
    };
    std::vector<PendingEvent> pending;

    // Tracks the delay applied to each currently-held note-on (keyed by
    // channel*128 + noteNumber) so the matching note-off gets the exact
    // same delay -- never a different one that could reorder or overlap.
    std::map<int, juce::int64> activeNoteDelays;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(YanoSwingProcessor)
};
