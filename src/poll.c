#include "poll.h"

struct {
	const char *string;
	BYTE keycode;
} ConfigKeyboardButtons[] = {
	{ "F1", VK_F1 },
	{ "F2", VK_F2 },
	{ "F3", VK_F3 },
	{ "F4", VK_F4 },
	{ "F5", VK_F5 },
	{ "F6", VK_F6 },
	{ "F7", VK_F7 },
	{ "F8", VK_F8 },
	{ "F9", VK_F9 },
	{ "F10", VK_F10 },
	{ "F11", VK_F11 },
	{ "F12", VK_F12 },
	{ "NUM1", '1' },
	{ "NUM2", '2' },
	{ "NUM3", '3' },
	{ "NUM4", '4' },
	{ "NUM5", '5' },
	{ "NUM6", '6' },
	{ "NUM7", '7' },
	{ "NUM8", '8' },
	{ "NUM9", '9' },
	{ "NUM0", '0' },
	{ "Q", 'Q' },
	{ "W", 'W' },
	{ "E", 'E' },
	{ "R", 'R' },
	{ "T", 'T' },
	{ "Y", 'Y' },
	{ "U", 'U' },
	{ "I", 'I' },
	{ "O", 'O' },
	{ "P", 'P' },
	{ "A", 'A' },
	{ "S", 'S' },
	{ "D", 'D' },
	{ "F", 'F' },
	{ "G", 'G' },
	{ "H", 'H' },
	{ "J", 'J' },
	{ "K", 'K' },
	{ "L", 'L' },
	{ "Z", 'Z' },
	{ "X", 'X' },
	{ "C", 'C' },
	{ "V", 'V' },
	{ "B", 'B' },
	{ "N", 'N' },
	{ "M", 'M' },
	{ "UPARROW", VK_UP },
	{ "LEFTARROW", VK_LEFT },
	{ "DOWNARROW", VK_DOWN },
	{ "RIGHTARROW", VK_RIGHT },
	{ "ENTER", VK_RETURN },
	{ "SPACE", VK_SPACE },
	{ "CONTROL", VK_CONTROL },
	{ "SHIFT", VK_SHIFT },
	{ "TAB", VK_TAB },
};

struct {
	const char *string;
	SDL_GameControllerButton button;
} ConfigControllerButtons[] = {
	{ "SDL_A", SDL_CONTROLLER_BUTTON_A },
	{ "SDL_B", SDL_CONTROLLER_BUTTON_B },
	{ "SDL_X", SDL_CONTROLLER_BUTTON_X },
	{ "SDL_Y", SDL_CONTROLLER_BUTTON_Y },
	{ "SDL_BACK", SDL_CONTROLLER_BUTTON_BACK },
	{ "SDL_GUIDE", SDL_CONTROLLER_BUTTON_GUIDE },
	{ "SDL_START", SDL_CONTROLLER_BUTTON_START },
	{ "SDL_LSTICK_PRESS", SDL_CONTROLLER_BUTTON_LEFTSTICK },
	{ "SDL_RSTICK_PRESS", SDL_CONTROLLER_BUTTON_RIGHTSTICK },
	{ "SDL_LSHOULDER", SDL_CONTROLLER_BUTTON_LEFTSHOULDER },
	{ "SDL_RSHOULDER", SDL_CONTROLLER_BUTTON_RIGHTSHOULDER },
	{ "SDL_DPAD_UP", SDL_CONTROLLER_BUTTON_DPAD_UP },
	{ "SDL_DPAD_DOWN", SDL_CONTROLLER_BUTTON_DPAD_DOWN },
	{ "SDL_DPAD_LEFT", SDL_CONTROLLER_BUTTON_DPAD_LEFT },
	{ "SDL_DPAD_RIGHT", SDL_CONTROLLER_BUTTON_DPAD_RIGHT },
	{ "SDL_MISC", SDL_CONTROLLER_BUTTON_MISC1 },
	{ "SDL_PADDLE1", SDL_CONTROLLER_BUTTON_PADDLE1 },
	{ "SDL_PADDLE2", SDL_CONTROLLER_BUTTON_PADDLE2 },
	{ "SDL_PADDLE3", SDL_CONTROLLER_BUTTON_PADDLE3 },
	{ "SDL_PADDLE4", SDL_CONTROLLER_BUTTON_PADDLE4 },
	{ "SDL_TOUCHPAD", SDL_CONTROLLER_BUTTON_TOUCHPAD },
};

