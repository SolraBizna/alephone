#include "StreamPlayer.h"
#include "OpenALManager.h"

StreamPlayer::StreamPlayer(CallBackStreamPlayer callback, int rate, bool stereo, AudioFormat audioFormat, float initialGain, bool shouldRoutinelyStop)
	: AudioPlayer(rate, stereo, audioFormat) {
	CallBackFunction = callback;
	this->shouldRoutinelyStop = shouldRoutinelyStop;
	gain = initialGain;
}

int StreamPlayer::GetNextData(uint8* data, int length) {
	return CallBackFunction(data, length);
}

SetupALResult StreamPlayer::SetUpALSourceIdle() {
	float currentGain = gain * OpenALManager::Get()->GetMasterVolume();
	alSourcef(audio_source->source_id, AL_MAX_GAIN, currentGain);
	alSourcef(audio_source->source_id, AL_GAIN, currentGain);
	return SetupALResult(alGetError() == AL_NO_ERROR, true);
}