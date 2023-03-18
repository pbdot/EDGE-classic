//----------------------------------------------------------------------------
//  EDGE Main
//----------------------------------------------------------------------------
//
//  Copyright (c) 1999-2009  The EDGE Team.
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//----------------------------------------------------------------------------

#include "i_defs.h"
#include "i_sdlinc.h"  // needed for proper SDL main linkage
#include "filesystem.h"
#include "str_util.h"

#include "dm_defs.h"
#include "m_argv.h"
#include "e_main.h"
#include "version.h"

#include <emscripten/html5.h>

// Event reference
// https://github.com/emscripten-ports/SDL2/blob/master/src/video/emscripten/SDL_emscriptenevents.c

std::filesystem::path exe_path = ".";

void E_WebTick(void)
{
	// We always do this once here, although the engine may
	// makes in own calls to keep on top of the event processing
	I_ControlGetEvents();

	if (app_state & APP_STATE_ACTIVE)
		E_Tick();
}

extern "C" {

static EM_BOOL I_WebHandlePointerLockChange(int eventType, const EmscriptenPointerlockChangeEvent *changeEvent, void *userData)
{	
	if (changeEvent->isActive)
	{
		SDL_ShowCursor(SDL_FALSE);
	}
	else
	{
		SDL_ShowCursor(SDL_TRUE);
	}
	
    return 0;
}

void EMSCRIPTEN_KEEPALIVE I_WebMain(int argc, const char **argv) 
{

	emscripten_set_main_loop(E_WebTick, 0, 0);

	emscripten_set_pointerlockchange_callback(EMSCRIPTEN_EVENT_TARGET_DOCUMENT, nullptr, 0, I_WebHandlePointerLockChange);

	if (SDL_Init(0) < 0)
		I_Error("Couldn't init SDL!!\n%s\n", SDL_GetError());

	exe_path = UTFSTR(SDL_GetBasePath());

	E_Main(argc, argv);
	
	EM_ASM_({
		if (Module.edgePostInit) {
			Module.edgePostInit();
		}
	});

}

int main(int argc, char *argv[])
{
	EM_ASM_({

		const args = [];
		for (let i = 0; i < $0; i++) {
			args.push(UTF8ToString(HEAP32[($1 >> 2) + i]));
		}

		console.log(`Edge command line: ${args}`);

		const homeIndex = args.indexOf("-home");
		if (homeIndex === -1 || homeIndex >= args.length || args[homeIndex + 1].startsWith("-")) {
			throw "No home command line option specified"
		}

		const homeDir = args[homeIndex + 1];
		
		if (!FS.analyzePath(homeDir).exists) 
		{
			FS.mkdirTree(homeDir);
		}
		
		FS.mount(IDBFS, {}, homeDir);
		FS.syncfs(true, function (err) {
			if (err) {
				console.error(`Error mounting home dir ${err}`);
				return;
			}
			Module._I_WebMain($0, $1);
		});
		}, argc, argv);

	return 0;
}

}


