#include "YanoSwingProcessor.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <iostream>
#include <vector>

// Dev-only listening aid: Yano Swing has no audio to process directly (it's
// a MIDI effect), so instead of running a WAV through processBlock() like
// the other Yano/Montagem RenderTools, this renders a straight 16th-note
// click pattern twice -- once at the original grid positions, once at the
// positions Yano Swing actually schedules them at -- so the groove can be
// auditioned as audio.
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

    void writeClicksToWav(const juce::File& outFile, const std::vector<juce::int64>& clickSamples,
                           int totalSamples, double sampleRate)
    {
        juce::AudioBuffer<float> buffer(2, totalSamples);
        buffer.clear();

        const double clickFreq = 1200.0;
        const double decaySeconds = 0.015;
        const int clickLenSamples = (int) (decaySeconds * 6.0 * sampleRate);

        for (auto start : clickSamples)
        {
            for (int i = 0; i < clickLenSamples; ++i)
            {
                const juce::int64 s = start + i;
                if (s < 0 || s >= totalSamples)
                    continue;
                const double t = (double) i / sampleRate;
                const float env = (float) std::exp(-t / decaySeconds);
                const float sample = 0.8f * env * (float) std::sin(juce::MathConstants<double>::twoPi * clickFreq * t);
                buffer.addSample(0, (int) s, sample);
                buffer.addSample(1, (int) s, sample);
            }
        }

        outFile.deleteFile();
        std::unique_ptr<juce::FileOutputStream> outStream(outFile.createOutputStream());
        juce::WavAudioFormat wavFormat;
        std::unique_ptr<juce::AudioFormatWriter> writer(
            wavFormat.createWriterFor(outStream.get(), sampleRate, 2, 16, {}, 0));
        if (writer == nullptr)
        {
            std::cout << "could not create writer for " << outFile.getFullPathName() << "\n";
            return;
        }
        outStream.release();
        writer->writeFromAudioSampleBuffer(buffer, 0, totalSamples);
        std::cout << "wrote " << outFile.getFullPathName() << "\n";
    }
}

int main(int argc, char** argv)
{
    if (argc < 4)
    {
        std::cout << "usage: RenderTool straight_out.wav swung_out.wav amount(0-1) [bpm=120] [numNotes=8]\n";
        return 1;
    }

    const juce::File straightOut(argv[1]);
    const juce::File swungOut(argv[2]);
    const float amount = (float) std::atof(argv[3]);
    const double bpm = argc > 4 ? std::atof(argv[4]) : 120.0;
    const int numNotes = argc > 5 ? std::atoi(argv[5]) : 8;

    const double sr = 44100.0;
    const double sixteenthSeconds = (60.0 / bpm) / 4.0;
    const int blockSize = (int) std::llround(sixteenthSeconds * sr);
    const int tailBlocks = 4; // lets any swung-late click flush before we stop
    const int totalSamples = (numNotes + tailBlocks) * blockSize + (int) (sr * 0.3); // + click decay tail

    YanoSwingProcessor processor;
    TestPlayHead playHead;
    playHead.bpm = bpm;
    processor.setPlayHead(&playHead);
    processor.setPlayConfigDetails(0, 0, sr, blockSize);
    processor.prepareToPlay(sr, blockSize);
    processor.apvts.getParameter("amount")->setValueNotifyingHost(amount);

    std::vector<juce::int64> straightClicks;
    std::vector<juce::int64> swungClicks;
    juce::int64 absoluteSample = 0;
    juce::AudioBuffer<float> dummyBuffer(0, blockSize);

    for (int i = 0; i < numNotes + tailBlocks; ++i)
    {
        playHead.ppq = i * 0.25;

        juce::MidiBuffer midi;
        if (i < numNotes)
        {
            straightClicks.push_back(absoluteSample);
            midi.addEvent(juce::MidiMessage::noteOn(1, 60, (juce::uint8) 100), 0);
        }

        processor.processBlock(dummyBuffer, midi);

        for (const auto metadata : midi)
            if (metadata.getMessage().isNoteOn())
                swungClicks.push_back(absoluteSample + metadata.samplePosition);

        absoluteSample += blockSize;
    }

    std::cout << "bpm=" << bpm << " amount=" << amount << " numNotes=" << numNotes
               << " blockSize=" << blockSize << " samples (1 sixteenth-note)\n";

    writeClicksToWav(straightOut, straightClicks, totalSamples, sr);
    writeClicksToWav(swungOut, swungClicks, totalSamples, sr);

    return 0;
}