struct {
	const char *string;
	enum SDLAxis axis;
} ConfigControllerAXIS[] = {
	{ "SDL_LSTICK_LEFT", SDL_AXIS_LEFT_LEFT },
	{ "SDL_LSTICK_UP", SDL_AXIS_LEFT_UP },
	{ "SDL_LSTICK_DOWN", SDL_AXIS_LEFT_DOWN },
	{ "SDL_LSTICK_RIGHT", SDL_AXIS_LEFT_RIGHT },
	{ "SDL_RSTICK_LEFT", SDL_AXIS_RIGHT_LEFT },
	{ "SDL_RSTICK_UP", SDL_AXIS_RIGHT_UP },
	{ "SDL_RSTICK_DOWN", SDL_AXIS_RIGHT_DOWN },
	{ "SDL_RSTICK_RIGHT", SDL_AXIS_RIGHT_RIGHT },
	{ "SDL_LTRIGGER", SDL_AXIS_LTRIGGER_DOWN },
	{ "SDL_RTRIGGER", SDL_AXIS_RTRIGGER_DOWN },
};

struct MouseState {
	POINT Position;
	POINT RelativePosition;
	bool ScrolledUp;
	bool ScrolledDown;
} currentMouseState, lastMouseState;

bool currentKeyboardState[0xFF];
bool lastKeyboardState[0xFF];

bool currentControllerButtonsState[SDL_CONTROLLER_BUTTON_MAX];
bool lastControllerButtonsState[SDL_CONTROLLER_BUTTON_MAX];
struct SDLAxisState currentControllerAxisState;
struct SDLAxisState lastControllerAxisState;

SDL_Window *window;
SDL_GameController *controllers[255];

void
SetConfigValue (toml_table_t *table, char *key, struct Keybindings *keybind) {
	toml_array_t *array = toml_array_in (table, key);
	if (!array) {
		printf ("Error at SetConfigValue (%s): Cannot find array\n", key);
		return;
	}

	memset (keybind, 0, sizeof (*keybind));
	for (int i = 0; i < COUNTOFARR (keybind->buttons); i++)
		keybind->buttons[i] = SDL_CONTROLLER_BUTTON_INVALID;

	for (int i = 0;; i++) {
		toml_datum_t bind = toml_string_at (array, i);
		if (!bind.ok)
			break;
		struct ConfigValue value = StringToConfigEnum (bind.u.s);
		free (bind.u.s);

		switch (value.type) {
		case keycode:
			for (int i = 0; i < COUNTOFARR (keybind->keycodes); i++) {
				if (keybind->keycodes[i] == 0) {
					keybind->keycodes[i] = value.keycode;
					break;
				}
			}
			break;
		case button:
			for (int i = 0; i < COUNTOFARR (keybind->buttons); i++) {
				if (keybind->buttons[i] == SDL_CONTROLLER_BUTTON_INVALID) {
					keybind->buttons[i] = value.button;
					break;
				}
			}
			break;
		case axis:
			for (int i = 0; i < COUNTOFARR (keybind->axis); i++) {
				if (keybind->axis[i] == 0) {
					keybind->axis[i] = value.axis;
					break;
				}
			}
			break;
		default:
			break;
		}
	}
}

