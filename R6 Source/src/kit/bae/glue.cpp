#include <BAE.h>
#include "glue.h"

BAEOutputMixer *instantiate_outputmixer(void)
{
	return new BAEOutputMixer();
}

BAEMidiSynth *instantiate_midisynth(BAEOutputMixer *mixer)
{
	return new BAEMidiSynth(mixer);
}

BAEMidiSong *instantiate_midisong(BAEOutputMixer *mixer)
{
	return new BAEMidiSong(mixer);
}

BAERmfSong *instantiate_rmfsong(BAEOutputMixer *mixer)
{
	return new BAERmfSong(mixer);
}
