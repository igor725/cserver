#include <core.h>
#include <server.h>

#include "SDL.h"

#define MOUSE_LOG "Mouse button %d %s %d times (%d, %d)"
#define WHEEL_LOG "Mouse wheel scrolled (%d, %d)"
#define TEXT_LOG "Text input: %s"
#define KEY_LOG "Key %d pressed, mod: %d, repeat: %d, state: %s"
#define FILE_LOG "File %s dropped"

bool Gui_Enabled = true;
bool Gui_Done = false;

static void Gui_Mouse(char state, char clicks, int button, int x, int y) {
	Log_Info(MOUSE_LOG, button, state == SDL_PRESSED ? "pressed" : "released", clicks, x, y);
}

static void Gui_MouseWheel(int x, int y) {
	Log_Info(WHEEL_LOG, x, y);
}

static void Gui_TextInput(char* text) {
	Log_Info(TEXT_LOG, text);
}

static void Gui_Key(char state, char repeat, SDL_Keysym* keysym) {
	Log_Info(KEY_LOG, keysym->scancode, keysym->mod, repeat,
		state == SDL_PRESSED ? "pressed" : "released");
}

static void Gui_FileDrop(const char* fdir) {
	Log_Info(FILE_LOG, fdir);
}

static TRET Gui_ThreadProc(TARG param) {
	SDL_DisplayMode mode;
	SDL_Surface* screenSurface = NULL;

	if(SDL_Init(SDL_INIT_EVENTS|SDL_INIT_VIDEO) != 0){
		Log_Info("SDL_Init error: %s", SDL_GetError());
		return false;
	}

	if(SDL_GetDesktopDisplayMode(0, &mode)) {
		Log_Info("SDL_GetDesktopDisplayMode error: %s", SDL_GetError());
		return 0;
	}

	SDL_Window* win = SDL_CreateWindow(
		"C-Server GUI",
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		700, 400,
		SDL_WINDOW_SHOWN
	);

	screenSurface = SDL_GetWindowSurface(win);
	SDL_FillRect(screenSurface, NULL, SDL_MapRGB(screenSurface->format, 0x00, 0x00, 0x00));

	while(Gui_Enabled) {
		SDL_Event evt;
		while(SDL_PollEvent(&evt)) {
			switch(evt.type) {
				case SDL_QUIT:
					Gui_Enabled = false;
					break;
				case SDL_MOUSEBUTTONUP:
				case SDL_MOUSEBUTTONDOWN:
					Gui_Mouse(evt.button.state, evt.button.clicks, evt.button.button, evt.motion.x, evt.motion.y);
					break;
				case SDL_MOUSEWHEEL:
					Gui_MouseWheel(evt.wheel.x, evt.wheel.y);
					break;
				case SDL_TEXTINPUT:
					Gui_TextInput(evt.text.text);
					break;
				case SDL_KEYDOWN:
				case SDL_KEYUP:
					Gui_Key(evt.key.state, evt.key.repeat, &evt.key.keysym);
					break;
				case SDL_DROPFILE:
					Gui_FileDrop(evt.drop.file);
					SDL_free(evt.drop.file);
					break;
			}
			SDL_UpdateWindowSurface(win);
		}
		SDL_Delay(10);
	}

	SDL_DestroyWindow(win);
	Log_Info("GUI window closed");
	SDL_Quit();
	serverActive = false;
	Gui_Done = true;
	return 0;
}

EXP int Plugin_ApiVer = 100;
EXP bool Plugin_Load() {
	Thread_Create(Gui_ThreadProc, NULL);
	return true;
}
EXP bool Plugin_Unload() {
	Gui_Enabled = false;
	while(!Gui_Done) {}
	return true;
}
