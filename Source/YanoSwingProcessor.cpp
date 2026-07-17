#include "YanoSwingProcessor.h"
#include "PluginEditor.h"
#include <cmath>

YanoSwingProcessor::YanoSwingProcessor()
    : AudioProcessor(BusesProperties()), // MIDI effect -- no audio buses
      apvts(*this, nullptr, "STATE", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout YanoSwingProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "amount", "Amount", juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
    return { params.begin(), params.end() };
}

void YanoSwingProcessor::prepareToPlay(double sampleRate, int)
{
    lastSampleRate = sampleRate;
    blockStartAbsoluteSample = 0;
    freeRunningSampleCounter = 0;
    pending.clear();
    activeNoteDelays.clear();
}

double YanoSwingProcessor::computeSwingDelaySamples(double ppqPosition, double bpm, double sampleRate, float amount01)
{
    // 16th-note grid: 1 quarter note (1.0 ppq) = 4 sixteenths.
    const double gridPos = ppqPosition * 4.0;
    const juce::int64 nearestSixteenth = (juce::int64) std::llround(gridPos);
    const bool isOffGrid = (nearestSixteenth % 2 != 0);

    if (!isOffGrid || amount01 <= 0.0f)
        return 0.0;

    // Max delay is tuned against amapiano production references for the
    // shipped build; the exact fraction here is simplified for this public
    // version.
    const double maxDelaySixteenths = 0.4;
    const double delaySixteenths = (double) amount01 * maxDelaySixteenths;
    const double delayPpq = delaySixteenths / 4.0;
    const double delaySeconds = delayPpq * (60.0 / bpm);
    return delaySeconds * sampleRate;
}

void YanoSwingProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    const float amount01 = *apvts.getRawParameterValue("amount");

    double bpm = 120.0;
    double ppqAtBlockStart = 0.0;
    bool havePpq = false;
    if (auto* ph = getPlayHead())
    {
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm())
                bpm = *b;
            if (auto ppq = pos->getPpqPosition())
            {
                ppqAtBlockStart = *ppq;
                havePpq = true;
            }
        }
    }

    juce::MidiBuffer processedMidi;

    for (const auto metadata : midiMessages)
    {
        auto msg = metadata.getMessage();
        const int sampleOffset = metadata.samplePosition;

        const double ppqAtEvent = havePpq
            ? ppqAtBlockStart + ((double) sampleOffset / lastSampleRate) * (bpm / 60.0)
            : ((double) (freeRunningSampleCounter + sampleOffset) / lastSampleRate) * (bpm / 60.0);

        const juce::int64 eventAbsoluteSample = blockStartAbsoluteSample + sampleOffset;

        if (msg.isNoteOn())
        {
            const juce::int64 delaySamples = (juce::int64) std::llround(
                computeSwingDelaySamples(ppqAtEvent, bpm, lastSampleRate, amount01));
            const int key = msg.getChannel() * 128 + msg.getNoteNumber();
            activeNoteDelays[key] = delaySamples;

            if (delaySamples <= 0)
                processedMidi.addEvent(msg, sampleOffset);
            else
                pending.push_back({ msg, eventAbsoluteSample + delaySamples });
        }
        else if (msg.isNoteOff())
        {
            const int key = msg.getChannel() * 128 + msg.getNoteNumber();
            juce::int64 delaySamples = 0;
            if (auto it = activeNoteDelays.find(key); it != activeNoteDelays.end())
            {
                delaySamples = it->second;
                activeNoteDelays.erase(it);
            }

            if (delaySamples <= 0)
                processedMidi.addEvent(msg, sampleOffset);
            else
                pending.push_back({ msg, eventAbsoluteSample + delaySamples });
        }
        else
        {
            // Non-note messages (CC, pitch bend, etc.) pass through
            // immediately -- only note timing is groove-quantized in v0.1.
            processedMidi.addEvent(msg, sampleOffset);
        }
    }

    // Flush any pending (swung) events whose scheduled time falls in this block.
    for (size_t i = 0; i < pending.size();)
    {
        const juce::int64 target = pending[i].targetAbsoluteSample;
        if (target >= blockStartAbsoluteSample && target < blockStartAbsoluteSample + numSamples)
        {
            processedMidi.addEvent(pending[i].message, (int) (target - blockStartAbsoluteSample));
            pending.erase(pending.begin() + (long) i);
        }
        else if (target < blockStartAbsoluteSample)
        {
            // Missed its window (shouldn't normally happen) -- emit right
            // away rather than silently drop the note.
            processedMidi.addEvent(pending[i].message, 0);
            pending.erase(pending.begin() + (long) i);
        }
        else
        {
            ++i;
        }
    }

    midiMessages.swapWith(processedMidi);

    blockStartAbsoluteSample += numSamples;
    freeRunningSampleCounter += numSamples;

    buffer.clear();
}

juce::AudioProcessorEditor* YanoSwingProcessor::createEditor()
{
    return new YanoSwingEditor(*this);
}

void YanoSwingProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto state = apvts.copyState(); state.isValid())
        if (auto xml = state.createXml())
            copyXmlToBinary(*xml, destData);
}

void YanoSwingProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}
