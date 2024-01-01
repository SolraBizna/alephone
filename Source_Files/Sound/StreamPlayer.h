#ifndef __STREAM_PLAYER_H
#define __STREAM_PLAYER_H

#include "AudioPlayer.h"

typedef int (*CallBackStreamPlayer)(uint8* data, int length);

class StreamPlayer : public AudioPlayer {
public:
	StreamPlayer(CallBackStreamPlayer callback, int rate, bool stereo, AudioFormat audioFormat, float initialGain, bool shouldRoutinelyStop); //Must not be used outside OpenALManager (public for make_shared)
	float GetPriority() const override { return 10; } //As long as it's only used for intro video, it doesn't really matter
	float GetGain() const { return gain; }
	void SetGain(float gain) { this->gain = gain; }
protected:
	SetupALResult SetUpALSourceIdle() override;
	bool ShouldRoutinelyStop() const override { return shouldRoutinelyStop; }
private:
	int GetNextData(uint8* data, int length) override;
	CallBackStreamPlayer CallBackFunction;
	bool shouldRoutinelyStop;
	float gain;
	friend class OpenALManager;
};

#endif