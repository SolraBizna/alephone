#ifndef __SECONDMUSICSYSTEM_H
#define __SECONDMUSICSYSTEM_H

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

#include <string>

struct SMS_Commander;

namespace SMS {
    // Turns SMS off, if it was on. Harmless to call more than once.
	//
	// Call from the main thread.
    void Deactivate();
    // Returns the `SMS_Commander` for the active instance of the engine. If the
    // engine is not yet active, makes it active.
	//
	// Call from the main thread.
    struct SMS_Commander* GetCommander();
    // Returns the `SMS_Commander` for the active instance of the engine, or
	// nullptr if the engine is not active.
	//
	// Call from the main thread.
    struct SMS_Commander* GetOptionalCommander();
	// If the engine is active, mixes some output into the target buffer and
	// returns true. If the engine is inactive, doesn't touch the target buffer
	// and returns false.
	//
	// out_len is the number of *floats*, not the number of bytes or the number
	// of sample frames.
	//
	// Call from the audio thread.
	bool TurnHandle(float* out, size_t out_len);
	// Changes the base path at which the engine will search for music. Call
	// from any thread.
	//
	// Currently called with the Lua search path any time `replace_soundtrack`
	// is called. Whatever Lua script has opinions about what music should
	// be playing is the one whose search path should be followed.
	void SetMusicSearchPath(std::string);
	// Call from the main thread when exiting a level for any reason.
	void LeavingMap();
	// Call when the status of film recording changes. This may result in the
	// engine being recreated.
	void SetBackgroundLoading(bool);
	// Call from the main thread to access the "leave map behavior".
	float GetFadeOnLeaveMap();
	void SetFadeOnLeaveMap(float);
	std::string GetStartFlowOnLeaveMap();
	void SetStartFlowOnLeaveMap(std::string);
	std::string GetFlowControlOnLeaveMap();
	void SetFlowControlOnLeaveMap(std::string);
}

#endif

#endif
