/*

	Copyright (C) 2023 Solra Bizna
	and the "Aleph One" developers.

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	This license is contained in the file "COPYING",
	which is included with this source code; it is available online at
	http://www.gnu.org/licenses/gpl.html

    -

	Manages the Second Music System interface.

*/

#ifdef HAVE_SECOND_MUSIC_SYSTEM

#include "SecondMusicSystem.h"

#include "second-music-system.h"

#include <atomic>
#include <memory>
#include <mutex>
#include <thread>

#include "Logging.h"
#include "MusicPlayer.h" // to get the music playback volume as a ratio (not dB)
#include "OpenALManager.h"
#include "SndfileDecoder.h"
#include "SoundManager.h"
#include "csalerts.h"
#include "cseries.h"

namespace {
    // The delegate that SMS will use to print warnings and open audio files.
    struct SMS_SoundDelegate* delegate = nullptr;
    // The Commander instance that the main thread can use to talk to the
    // engine.
    struct SMS_Commander* commander = nullptr;
    // The engine instance that the audio thread uses to slurp down samples.
    struct SMS_Engine* engine = nullptr;
    // Indicates the desired state of the engine. When bringing the engine up,
    // the main thread will create a new engine, put it in `engine`, and store
    // true. When bringing the engine down, the main thread will store `false`,
    // then spin on `is_active` (to make sure the audio thread tears down the
    // engine).
    std::atomic<bool> want_active;
    // Indicates the current state of the engine, as seen by the audio thread.
    std::atomic<bool> is_active;
    // The path where the engine will look for music files.
    std::string music_search_path;
    std::mutex music_search_path_lock;
    // If non-negative, we will induce all flows to fade out upon leaving the
    // map.
    float fade_on_leave_map = 0.0f;
    // If non-empty, we will start this flow upon leaving the map.
    std::string start_flow_on_leave_map;
    // If non-empty, we will set this flow control to "leaving" upon leaving
    // the map.
    std::string set_flow_control_on_leave_map;
    // Whether we should use background loading (in case of film recording).
    bool use_background_loading = true;
    // Whether we think we're in a map. (Whether any SMS commands have been
    // issued since the last time LeavingMap or Deactivate was called.)
    bool in_map = false;

    std::shared_ptr<StreamPlayer> current_sms_player;
    int SMSCallback(uint8* data, int length) {
        bool still_active = SMS::TurnHandle(reinterpret_cast<float*>(data), length / sizeof(float));
        if(!still_active) {
            if(current_sms_player) {
                current_sms_player->AskStop();
            }
            return 0;
        } else {
            return length;
        }
        /*
        SetupALResult SetUpALSourceIdle() override {
            float gain = MusicPlayer::GetDefaultVolume()
                * OpenALManager::Get()->GetMasterVolume();
            alSourcef(audio_source->source_id, AL_MAX_GAIN, gain);
            alSourcef(audio_source->source_id, AL_GAIN, gain);
            return SetupALResult(alGetError() == AL_NO_ERROR, true);
        }*/
    }
    size_t read_handler(void* cdata, void* buf, size_t num_samples_in_buf) {
        SndfileDecoder* decoder = reinterpret_cast<SndfileDecoder*>(cdata);
        int32 result = decoder->Decode(reinterpret_cast<uint8*>(buf), num_samples_in_buf * sizeof(float));
        if(result <= 0) return 0;
        else return size_t(result / sizeof(float));
    }

    void stream_free_handler(void* cdata) {
        SndfileDecoder* decoder = reinterpret_cast<SndfileDecoder*>(cdata);
        delete decoder;
    }

    uint64_t estimate_len_handler(void* cdata) {
        SndfileDecoder* decoder = reinterpret_cast<SndfileDecoder*>(cdata);
        int32 frames = decoder->Frames();
        if(frames <= 0) return uint64_t(-1);
        else return uint64_t(frames);
    }

    SMS_FormattedSoundStream* file_open_handler(void*, const char* path) {
        FileSpecifier fsspec;
        {
            const std::lock_guard<std::mutex> lock(music_search_path_lock);
            if(music_search_path.size() != 0) {
                if(!fsspec.SetNameWithPath(path, music_search_path))
                    return nullptr;
            } else {
                if(!fsspec.SetNameWithPath(path))
                    return nullptr;
            }
        }
        std::unique_ptr<SndfileDecoder> decoder = std::make_unique<SndfileDecoder>();
        if(!decoder->Open(fsspec)) {
            // SMS will log a warning about this.
            //logError("Could not open music file: %s", path);
            return nullptr;
        }
        int format;
        switch(decoder->GetAudioFormat()) {
        case AudioFormat::_8_bit:
            format = SMS_SOUND_FORMAT_UNSIGNED_8;
            break;
        case AudioFormat::_16_bit:
            format = SMS_SOUND_FORMAT_SIGNED_16;
            break;
        case AudioFormat::_32_float:
            format = SMS_SOUND_FORMAT_FLOAT_32;
            break;
        default: assert(false);
        }
        int speaker_layout = !decoder->IsStereo()
            ? SMS_SPEAKER_LAYOUT_MONO
            : OpenALManager::Get()->Is_HRTF_Enabled()
            ? SMS_SPEAKER_LAYOUT_HEADPHONES
            : SMS_SPEAKER_LAYOUT_STEREO;
        float sample_rate = decoder->Rate();
        assert(format == SMS_SOUND_FORMAT_UNSIGNED_8
            || PlatformIsLittleEndian() == decoder->IsLittleEndian());
        return SMS_FormattedSoundStream_new(
            decoder.release(), // (callback data)
            sample_rate,
            speaker_layout,
            format,
            read_handler,
            stream_free_handler,
            nullptr, // no seek handler
            nullptr, // default skip handlers
            nullptr, // ...
            nullptr, // not cloneable
            estimate_len_handler
        );
    }

