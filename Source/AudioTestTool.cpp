#include "YanoSwingProcessor.h"
#include <iostream>
#include <vector>

// Dev-only headless sanity check for the MIDI groove grid. Yano Swing has
// no audio to render (it's a MIDI effect), so this tool -- unlike the
// AudioTestTool in the other Yano/Montagem plugins -- drives processBlock()
// with a mock AudioPlayHead and a straight 8-note 16th-note pattern, then
// verifies: on-grid (even) 16ths pass through unswung, off-grid (odd)
// 16ths are delayed by the expected amount, note-offs get the exact same
// delay as their note-on (duration preserved), and no notes are dropped.
namespace
{
    struct TestPlayHead : public juce::AudioPlayHead
    {
        double bpm = 120.0;
        double ppq = 0.0;

        juce::Optional<PositionInfo> getPosition() const override
        {
            PositionInfo info;
            info.setBpm(bpm);
            info.setPpqPosition(ppq);
            info.setIsPlaying(true);
            return info;
        }
    };

    struct CapturedEvent
    {
        juce::MidiMessage message;
        juce::int64 absoluteSample;
    };
}

int main()
{
    const double sr = 44100.0;
    const double bpm = 120.0;
    const float amount = 1.0f; // max swing -- largest, easiest-to-check delay

    const double sixteenthSeconds = (60.0 / bpm) / 4.0;
    const int blockSize = (int) std::llround(sixteenthSeconds * sr);

    YanoSwingProcessor processor;
    TestPlayHead playHead;
    playHead.bpm = bpm;
    processor.setPlayHead(&playHead);
    processor.setPlayConfigDetails(0, 0, sr, blockSize);
    processor.prepareToPlay(sr, blockSize);
    processor.apvts.getParameter("amount")->setValueNotifyingHost(amount);

    const int numNotes = 8; // two beats of straight 16ths
    juce::int64 absoluteSample = 0;
    std::vector<CapturedEvent> captured;
    juce::AudioBuffer<float> dummyBuffer(0, blockSize);

    // Feed one 16th-note block per note, plus trailing blocks so any
    // swung-late events (up to 0.5 of a 16th) have a chance to flush.
    for (int i = 0; i < numNotes + 4; ++i)
    {
        playHead.ppq = i * 0.25; // 1 sixteenth-note = 0.25 ppq

        juce::MidiBuffer midi;
        if (i < numNotes)
        {
            midi.addEvent(juce::MidiMessage::noteOn(1, 60, (juce::uint8) 100), 0);
            midi.addEvent(juce::MidiMessage::noteOff(1, 60), blockSize / 2);
        }

        processor.processBlock(dummyBuffer, midi);

        for (const auto metadata : midi)
            captured.push_back({ metadata.getMessage(), absoluteSample + metadata.samplePosition });

        absoluteSample += blockSize;
    }

    bool ok = true;

    const int expectedEvents = numNotes * 2;
    std::cout << "captured " << captured.size() << " events, expected " << expectedEvents << "\n";
    bool countOk = (int) captured.size() == expectedEvents;
    std::cout << (countOk ? "[PASS] " : "[FAIL] ") << "no notes dropped or duplicated\n";
    ok &= countOk;

    for (int i = 0; i < numNotes; ++i)
    {
        const double ppqAtNote = i * 0.25;
        const juce::int64 expectedDelay = (juce::int64) std::llround(
            YanoSwingProcessor::computeSwingDelaySamples(ppqAtNote, bpm, sr, amount));
        const bool isOffGrid = (i % 2 != 0);

        // Find the note-on originally scheduled at input sample i*blockSize.
        const juce::int64 inputSample = (juce::int64) i * blockSize;
        auto it = std::find_if(captured.begin(), captured.end(), [&](const CapturedEvent& e) {
            return e.message.isNoteOn() && e.absoluteSample == inputSample + expectedDelay;
        });

        bool found = it != captured.end();
        std::cout << "note " << i << " (" << (isOffGrid ? "off-grid" : "on-grid") << "): expected delay "
                   << expectedDelay << " samples -- " << (found ? "found" : "MISSING") << "\n";
        ok &= found;

        if (found)
        {
            // Matching note-off should carry the exact same delay.
            const juce::int64 inputOffSample = inputSample + blockSize / 2;
            auto offIt = std::find_if(captured.begin(), captured.end(), [&](const CapturedEvent& e) {
                return e.message.isNoteOff() && e.absoluteSample == inputOffSample + expectedDelay;
            });
            bool offFound = offIt != captured.end();
            std::cout << "  matching note-off: " << (offFound ? "found (same delay)" : "MISSING/wrong delay") << "\n";
            ok &= offFound;
        }
    }

    // Sanity: the off-grid delay should be strictly greater than zero at
    // amount=1, at most half a 16th-note (the designed max), and strictly
    // less than a full 16th-note so it never reaches/overtakes the next
    // on-grid hit one block later.
    const juce::int64 offGridDelay = (juce::int64) std::llround(
        YanoSwingProcessor::computeSwingDelaySamples(0.25, bpm, sr, amount));
    const juce::int64 onGridDelay = (juce::int64) std::llround(
        YanoSwingProcessor::computeSwingDelaySamples(0.0, bpm, sr, amount));
    std::cout << "on-grid delay=" << onGridDelay << " samples, off-grid delay=" << offGridDelay
               << " samples (block=" << blockSize << " samples)\n";
    bool delayShapeOk = onGridDelay == 0 && offGridDelay > 0 && offGridDelay < blockSize;
    std::cout << (delayShapeOk ? "[PASS] " : "[FAIL] ")
               << "on-grid stays put, off-grid delayed but never overtakes the next hit\n";
    ok &= delayShapeOk;

    std::cout << (ok ? "\nALL TESTS PASSED\n" : "\nSOME TESTS FAILED\n");
    return ok ? 0 : 1;
}