void
InitializePoll (HWND DivaWindowHandle) {
	SDL_SetMainReady ();

	SDL_SetHint (SDL_HINT_JOYSTICK_HIDAPI_PS4, "1");
	SDL_SetHint (SDL_HINT_JOYSTICK_HIDAPI_PS4_RUMBLE, "1");
	SDL_SetHint (SDL_HINT_JOYSTICK_HIDAPI_PS5, "1");
	SDL_SetHint (SDL_HINT_JOYSTICK_HIDAPI_PS5_RUMBLE, "1");

	if (SDL_Init (SDL_INIT_JOYSTICK | SDL_INIT_HAPTIC | SDL_INIT_GAMECONTROLLER
				  | SDL_INIT_EVENTS | SDL_INIT_VIDEO)
		!= 0)
		printf ("Error at SDL_Init (SDL_INIT_JOYSTICK | SDL_INIT_HAPTIC | "
				"SDL_INIT_GAMECONTROLLER | SDL_INIT_EVENTS | SDL_INIT_VIDEO): "
				"%s\n",
				SDL_GetError ());

	if (SDL_GameControllerAddMappingsFromFile (
			configPath ("gamecontrollerdb.txt"))
		== -1)
		printf ("Error at InitializePoll (): Cannot read "
				"plugins/gamecontrollerdb.txt\n");
	SDL_GameControllerEventState (SDL_ENABLE);

	for (int i = 0; i < SDL_NumJoysticks (); i++) {
		if (!SDL_IsGameController (i))
			continue;

		SDL_GameController *controller = SDL_GameControllerOpen (i);

		if (!controller) {
			printf ("Could not open gamecontroller %s: %s\n",
					SDL_GameControllerNameForIndex (i), SDL_GetError ());
			continue;
		}

		controllers[i] = controller;
		break;
	}

	window = SDL_CreateWindowFrom (DivaWindowHandle);
	if (window != NULL)
		SDL_SetWindowResizable (window, true);
	else
		printf ("Error at SDL_CreateWindowFrom (DivaWindowHandle): %s\n",
				SDL_GetError ());
}

