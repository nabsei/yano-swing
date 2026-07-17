#include "YanoSwingProcessor.h"

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new YanoSwingProcessor();
}
