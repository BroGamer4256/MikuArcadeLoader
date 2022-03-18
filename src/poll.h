#include "helpers.h"
#include <SDL.h>
#include <windows.h>

enum SDLAxis {
	SDL_AXIS_NULL,
	SDL_AXIS_LEFT_LEFT,
	SDL_AXIS_LEFT_RIGHT,
	SDL_AXIS_LEFT_UP,
	SDL_AXIS_LEFT_DOWN,
	SDL_AXIS_RIGHT_LEFT,
	SDL_AXIS_RIGHT_RIGHT,
	SDL_AXIS_RIGHT_UP,
	SDL_AXIS_RIGHT_DOWN,
	SDL_AXIS_LTRIGGER_DOWN,
	SDL_AXIS_RTRIGGER_DOWN,
	SDL_AXIS_MAX
};

struct SDLAxisState {
	unsigned int LeftLeft : 1;
	unsigned int LeftRight : 1;
	unsigned int LeftUp : 1;
	unsigned int LeftDown : 1;
	unsigned int RightLeft : 1;
	unsigned int RightRight : 1;
	unsigned int RightUp : 1;
	unsigned int RightDown : 1;
	unsigned int LTriggerDown : 1;
	unsigned int RTriggerDown : 1;
};

struct Keybindings {
	BYTE keycodes[255];
	SDL_GameControllerButton buttons[255];
	enum SDLAxis axis[255];
};

enum EnumType { none, keycode, button, axis };

struct ConfigValue {
	enum EnumType type;
	BYTE keycode;
	SDL_GameControllerButton button;
	enum SDLAxis axis;
};

struct InternalButtonState {
	unsigned int Released : 1;
	unsigned int Down : 1;
	unsigned int Tapped : 1;
};

void InitializePoll (HWND DivaWindowHandle);
void UpdatePoll (HWND DivaWindowHandle);
void DisposePoll ();

struct ConfigValue StringToConfigEnum (char *value);
void SetConfigValue (toml_table_t *table, char *key,
					 struct Keybindings *keybind);
struct InternalButtonState
GetInternalButtonState (struct Keybindings bindings);
void SetRumble (int left, int right);

bool KeyboardIsDown (BYTE keycode);
bool KeyboardIsUp (BYTE keycode);
bool KeyboardIsTapped (BYTE keycode);
bool KeyboardIsReleased (BYTE keycode);
bool KeyboardWasDown (BYTE keycode);
bool KeyboardWasUp (BYTE keycode);
POINT GetMousePosition ();
POINT GetLastMousePosition ();
POINT GetMouseRelativePosition ();
POINT GetLastMouseRelativePosition ();
void SetMousePosition (POINT new);
bool GetMouseScrollUp ();
bool GetMouseScrollDown ();
bool ControllerButtonIsDown (SDL_GameControllerButton button);
bool ControllerButtonIsUp (SDL_GameControllerButton button);
bool ControllerButtonWasDown (SDL_GameControllerButton button);
bool ControllerButtonWasUp (SDL_GameControllerButton button);
bool ControllerButtonIsTapped (SDL_GameControllerButton button);
bool ControllerButtonIsReleased (SDL_GameControllerButton button);
bool ControllerAxisIsDown (enum SDLAxis axis);
bool ControllerAxisIsUp (enum SDLAxis axis);
bool ControllerAxisWasDown (enum SDLAxis axis);
bool ControllerAxisWasUp (enum SDLAxis axis);
bool ControllerAxisIsTapped (enum SDLAxis axis);
bool ControllerAxisIsReleased (enum SDLAxis axis);
bool IsButtonTapped (struct Keybindings bindings);
bool IsButtonReleased (struct Keybindings bindings);
bool IsButtonDown (struct Keybindings bindings);