void
UpdatePoll (HWND DivaWindowHandle) {
	if (DivaWindowHandle == NULL || GetForegroundWindow () != DivaWindowHandle)
		return;

	memcpy (lastKeyboardState, currentKeyboardState, 255);
	lastMouseState = currentMouseState;
	lastControllerAxisState = currentControllerAxisState;

	for (BYTE i = 0; i < 0xFF; i++)
		currentKeyboardState[i] = GetAsyncKeyState (i) != 0;

	currentMouseState.ScrolledUp = false;
	currentMouseState.ScrolledDown = false;

	GetCursorPos (&currentMouseState.Position);
	currentMouseState.RelativePosition = currentMouseState.Position;

	ScreenToClient (DivaWindowHandle, &currentMouseState.RelativePosition);

	RECT hWindow;
	GetClientRect (DivaWindowHandle, &hWindow);

	int *gameWidth = (int *)0x140EDA8BC;
	int *gameHeight = (int *)0x140EDA8C0;
	int *fbWidth = (int *)0x1411ABCA8;
	int *fbHeight = (int *)0x1411ABCAC;

	if ((fbWidth != gameWidth) && (fbHeight != gameHeight)) {
		float scale;
		float xoffset = (16.0f / 9.0f) * hWindow.bottom;
		if (xoffset != hWindow.right) {
			scale = xoffset / hWindow.right;
			xoffset = (hWindow.right / 2) - (xoffset / 2);
		} else {
			xoffset = 0;
			scale = 1;
		}

		currentMouseState.RelativePosition.x
			= ((currentMouseState.RelativePosition.x - xoffset) * *gameWidth
			   / hWindow.right)
			  / scale;
		currentMouseState.RelativePosition.y
			= currentMouseState.RelativePosition.y * *gameHeight
			  / hWindow.bottom;
	}

	SDL_Event event;
	while (SDL_PollEvent (&event) != 0) {
		switch (event.type) {
		case SDL_CONTROLLERDEVICEADDED:
			if (!SDL_IsGameController (event.cdevice.which))
				break;

			SDL_GameController *controller
				= SDL_GameControllerOpen (event.cdevice.which);

			if (!controller) {
				printf ("Error at PollSDLInput (): Could not open "
						"gamecontroller %s: %s\n",
						SDL_GameControllerNameForIndex (event.cdevice.which),
						SDL_GetError ());
				continue;
			}

			controllers[event.cdevice.which] = controller;
			break;
		case SDL_CONTROLLERDEVICEREMOVED:
			if (!SDL_IsGameController (event.cdevice.which))
				break;
			SDL_GameControllerClose (controllers[event.cdevice.which]);

			break;
		case SDL_MOUSEWHEEL:
			if (event.wheel.y > 0)
				currentMouseState.ScrolledUp = true;
			else if (event.wheel.y < 0)
				currentMouseState.ScrolledDown = true;
			break;
		case SDL_CONTROLLERBUTTONUP:
		case SDL_CONTROLLERBUTTONDOWN:
			currentControllerButtonsState[event.cbutton.button]
				= event.cbutton.state;
			break;
		case SDL_CONTROLLERAXISMOTION:
			if (event.caxis.value > 8000) {
				switch (event.caxis.axis) {
				case SDL_CONTROLLER_AXIS_LEFTX:
					currentControllerAxisState.LeftRight = 1;
					break;
				case SDL_CONTROLLER_AXIS_LEFTY:
					currentControllerAxisState.LeftDown = 1;
					break;
				case SDL_CONTROLLER_AXIS_RIGHTX:
					currentControllerAxisState.RightRight = 1;
					break;
				case SDL_CONTROLLER_AXIS_RIGHTY:
					currentControllerAxisState.RightDown = 1;
					break;
				case SDL_CONTROLLER_AXIS_TRIGGERLEFT:
					currentControllerAxisState.LTriggerDown = 1;
					break;
				case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:
					currentControllerAxisState.RTriggerDown = 1;
					break;
				}
			} else if (event.caxis.value < -8000) {
				switch (event.caxis.axis) {
				case SDL_CONTROLLER_AXIS_LEFTX:
					currentControllerAxisState.LeftLeft = 1;
					break;
				case SDL_CONTROLLER_AXIS_LEFTY:
					currentControllerAxisState.LeftUp = 1;
					break;
				case SDL_CONTROLLER_AXIS_RIGHTX:
					currentControllerAxisState.RightLeft = 1;
					break;
				case SDL_CONTROLLER_AXIS_RIGHTY:
					currentControllerAxisState.RightUp = 1;
					break;
				}
			} else {
				switch (event.caxis.axis) {
				case SDL_CONTROLLER_AXIS_LEFTX:
					currentControllerAxisState.LeftRight = 0;
					currentControllerAxisState.LeftLeft = 0;
					break;
				case SDL_CONTROLLER_AXIS_LEFTY:
					currentControllerAxisState.LeftDown = 0;
					currentControllerAxisState.LeftUp = 0;
					break;
				case SDL_CONTROLLER_AXIS_RIGHTX:
					currentControllerAxisState.RightRight = 0;
					currentControllerAxisState.RightLeft = 0;
					break;
				case SDL_CONTROLLER_AXIS_RIGHTY:
					currentControllerAxisState.RightDown = 0;
					currentControllerAxisState.RightUp = 0;
					break;
				case SDL_CONTROLLER_AXIS_TRIGGERLEFT:
					currentControllerAxisState.LTriggerDown = 0;
					break;
				case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:
					currentControllerAxisState.RTriggerDown = 0;
					break;
				}
			}
			break;
		}
	}
}

void
DisposePoll () {
	SDL_DestroyWindow (window);
	SDL_Quit ();
}

struct ConfigValue
StringToConfigEnum (char *value) {
	struct ConfigValue rval = { 0 };
	for (int i = 0; i < COUNTOFARR (ConfigKeyboardButtons); ++i)
		if (!strcmp (value, ConfigKeyboardButtons[i].string)) {
			rval.type = keycode;
			rval.keycode = ConfigKeyboardButtons[i].keycode;
			return rval;
		}
	for (int i = 0; i < COUNTOFARR (ConfigControllerButtons); ++i)
		if (!strcmp (value, ConfigControllerButtons[i].string)) {
			rval.type = button;
			rval.button = ConfigControllerButtons[i].button;
			return rval;
		}
	for (int i = 0; i < COUNTOFARR (ConfigControllerAXIS); ++i)
		if (!strcmp (value, ConfigControllerAXIS[i].string)) {
			rval.type = axis;
			rval.axis = ConfigControllerAXIS[i].axis;
			return rval;
		}

	printf ("Error at StringToConfigEnum (%s): Unknown value\n", value);
	return rval;
}

