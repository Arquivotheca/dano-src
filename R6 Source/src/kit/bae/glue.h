
class BAEOutputMixer;
class BAEMidiSynth;
class BAEMidiSong;

extern "C" {
BAEOutputMixer *instantiate_outputmixer(void);
BAEMidiSynth *instantiate_midisynth(BAEOutputMixer *mixer);
BAEMidiSong *instantiate_midisong(BAEOutputMixer *mixer);
BAERmfSong *instantiate_rmfsong(BAEOutputMixer *mixer);
}