    void warning_handler(void*, const char* message) {
        logWarning("Second Music System warning: %s", message);
    }

    void reset_leave_map_vars() {
        fade_on_leave_map = 0.0f;
        start_flow_on_leave_map = "";
    }

    void maybe_initialize() {
        if(want_active.load()) return; // don't load if already active
        reset_leave_map_vars();
        auto openal = OpenALManager::Get();
        if(!openal) {
            logError("No OpenALManager instance when SMS was initialized");
            return;
        }
        assert(engine == nullptr);
        if(delegate == nullptr) {
            delegate = SMS_SoundDelegate_new(
                nullptr, // no callback data pointer
                file_open_handler,
                warning_handler,
                nullptr // nothing to free so no free handler
            );
        }
        auto sound_manager = SoundManager::instance();
        engine = SMS_Engine_new(
            delegate,
            SMS_SPEAKER_LAYOUT_STEREO,
            sound_manager->parameters.rate,
            use_background_loading,
            0, // sensible default number of decoder threads
            0 // sensible default core affinities
        );
        commander = SMS_Engine_clone_commander(engine);
        want_active.store(true);
        current_sms_player = OpenALManager::Get()->PlayStream(SMSCallback, SoundManager::instance()->parameters.rate, true, AudioFormat::_32_float, MusicPlayer::GetDefaultVolume(), false);
    }
}

void SMS::Deactivate() {
    bool was_active = is_active.load();
    if(was_active) {
        assert(engine != nullptr);
        assert(commander != nullptr);
        want_active.store(false);
        SMS_Commander_free(commander);
        commander = nullptr;
        // Spin until the audio thread disposes of the engine.
        while(is_active.load()) {
            std::this_thread::yield();
        }
        assert(engine == nullptr);
        current_sms_player = nullptr;
        in_map = false;
        reset_leave_map_vars();
    }
    // if we weren't active, there's nothing for us to do
}

struct SMS_Commander* SMS::GetCommander() {
    in_map = true;
    maybe_initialize();
    return commander;
}

struct SMS_Commander* SMS::GetOptionalCommander() {
    return commander;
}

bool SMS::TurnHandle(float* out, size_t out_len) {
    bool wanting_active = want_active.load();
    bool am_active = is_active.load();
    if(wanting_active != am_active) {
        if(wanting_active) {
            assert(!am_active);
            // nothing else to do
        }
        else {
            assert(am_active);
            SMS_Engine_free(engine);
            engine = nullptr;
        }
        is_active.store(wanting_active);
        am_active = wanting_active;
    }
    if(am_active) {
        SMS_Engine_turn_handle(engine, out, out_len);
        return true;
    }
    else {
        return false;
    }
}

void SMS::SetMusicSearchPath(std::string new_path) {
    const std::lock_guard<std::mutex> lock(music_search_path_lock);
    music_search_path = new_path;
}

void SMS::LeavingMap() {
    if(!in_map) return;
    auto commander = SMS::GetOptionalCommander();
    if(commander != nullptr) {
        if(fade_on_leave_map >= 0.0f) {
            SMS_Commander_fade_all_flows_out(commander, fade_on_leave_map, SMS_FADE_TYPE_DEFAULT);
        }
        if(!start_flow_on_leave_map.empty()) {
            SMS_Commander_start_flow(commander, start_flow_on_leave_map.data(), start_flow_on_leave_map.size(), 1.0, 0.0, SMS_FADE_TYPE_DEFAULT);
        }
        if(!set_flow_control_on_leave_map.empty()) {
            SMS_Commander_set_flow_control_to_string(commander, set_flow_control_on_leave_map.data(), set_flow_control_on_leave_map.size(), "leaving", 7);
        }
    }
    in_map = false;
    reset_leave_map_vars();
}

void SMS::SetBackgroundLoading(bool nu) {
    if(nu != use_background_loading) {
        SMS::Deactivate();
        use_background_loading = nu;
        // will be reactivated when needed
    }
}

float SMS::GetFadeOnLeaveMap() {
    return fade_on_leave_map;
}

void SMS::SetFadeOnLeaveMap(float new_value) {
    fade_on_leave_map = new_value;
}

std::string SMS::GetStartFlowOnLeaveMap() {
    return start_flow_on_leave_map;
}

void SMS::SetStartFlowOnLeaveMap(std::string new_value) {
    start_flow_on_leave_map = new_value;
}

std::string SMS::GetFlowControlOnLeaveMap() {
    return set_flow_control_on_leave_map;
}

void SMS::SetFlowControlOnLeaveMap(std::string new_value) {
    set_flow_control_on_leave_map = new_value;
}

#endif