struct InternalButtonState
GetInternalButtonState (struct Keybindings bindings) {
	struct InternalButtonState buttons = { 0 };

	for (int i = 0; i < COUNTOFARR (ConfigKeyboardButtons); i++) {
		if (bindings.keycodes[i] == 0)
			continue;
		if (KeyboardIsReleased (bindings.keycodes[i]))
			buttons.Released = 1;
		if (KeyboardIsDown (bindings.keycodes[i]))
			buttons.Down = 1;
		if (KeyboardIsTapped (bindings.keycodes[i]))
			buttons.Tapped = 1;
	}
	for (int i = 0; i < COUNTOFARR (ConfigControllerButtons); i++) {
		if (bindings.buttons[i] == SDL_CONTROLLER_BUTTON_INVALID)
			continue;
		if (ControllerButtonIsReleased (bindings.buttons[i]))
			buttons.Released = 1;
		if (ControllerButtonIsDown (bindings.buttons[i]))
			buttons.Down = 1;
		if (ControllerButtonIsTapped (bindings.buttons[i]))
			buttons.Tapped = 1;
	}
	for (int i = 0; i < COUNTOFARR (ConfigControllerAXIS); i++) {
		if (bindings.axis[i] == 0)
			continue;
		if (ControllerAxisIsReleased (bindings.axis[i]))
			buttons.Released = 1;
		if (ControllerAxisIsDown (bindings.axis[i]))
			buttons.Down = 1;
		if (ControllerAxisIsTapped (bindings.axis[i]))
			buttons.Tapped = 1;
	}

	return buttons;
}

void
SetRumble (int left, int right) {
	for (int i = 0; i < COUNTOFARR (controllers); i++) {
		if (!controllers[i] || !SDL_GameControllerHasRumble (controllers[i]))
			continue;

		SDL_GameControllerRumble (controllers[i], left, right, 1000);
	}
}

inline bool
KeyboardIsDown (BYTE keycode) {
	return currentKeyboardState[keycode];
}

inline bool
KeyboardIsUp (BYTE keycode) {
	return !KeyboardIsDown (keycode);
}

inline bool
KeyboardIsTapped (BYTE keycode) {
	return KeyboardIsDown (keycode) && KeyboardWasUp (keycode);
}

inline bool
KeyboardIsReleased (BYTE keycode) {
	return KeyboardIsUp (keycode) && KeyboardWasDown (keycode);
}

inline bool
KeyboardWasDown (BYTE keycode) {
	return lastKeyboardState[keycode];
}

inline bool
KeyboardWasUp (BYTE keycode) {
	return !KeyboardWasDown (keycode);
}

inline POINT
GetMousePosition () {
	return currentMouseState.Position;
}

inline POINT
GetLastMousePosition () {
	return lastMouseState.Position;
}

inline POINT
GetMouseRelativePosition () {
	return currentMouseState.RelativePosition;
}

inline POINT
GetLastMouseRelativePosition () {
	return lastMouseState.RelativePosition;
}

inline void
SetMousePosition (POINT new) {
	currentMouseState.Position = new;
}

inline bool
GetMouseScrollUp () {
	return currentMouseState.ScrolledUp;
}

inline bool
GetMouseScrollDown () {
	return currentMouseState.ScrolledDown;
}

inline bool
ControllerButtonIsDown (SDL_GameControllerButton button) {
	return currentControllerButtonsState[button];
}

inline bool
ControllerButtonIsUp (SDL_GameControllerButton button) {
	return !ControllerButtonIsDown (button);
}

inline bool
ControllerButtonWasDown (SDL_GameControllerButton button) {
	return lastControllerButtonsState[button];
}

inline bool
ControllerButtonWasUp (SDL_GameControllerButton button) {
	return !ControllerButtonWasDown (button);
}

inline bool
ControllerButtonIsTapped (SDL_GameControllerButton button) {
	return ControllerButtonIsDown (button) && ControllerButtonWasUp (button);
}

inline bool
ControllerButtonIsReleased (SDL_GameControllerButton button) {
	return ControllerButtonIsUp (button) && ControllerButtonWasDown (button);
}

inline bool
ControllerAxisIsDown (enum SDLAxis axis) {
	switch (axis) {
	case SDL_AXIS_LEFT_LEFT:
		return currentControllerAxisState.LeftLeft;
	case SDL_AXIS_LEFT_RIGHT:
		return currentControllerAxisState.LeftRight;
	case SDL_AXIS_LEFT_UP:
		return currentControllerAxisState.LeftUp;
	case SDL_AXIS_LEFT_DOWN:
		return currentControllerAxisState.LeftDown;
	case SDL_AXIS_RIGHT_LEFT:
		return currentControllerAxisState.RightLeft;
	case SDL_AXIS_RIGHT_RIGHT:
		return currentControllerAxisState.RightRight;
	case SDL_AXIS_RIGHT_UP:
		return currentControllerAxisState.RightUp;
	case SDL_AXIS_RIGHT_DOWN:
		return currentControllerAxisState.RightDown;
	case SDL_AXIS_LTRIGGER_DOWN:
		return currentControllerAxisState.LTriggerDown;
	case SDL_AXIS_RTRIGGER_DOWN:
		return currentControllerAxisState.RTriggerDown;
	case SDL_AXIS_NULL:
	case SDL_AXIS_MAX:
		return false;
	}
}

inline bool
ControllerAxisIsUp (enum SDLAxis axis) {
	return !ControllerAxisIsDown (axis);
}

inline bool
ControllerAxisWasDown (enum SDLAxis axis) {
	switch (axis) {
	case SDL_AXIS_LEFT_LEFT:
		return lastControllerAxisState.LeftLeft;
	case SDL_AXIS_LEFT_RIGHT:
		return lastControllerAxisState.LeftRight;
	case SDL_AXIS_LEFT_UP:
		return lastControllerAxisState.LeftUp;
	case SDL_AXIS_LEFT_DOWN:
		return lastControllerAxisState.LeftDown;
	case SDL_AXIS_RIGHT_LEFT:
		return lastControllerAxisState.RightLeft;
	case SDL_AXIS_RIGHT_RIGHT:
		return lastControllerAxisState.RightRight;
	case SDL_AXIS_RIGHT_UP:
		return lastControllerAxisState.RightUp;
	case SDL_AXIS_RIGHT_DOWN:
		return lastControllerAxisState.RightDown;
	case SDL_AXIS_LTRIGGER_DOWN:
		return lastControllerAxisState.LTriggerDown;
	case SDL_AXIS_RTRIGGER_DOWN:
		return lastControllerAxisState.RTriggerDown;
	case SDL_AXIS_NULL:
	case SDL_AXIS_MAX:
		return false;
	}
}

inline bool
ControllerAxisWasUp (enum SDLAxis axis) {
	return !ControllerAxisWasDown (axis);
}

inline bool
ControllerAxisIsTapped (enum SDLAxis axis) {
	return ControllerAxisIsDown (axis) && ControllerAxisWasUp (axis);
}

inline bool
ControllerAxisIsReleased (enum SDLAxis axis) {
	return ControllerAxisIsUp (axis) && ControllerAxisWasDown (axis);
}

inline bool
IsButtonTapped (struct Keybindings bindings) {
	return GetInternalButtonState (bindings).Tapped;
}

inline bool
IsButtonReleased (struct Keybindings bindings) {
	return GetInternalButtonState (bindings).Released;
}

inline bool
IsButtonDown (struct Keybindings bindings) {
	return GetInternalButtonState (bindings).Down;
}
