#define SDL_MAIN_HANDLED
#include "io.h"
#include "helpers.h"
#include <SDL.h>
#include <math.h>
#include <stdio.h>

bool IsMouseScrollUp = false;
bool IsMouseScrollDown = false;
bool unlockedCamera = false;
bool HasWindowFocus = false;
bool currentKeyboardState[0xFF];
bool lastKeyboardState[0xFF];
bool isPaused = false;
int rumbleIntensity = 8000;

struct ContactPoint {
	float Position;
	bool InContact;
} ContactPoints[2];

struct TouchSliderState {
	uint8_t Padding0000[112];

	int32_t State;

	uint8_t Padding0074[20 + 12];

	int32_t SensorPressureLevels[32];

	uint8_t Padding0108[52 - 12];

	float SectionPositions[4];
	int SectionConnections[4];
	uint8_t Padding015C[4];
	bool SectionTouched[4];

	uint8_t Padding013C[3128 - 52 - 40];

	struct {
		uint8_t Padding00[2];
		bool IsTouched;
		uint8_t Padding[45];
	} SensorTouched[32];
} * sliderState;

enum JvsButtons : uint32_t {
	JVS_NONE = 0 << 0x00,

	JVS_TEST = 1 << 0x00,
	JVS_SERVICE = 1 << 0x01,

	JVS_START = 1 << 0x02,
	JVS_TRIANGLE = 1 << 0x07,
	JVS_SQUARE = 1 << 0x08,
	JVS_CROSS = 1 << 0x09,
	JVS_CIRCLE = 1 << 0x0A,
	JVS_L = 1 << 0x0B,
	JVS_R = 1 << 0x0C,

	JVS_SW1 = 1 << 0x12,
	JVS_SW2 = 1 << 0x13,
} lastInputState;

union ButtonState {
	enum JvsButtons Buttons;
	uint32_t State[4];
};

struct InternalButtonState {
	unsigned int Released : 1;
	unsigned int Down : 1;
	unsigned int Tapped : 1;
};

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

struct Keybindings {
	BYTE keycodes[COUNTOFARR (ConfigKeyboardButtons)];
	SDL_GameControllerButton buttons[COUNTOFARR (ConfigControllerButtons)];
	enum SDLAxis axis[COUNTOFARR (ConfigControllerAXIS)];
};

enum EnumType { none, keycode, button, axis };

struct ConfigValue {
	enum EnumType type;
	BYTE keycode;
	SDL_GameControllerButton button;
	enum SDLAxis axis;
};

struct InputState {
	union ButtonState Tapped;
	union ButtonState Released;

	union ButtonState Down;
	uint32_t Padding_20[4];

	union ButtonState DoubleTapped;
	uint32_t Padding_30[4];

	union ButtonState IntervalTapped;
	uint32_t Padding_38[12];

	int32_t MouseX;
	int32_t MouseY;
	int32_t MouseDeltaX;
	int32_t MouseDeltaY;

	uint32_t Padding_AC[8];
	uint8_t Padding_D0[3];
	char Key;
} * inputState;

struct TouchPanelState {
	int32_t Padding00[0x1E];
	int32_t ConnectionState;
	int32_t Padding01[0x06];
	float XPosition;
	float YPosition;
	float Pressure;
	int32_t ContactType;
} * currentTouchPanelState;

struct MouseState {
	POINT Position;
	POINT RelativePosition;
	long MouseWheel;
	bool ScrolledUp;
	bool ScrolledDown;
} currentMouseState, lastMouseState;

struct TargetState {
	struct TargetState *prev;
	struct TargetState *next;
	uint8_t padding10[0x4];
	int32_t tgtType;
	float tgtRemainingTime;
	uint8_t padding1C[0x434];
	uint8_t ToBeRemoved;
	uint8_t padding451[0xB];
	int32_t tgtHitState;
	uint8_t padding460[0x48];
} * targetStates;

struct DwGuiDisplay {
	void *vftable;
	void *active;
	void *cap;
	void *on;
	void *move;
	void *widget;
} * dwGuiDisplay;

struct KeyBit {
	uint32_t bit;
	uint8_t keycode;
};

struct Camera {
	float PositionX;
	float PositionY;
	float PositionZ;
	float FocusX;
	float FocusY;
	float FocusZ;
	float Rotation;
	float HorizontalFov;
	float VerticalFov;
} * camera;

struct RawFont {
	uint32_t sprId;
	uint8_t width1;
	uint8_t height1;
	uint8_t width2;
	uint8_t height2;
	uint8_t layoutParam2Num;
	uint8_t layoutParam2Div;
	uint8_t padding0a[0x02];
	int32_t fontmapId;
	float layoutParam2NumOverDiv;
	uint8_t padding14[0x04];
	uint64_t texWidthChars;
	int64_t dataBegin;
	int64_t dataEnd;
	int64_t dataCapacityEnd;
	uint8_t layoutParam1;
	uint8_t padding39[0x7];
};

struct FontInfo {
	uint32_t fontId;
	uint8_t padding04[0x4];
	struct RawFont *rawfont;
	uint16_t flag10;
	uint8_t padding12[0x02];
	float width1;
	float height1;
	float width2;
	float height2;
	float userSizeWidth;
	float userSizeHeight;
	float userSizeWidthMultiplier;
	float userSizeHeightMultiplier;
	float spacingWidth;
	float spacingHeight;
};

struct DrawParams {
	uint32_t colour;
	uint32_t fillColour;
	uint8_t clip;
	uint8_t unk09[0x3];
	float clipRectX;
	float clipRectY;
	float clipRectWidth;
	float clipRectHeight;
	uint32_t layer;
	uint32_t unk20;
	uint32_t unk24;
	uint32_t unk28;
	float textCurrentLocX;
	float textCurrentLocY;
	float lineOriginLocX;
	float lineOriginLocY;
	uint8_t padding3c[0x4];
	uint64_t lineLength;
	struct FontInfo *font;
	uint16_t unk50;
};

struct Rectangle {
	float x;
	float y;
	float width;
	float height;
};

void DrawPauseMenu ();
void DrawTestMenu ();

bool IsButtonTapped (struct Keybindings bindings);
void PollMouseInput ();
bool KeyboardIsDown (BYTE keycode);
bool KeyboardIsUp (BYTE keycode);
bool KeyboardIsTapped (BYTE keycode);
bool KeyboardIsReleased (BYTE keycode);
bool KeyboardWasDown (BYTE keycode);
bool KeyboardWasUp (BYTE keycode);
void PollSDLInput ();
void SetConfigValue (toml_table_t *table, char *key,
					 struct Keybindings *keybind);
void UpdateTouch ();
void UpdateInputUnfocused ();
void UpdateInput ();
void UpdateDwGuiInput ();
void UpdateUnlockedCamera (HWND DivaWindowHandle);
void SetSensor (int index, int value);
void EndRumble ();

struct Keybindings TEST = { .keycodes = { VK_F1 } };
struct Keybindings SERVICE = { .keycodes = { VK_F2 } };
struct Keybindings ADVERTISE = { .keycodes = { VK_F4 } };
struct Keybindings GAME = { .keycodes = { VK_F5 } };
struct Keybindings DATA_TEST = { .keycodes = { VK_F6 } };
struct Keybindings TEST_MODE = { .keycodes = { VK_F7 } };
struct Keybindings APP_ERROR = { .keycodes = { VK_F8 } };

struct Keybindings START = { .keycodes = { VK_RETURN },
							 .buttons = { SDL_CONTROLLER_BUTTON_START } };
struct Keybindings TRIANGLE = { .keycodes = { 'W', 'I' },
								.buttons = { SDL_CONTROLLER_BUTTON_Y,
											 SDL_CONTROLLER_BUTTON_DPAD_UP } };
struct Keybindings SQUARE = { .keycodes = { 'A', 'J' },
							  .buttons = { SDL_CONTROLLER_BUTTON_X,
										   SDL_CONTROLLER_BUTTON_DPAD_LEFT } };
struct Keybindings CROSS = { .keycodes = { 'S', 'K' },
							 .buttons = { SDL_CONTROLLER_BUTTON_A,
										  SDL_CONTROLLER_BUTTON_DPAD_DOWN } };
struct Keybindings CIRCLE
	= { .keycodes = { 'D', 'L' },
		.buttons
		= { SDL_CONTROLLER_BUTTON_B, SDL_CONTROLLER_BUTTON_DPAD_RIGHT } };
struct Keybindings LEFT_LEFT
	= { .keycodes = { 'Q' }, .axis = { SDL_AXIS_LEFT_LEFT } };
struct Keybindings LEFT_RIGHT
	= { .keycodes = { 'E' }, .axis = { SDL_AXIS_LEFT_RIGHT } };
struct Keybindings RIGHT_LEFT
	= { .keycodes = { 'U' }, .axis = { SDL_AXIS_RIGHT_LEFT } };
struct Keybindings RIGHT_RIGHT
	= { .keycodes = { 'O' }, .axis = { SDL_AXIS_RIGHT_RIGHT } };

struct Keybindings CAMERA_UNLOCK_TOGGLE
	= { .keycodes = { VK_F3 }, .buttons = { SDL_CONTROLLER_BUTTON_BACK } };
struct Keybindings CAMERA_MOVE_FORWARD
	= { .keycodes = { 'W' }, .axis = { SDL_AXIS_LEFT_UP } };
struct Keybindings CAMERA_MOVE_BACKWARD
	= { .keycodes = { 'S' }, .axis = { SDL_AXIS_LEFT_DOWN } };
struct Keybindings CAMERA_MOVE_LEFT
	= { .keycodes = { 'A' }, .axis = { SDL_AXIS_LEFT_LEFT } };
struct Keybindings CAMERA_MOVE_RIGHT
	= { .keycodes = { 'D' }, .axis = { SDL_AXIS_LEFT_RIGHT } };
struct Keybindings CAMERA_MOVE_UP
	= { .keycodes = { VK_SPACE }, .buttons = { SDL_CONTROLLER_BUTTON_A } };
struct Keybindings CAMERA_MOVE_DOWN
	= { .keycodes = { VK_CONTROL }, .buttons = { SDL_CONTROLLER_BUTTON_B } };
struct Keybindings CAMERA_ROTATE_CW
	= { .keycodes = { 'E' },
		.buttons = { SDL_CONTROLLER_BUTTON_RIGHTSHOULDER } };
struct Keybindings CAMERA_ROTATE_CCW
	= { .keycodes = { 'Q' },
		.buttons = { SDL_CONTROLLER_BUTTON_LEFTSHOULDER } };
struct Keybindings CAMERA_ZOOM_IN
	= { .keycodes = { 'R' }, .axis = { SDL_AXIS_LTRIGGER_DOWN } };
struct Keybindings CAMERA_ZOOM_OUT
	= { .keycodes = { 'F' }, .axis = { SDL_AXIS_RTRIGGER_DOWN } };
struct Keybindings CAMERA_MOVE_FAST
	= { .keycodes = { VK_SHIFT }, .buttons = { SDL_CONTROLLER_BUTTON_X } };
struct Keybindings CAMERA_MOVE_SLOW
	= { .keycodes = { VK_TAB }, .buttons = { SDL_CONTROLLER_BUTTON_Y } };

struct KeyBit keyBits[20] = {
	{ 5, VK_LEFT },		{ 6, VK_RIGHT },

	{ 29, VK_SPACE },	{ 39, 'A' },		{ 43, 'E' },
	{ 42, 'D' },		{ 55, 'Q' },		{ 57, 'S' },
	{ 61, 'W' },		{ 63, 'Y' },		{ 84, 'L' },

	{ 80, VK_RETURN },	{ 81, VK_SHIFT },	{ 82, VK_CONTROL },
	{ 83, VK_MENU },

	{ 91, VK_UP },		{ 93, VK_DOWN },

	{ 96, MK_LBUTTON }, { 97, VK_MBUTTON }, { 98, MK_RBUTTON },
};

bool giveUp = false;
HOOK (bool, __stdcall, GiveUp, 0x14010EF00, void *cls) {
	if (giveUp) {
		giveUp = false;
		return true;
	}
	return originalGiveUp (cls);
}

SDL_Window *window;
SDL_GameController *controllers[255];

void
InitializeIO (HWND DivaWindowHandle) {
	toml_table_t *config = openConfig (configPath ("keyconfig.toml"));
	if (!config)
		return;

	SetConfigValue (config, "TEST", &TEST);
	SetConfigValue (config, "SERVICE", &SERVICE);
	SetConfigValue (config, "ADVERTISE", &ADVERTISE);
	SetConfigValue (config, "GAME", &GAME);
	SetConfigValue (config, "DATA_TEST", &DATA_TEST);
	SetConfigValue (config, "TEST_MODE", &TEST_MODE);
	SetConfigValue (config, "APP_ERROR", &APP_ERROR);

	SetConfigValue (config, "START", &START);
	SetConfigValue (config, "TRIANGLE", &TRIANGLE);
	SetConfigValue (config, "SQUARE", &SQUARE);
	SetConfigValue (config, "CROSS", &CROSS);
	SetConfigValue (config, "CIRCLE", &CIRCLE);
	SetConfigValue (config, "LEFT_LEFT", &LEFT_LEFT);
	SetConfigValue (config, "LEFT_RIGHT", &LEFT_RIGHT);
	SetConfigValue (config, "RIGHT_LEFT", &RIGHT_LEFT);
	SetConfigValue (config, "RIGHT_RIGHT", &RIGHT_RIGHT);

	SetConfigValue (config, "CAMERA_UNLOCK_TOGGLE", &CAMERA_UNLOCK_TOGGLE);
	SetConfigValue (config, "CAMERA_MOVE_FORWARD", &CAMERA_MOVE_FORWARD);
	SetConfigValue (config, "CAMERA_MOVE_BACKWARD", &CAMERA_MOVE_BACKWARD);
	SetConfigValue (config, "CAMERA_MOVE_LEFT", &CAMERA_MOVE_LEFT);
	SetConfigValue (config, "CAMERA_MOVE_RIGHT", &CAMERA_MOVE_RIGHT);
	SetConfigValue (config, "CAMERA_MOVE_UP", &CAMERA_MOVE_UP);
	SetConfigValue (config, "CAMERA_MOVE_DOWN", &CAMERA_MOVE_DOWN);
	SetConfigValue (config, "CAMERA_ROTATE_CW", &CAMERA_ROTATE_CW);
	SetConfigValue (config, "CAMERA_ROTATE_CCW", &CAMERA_ROTATE_CCW);
	SetConfigValue (config, "CAMERA_ZOOM_IN", &CAMERA_ZOOM_IN);
	SetConfigValue (config, "CAMERA_ZOOM_OUT", &CAMERA_ZOOM_OUT);
	SetConfigValue (config, "CAMERA_MOVE_FAST", &CAMERA_MOVE_FAST);
	SetConfigValue (config, "CAMERA_MOVE_SLOW", &CAMERA_MOVE_SLOW);

	toml_free (config);

	config = openConfig (configPath ("config.toml"));
	if (!config)
		return;

	rumbleIntensity
		= 65535 * readConfigInt (config, "rumbleIntensity", 25) / 100;

	toml_free (config);

	SDL_SetMainReady ();
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
		printf ("Error at InitializeIO (): Cannot read "
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
		if (rumbleIntensity != 0 && !SDL_GameControllerHasRumble (controller))
			printf ("Warning: Controller %s does not support rumble\n",
					SDL_GameControllerName (controller));

		controllers[i] = controller;
		break;
	}

	window = SDL_CreateWindowFrom (DivaWindowHandle);
	if (window != NULL)
		SDL_SetWindowResizable (window, true);
	else
		printf ("Error at SDL_CreateWindowFrom (DivaWindowHandle): %s\n",
				SDL_GetError ());

	sliderState = (struct TouchSliderState *)0x14CC5DE40;
	inputState = (struct InputState *)(*(uint64_t *)(void *)0x140EDA330);
	currentTouchPanelState = (struct TouchPanelState *)0x14CC9EC30;
	targetStates = (struct TargetState *)0x140D0B688;
	dwGuiDisplay = (struct DwGuiDisplay *)*(uint64_t *)0x141190108;
	camera = (struct Camera *)0x140FBC2C0;

	INSTALL_HOOK (GiveUp);
}

void
UpdateIO (HWND DivaWindowHandle) {
	bool HadWindowFocus = HasWindowFocus;
	HasWindowFocus = DivaWindowHandle == NULL
					 || GetForegroundWindow () == DivaWindowHandle;

	currentTouchPanelState->ConnectionState = 1;
	sliderState->State = 3;

	if (HasWindowFocus) {
		memcpy (lastKeyboardState, currentKeyboardState,
				sizeof (currentKeyboardState));

		for (BYTE i = 0; i < 0xFF; i++)
			currentKeyboardState[i] = GetAsyncKeyState (i) != 0;

		PollSDLInput ();
		PollMouseInput (DivaWindowHandle);

		if (KeyboardIsDown (VK_ESCAPE))
			*(bool *)0x140EDA6B0 = true;

		UpdateUnlockedCamera (DivaWindowHandle);
		if (unlockedCamera || isPaused)
			return;

		UpdateInput ();
		UpdateTouch ();
		UpdateDwGuiInput ();

		if (dwGuiDisplay->on)
			return;

		/* Scrolling through menus */
		int *slotsToScroll = (int *)0x14CC12470;
		int *modulesToScroll = (int *)0x1418047EC;
		int pvSlotsConst = *(int *)0x14CC119C8;
		int moduleIsReccomended = *(int *)0x1418047E0;

		if (IsMouseScrollUp) {
			if (pvSlotsConst < 26)
				*slotsToScroll -= 1;
			if (moduleIsReccomended == 0)
				*modulesToScroll -= 1;
			IsMouseScrollUp = false;
		}
		if (IsMouseScrollDown) {
			if (pvSlotsConst < 26)
				*slotsToScroll += 1;
			if (moduleIsReccomended == 0)
				*modulesToScroll += 1;
			IsMouseScrollDown = false;
		}
	} else if (HadWindowFocus) {
		/* Clear state when focus lost */
		currentTouchPanelState->XPosition = 0.0f;
		currentTouchPanelState->YPosition = 0.0f;
		currentTouchPanelState->ContactType = 0;
		currentTouchPanelState->Pressure = 0.0f;
		inputState->Released.Buttons = 0;
		inputState->Down.Buttons = 0;

		inputState->MouseX = 0;
		inputState->MouseY = 0;
		inputState->MouseDeltaX = 0;
		inputState->MouseDeltaY = 0;

		for (int i = 0; i < 32; i++)
			SetSensor (i, 0);
		for (int i = 0; i < 2; i++) {
			sliderState->SectionTouched[i] = false;
			sliderState->SectionPositions[i] = 0.0f;
		}

		EndRumble ();
	}
}

FUNCTION_PTR (void, __stdcall, DivaDrawText, 0x140198500,
			  struct DrawParams *drawParam, uint32_t flags, const char *text,
			  int64_t len);
FUNCTION_PTR (struct FontInfo *, __thiscall, GetFontInfoFromID, 0x140196510,
			  struct FontInfo *fontInfo, uint32_t id);
FUNCTION_PTR (void, __stdcall, FillRectangle, 0x140198D80,
			  struct DrawParams *drawParam, const struct Rectangle *rect);
void
DrawMenu (int index, const char **items, int items_len) {
	struct FontInfo fontInfo;
	fontInfo = *GetFontInfoFromID (&fontInfo, 0x11);

	struct DrawParams drawParam = { 0 };
	drawParam.colour = 0xFF000000;
	drawParam.fillColour = 0xFF000000;
	drawParam.layer = 0x18;
	drawParam.unk24 = 0xD;
	drawParam.textCurrentLocX = (1280 / 2) - 100;
	drawParam.textCurrentLocY = (720 / 2) - (items_len / 2 * 24);
	drawParam.font = &fontInfo;
	drawParam.unk50 = 0x25A1;

	struct Rectangle rect = { 0 };
	rect.width = 1280;
	rect.height = 720;
	FillRectangle (&drawParam, &rect);
	drawParam.layer = 0x19;
	for (int i = 0; i < items_len; i++) {
		char buf[32];
		sprintf (buf, "%s\n", items[i]);
		drawParam.colour = (i == index ? 0xFF00FFFF : 0xFFFFFFFF);
		DivaDrawText (&drawParam, 0x1005, buf, 32);
	}
}

void
Update2DIO () {
	DrawTestMenu ();
	DrawPauseMenu ();
}

const char *PauseMenuItems[] = { "RESUME", "RESTART", "GIVE UP" };
bool firstPauseFrame = true;
int pauseIndex = 0;
FUNCTION_PTR (void, __stdcall, PauseDSC, 0x1401295C0);
FUNCTION_PTR (void, __stdcall, UnPauseDSC, 0x140129590);
FUNCTION_PTR (void, __stdcall, LoadPV, 0x1400FDDC0, uint64_t cls);
uint8_t aetMoveOriginal[8];
uint8_t framespeedOriginal[4];
uint8_t ageageHairOriginal[3];
bool playstates[8]; /* Should be enough */
void
DrawPauseMenu () {
	if (*(uint32_t *)0x140EDA82C != 13)
		isPaused = false;
	if (!isPaused)
		return;

	struct Keybindings UP = { .keycodes = { VK_UP },
							  .buttons = { SDL_CONTROLLER_BUTTON_DPAD_UP },
							  .axis = { SDL_AXIS_LEFT_UP } };
	struct Keybindings DOWN = { .keycodes = { VK_DOWN },
								.buttons = { SDL_CONTROLLER_BUTTON_DPAD_DOWN },
								.axis = { SDL_AXIS_LEFT_DOWN } };

	if (IsButtonTapped (UP))
		pauseIndex--;
	if (IsButtonTapped (DOWN))
		pauseIndex++;

	if (pauseIndex > 2)
		pauseIndex = 0;
	if (pauseIndex < 0)
		pauseIndex = 2;

	if (firstPauseFrame) {
		/* Clear state */
		currentTouchPanelState->XPosition = 0.0f;
		currentTouchPanelState->YPosition = 0.0f;
		currentTouchPanelState->ContactType = 0;
		currentTouchPanelState->Pressure = 0.0f;
		inputState->Released.Buttons = 0;
		inputState->Down.Buttons = 0;

		inputState->MouseX = 0;
		inputState->MouseY = 0;
		inputState->MouseDeltaX = 0;
		inputState->MouseDeltaY = 0;

		for (int i = 0; i < 32; i++)
			SetSensor (i, 0);
		for (int i = 0; i < 2; i++) {
			sliderState->SectionTouched[i] = false;
			sliderState->SectionPositions[i] = 0.0f;
		}

		EndRumble ();

		firstPauseFrame = false;
		PauseDSC ();
		memcpy (&aetMoveOriginal, (void *)0x1401703B3, 8);
		memcpy (&framespeedOriginal, (void *)0x140192D50, 4);
		memcpy (&ageageHairOriginal, (void *)0x14054352C, 3);
		WRITE_NOP (0x1401703B3, 8);
		WRITE_MEMORY (0x140192D50, uint8_t, 0x0F, 0x57, 0xC0, 0xC3);
		WRITE_MEMORY (0x14054352C, uint8_t, 0x0F, 0x57, 0xDB);

		uint64_t audioMixer = *(uint64_t *)0x14CC61190;
		uint64_t audioStreams = *(uint64_t *)(audioMixer + 0x18);
		int32_t audioStreamsLen = *(uint64_t *)(audioMixer + 0x20);

		for (int i = 0; i < audioStreamsLen; i++) {
			uint32_t *state = (uint32_t *)(audioStreams + i * 0x50 + 0x18);
			playstates[i] = *state;
			*state = 0;
		}
	} else if (IsButtonTapped (START) || IsButtonTapped (CIRCLE)) {
		firstPauseFrame = true;
		isPaused = false;
		if (pauseIndex == 2)
			giveUp = true;

		UnPauseDSC ();
		for (int i = 0; i < 8; i++)
			WRITE_MEMORY (0x1401703B3 + i, uint8_t, aetMoveOriginal[i]);
		for (int i = 0; i < 4; i++)
			WRITE_MEMORY (0x140192D50 + i, uint8_t, framespeedOriginal[i]);
		for (int i = 0; i < 3; i++)
			WRITE_MEMORY (0x14054352C + i, uint8_t, ageageHairOriginal[i]);

		uint64_t audioMixer = *(uint64_t *)0x14CC61190;
		uint64_t audioStreams = *(uint64_t *)(audioMixer + 0x18);
		int32_t audioStreamsLen = *(uint64_t *)(audioMixer + 0x20);

		for (int i = 0; i < audioStreamsLen; i++) {
			uint32_t *state = (uint32_t *)(audioStreams + i * 0x50 + 0x18);
			*state = playstates[i];
		}

		if (pauseIndex != 1)
			return;

		WRITE_MEMORY (0x1401038CD, uint8_t, 0x15);
		WRITE_MEMORY (0x140103B94, uint8_t, 0x18);
		*(uint8_t *)0x140D0B512 = 0;
		*(uint8_t *)0x140D0B524 = 8;
		*(int32_t *)0x140CDD8D8 = 0x11;
		while (*(int32_t *)0x140CDD8D8 < 0x18)
			LoadPV (0x140CDD8D0);
		*(int32_t *)0x140D0A9BC = 0;
		*(int32_t *)0x140D0A9B8 = 0;
		*(int32_t *)0x140D0A9C0 = 0;
		*(uint8_t *)0x140D0A50C = 0;
		*(int32_t *)0x140D0AA0F = 0;
		WRITE_MEMORY (0x1401038CD, uint8_t, 0x12);
		WRITE_MEMORY (0x140103B94, uint8_t, 0x16);

		return;
	}

	DrawMenu (pauseIndex, PauseMenuItems, 3);
}

const char *DataTestNames[] = {
	"MAIN TEST",	 "MISC TEST",	   "OBJECT TEST",	"STAGE TEST",
	"MOTION TEST",	 "COLLISION TEST", "SPRITE TEST",	"2DAUTH TEST",
	"3DAUTH TEST",	 "CHARA TEST",	   "ITEM TEST",		"PERFORMANCE TEST",
	"PVSCRIPT TEST", "PRINT TEST",	   "CARD TEST",		"OPD TEST",
	"SLIDER TEST",	 "GLITTER TEST",   "GRAPHICS TEST", "COLLECTION CARD TEST",
};
bool ShouldDrawTestMenu = false;
int dataTestIndex = 20;
FUNCTION_PTR (void, __stdcall, ChangeSubState, 0x140195260, uint32_t gameState,
			  uint32_t subState);
void
DrawTestMenu () {
	if (*(uint32_t *)0x140EDA810 != 3)
		ShouldDrawTestMenu = false;
	if (!ShouldDrawTestMenu)
		return;

	struct Keybindings UP = { .keycodes = { VK_UP },
							  .buttons = { SDL_CONTROLLER_BUTTON_DPAD_UP },
							  .axis = { SDL_AXIS_LEFT_UP } };
	struct Keybindings DOWN = { .keycodes = { VK_DOWN },
								.buttons = { SDL_CONTROLLER_BUTTON_DPAD_DOWN },
								.axis = { SDL_AXIS_LEFT_DOWN } };

	if (IsButtonTapped (UP))
		dataTestIndex--;
	if (IsButtonTapped (DOWN))
		dataTestIndex++;

	if (dataTestIndex > 39)
		dataTestIndex = 20;
	if (dataTestIndex < 20)
		dataTestIndex = 39;

	if (IsButtonTapped (START)) {
		ShouldDrawTestMenu = false;
		ChangeSubState (3, dataTestIndex - 1);
	}

	DrawMenu (dataTestIndex - 20, DataTestNames, 20);
}

void
DiposeIO () {
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
PollMouseInput (HWND DivaWindowHandle) {
	lastMouseState = currentMouseState;

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

int deadzone = 8000;
bool currentControllerButtonsState[SDL_CONTROLLER_BUTTON_MAX];
bool lastControllerButtonsState[SDL_CONTROLLER_BUTTON_MAX];
struct SDLAxisState currentControllerAxisState;
struct SDLAxisState lastControllerAxisState;

void
PollSDLInput () {
	memcpy (lastControllerButtonsState, currentControllerButtonsState,
			SDL_CONTROLLER_BUTTON_MAX);
	lastControllerAxisState = currentControllerAxisState;

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
			if (rumbleIntensity != 0
				&& !SDL_GameControllerHasRumble (controller))
				printf ("Warning: Controller %s does not support rumble\n",
						SDL_GameControllerName (controller));

			controllers[event.cdevice.which] = controller;
			break;
		case SDL_CONTROLLERDEVICEREMOVED:
			if (!SDL_IsGameController (event.cdevice.which))
				break;
			SDL_GameControllerClose (controllers[event.cdevice.which]);

			break;
		case SDL_MOUSEWHEEL:
			if (event.wheel.y > 0)
				IsMouseScrollUp = true;
			else if (event.wheel.y < 0)
				IsMouseScrollDown = true;
			break;
		case SDL_CONTROLLERBUTTONUP:
		case SDL_CONTROLLERBUTTONDOWN:
			currentControllerButtonsState[event.cbutton.button]
				= event.cbutton.state;
			break;
		case SDL_CONTROLLERAXISMOTION:
			if (event.caxis.value > deadzone) {
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
			} else if (event.caxis.value < -deadzone) {
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

void
UpdateTouch () {
	if (dwGuiDisplay->active || dwGuiDisplay->on)
		return;

	currentTouchPanelState->XPosition
		= (float)currentMouseState.RelativePosition.x;
	currentTouchPanelState->YPosition
		= (float)currentMouseState.RelativePosition.y;

	currentTouchPanelState->ContactType = KeyboardIsDown (VK_LBUTTON) ? 2
										  : KeyboardIsReleased (VK_LBUTTON)
											  ? 1
											  : 0;

	currentTouchPanelState->Pressure
		= (float)(currentTouchPanelState->ContactType != 0);
}

float sliderIncrement = 16.6f / 750.0f;
float sensorStep = (1.0f / 32);

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

bool
IsButtonTapped (struct Keybindings bindings) {
	return GetInternalButtonState (bindings).Tapped;
}

bool
IsButtonReleased (struct Keybindings bindings) {
	return GetInternalButtonState (bindings).Released;
}

bool
IsButtonDown (struct Keybindings bindings) {
	return GetInternalButtonState (bindings).Down;
}

void
SetSensor (int index, int value) {
	if (index < 0 || index >= 32)
		return;

	sliderState->SensorPressureLevels[index] = value;
	sliderState->SensorTouched[index].IsTouched = value > 0;
}

void
ApplyContactPoint (struct ContactPoint contactPoint, int section) {
	sliderState->SectionTouched[section] = contactPoint.InContact;

	float position = contactPoint.Position;
	if (position > 1.0f)
		position = 1.0f;
	else if (position < 0.0f)
		position = 0.0f;

	if (contactPoint.InContact) {
		int sensor = (int)(position * (32 - 1));
		SetSensor (sensor, contactPoint.InContact ? 180 : 0);
	}

	sliderState->SectionPositions[section]
		= contactPoint.InContact ? -1.0f + (position * 2.0f) : 0.0f;
}

void
EmulateSliderInput (struct Keybindings left, struct Keybindings right,
					struct ContactPoint *contactPoint, float start,
					float end) {
	bool leftDown = IsButtonDown (left);
	bool rightDown = IsButtonDown (right);

	if (leftDown)
		contactPoint->Position -= sliderIncrement;
	else if (rightDown)
		contactPoint->Position += sliderIncrement;

	if (contactPoint->Position < start)
		contactPoint->Position = end;
	if (contactPoint->Position > end)
		contactPoint->Position = start;

	bool leftTapped = IsButtonTapped (left);
	bool rightTapped = IsButtonTapped (right);

	if (leftTapped || rightTapped)
		contactPoint->Position = (start + end) / 2.0f;

	contactPoint->InContact = leftDown || rightDown;
}

enum JvsButtons
GetButtonsState (bool (*buttonTestFunc) (struct Keybindings)) {
	enum JvsButtons buttons = { 0 };

	if (buttonTestFunc (TEST))
		buttons |= JVS_TEST;
	if (buttonTestFunc (SERVICE))
		buttons |= JVS_SERVICE;

	if (buttonTestFunc (START))
		buttons |= JVS_START;

	if (buttonTestFunc (TRIANGLE))
		buttons |= JVS_TRIANGLE;
	if (buttonTestFunc (SQUARE))
		buttons |= JVS_SQUARE;
	if (buttonTestFunc (CROSS))
		buttons |= JVS_CROSS;
	if (buttonTestFunc (CIRCLE))
		buttons |= JVS_CIRCLE;

	if (buttonTestFunc (LEFT_LEFT))
		buttons |= JVS_L;
	if (buttonTestFunc (LEFT_RIGHT))
		buttons |= JVS_R;

	return buttons;
}

void
EndRumble () {
	for (int i = 0; i < COUNTOFARR (controllers); i++) {
		if (!controllers[i] || !SDL_GameControllerHasRumble (controllers[i]))
			continue;

		SDL_GameControllerRumble (controllers[i], 0, 0, 1000);
	}
}

FUNCTION_PTR (void, __stdcall, ChangeGameState, 0x1401953D0,
			  uint32_t gameState);
void
UpdateInput () {
	if (IsButtonDown (ADVERTISE))
		ChangeGameState (1);
	if (IsButtonDown (GAME))
		ChangeGameState (2);
	if (IsButtonDown (DATA_TEST)) {
		ShouldDrawTestMenu = true;
		ChangeGameState (3);
	}
	if (IsButtonDown (TEST_MODE))
		ChangeGameState (4);
	if (IsButtonDown (APP_ERROR))
		ChangeGameState (5);

	if (*(uint32_t *)0x140EDA82C == 13)
		isPaused = IsButtonTapped (START);

	if (dwGuiDisplay->active)
		return;

	lastInputState = inputState->Down.Buttons;

	inputState->Tapped.Buttons = GetButtonsState (IsButtonTapped);
	inputState->Released.Buttons = GetButtonsState (IsButtonReleased);
	inputState->Down.Buttons = GetButtonsState (IsButtonDown);
	inputState->DoubleTapped.Buttons = GetButtonsState (IsButtonTapped);
	inputState->IntervalTapped.Buttons = GetButtonsState (IsButtonTapped);

	if ((lastInputState &= inputState->Tapped.Buttons) != 0)
		inputState->Down.Buttons ^= inputState->Tapped.Buttons;

	EmulateSliderInput (LEFT_LEFT, LEFT_RIGHT, &ContactPoints[0], 0.0f, 0.5f);
	EmulateSliderInput (RIGHT_LEFT, RIGHT_RIGHT, &ContactPoints[1],
						0.5f + sensorStep, 1.0f + sensorStep);

	for (int i = 0; i < 32; i++)
		SetSensor (i, 0);

	for (int i = 0; i < 2; i++)
		ApplyContactPoint (ContactPoints[i], i);

	// Update rumble
	if (rumbleIntensity == 0)
		return;

	bool isSliderTouched = false;
	for (int i = 0; i < 2; i++) {
		if (ContactPoints[i].InContact)
			isSliderTouched = true;
	}
	if (!isSliderTouched) {
		EndRumble ();
		return;
	}
	for (int i = 0; i < 64; ++i) {
		if (targetStates[i].tgtRemainingTime < 0.13f
			&& targetStates[i].tgtRemainingTime > -0.13f
			&& targetStates[i].tgtRemainingTime != 0
			&& (targetStates[i].tgtType == 15 || targetStates[i].tgtType == 16)
			&& targetStates[i].tgtHitState == 21) {
			for (int i = 0; i < COUNTOFARR (controllers); i++) {
				if (!controllers[i]
					|| !SDL_GameControllerHasRumble (controllers[i]))
					continue;

				SDL_GameControllerRumble (controllers[i], rumbleIntensity * 2,
										  rumbleIntensity, 1000);
			}
			return;
		}
	}
	EndRumble ();
}

inline void
SetInputStateBit (uint8_t *data, uint8_t bit, bool isPressed) {
	data[bit / 8] = isPressed ? data[bit / 8] | 1 << (bit % 8)
							  : data[bit / 8] & ~(1 << (bit % 8));
}

void
UpdateDwGuiInput () {
	char keyState = 0;

	char caseDiff = 'A' - 'a';
	bool upperCase = KeyboardIsDown (VK_SHIFT);

	for (char key = '0'; key < 'Z'; key++)
		if (KeyboardIsTapped (key))
			keyState = upperCase || key < 'A' ? key : key - caseDiff;

	if (KeyboardIsTapped (VK_BACK))
		keyState = VK_BACK;
	if (KeyboardIsTapped (VK_TAB))
		keyState = VK_TAB;
	if (KeyboardIsTapped (VK_SPACE))
		keyState = VK_SPACE;

	inputState->MouseX = (int)currentMouseState.RelativePosition.x;
	inputState->MouseY = (int)currentMouseState.RelativePosition.y;
	inputState->MouseDeltaX
		= currentMouseState.Position.x - lastMouseState.Position.x;
	inputState->MouseDeltaY
		= currentMouseState.Position.y - lastMouseState.Position.y;
	inputState->Key = keyState;

	for (int i = 0; i < COUNTOFARR (keyBits); i++) {
		SetInputStateBit ((uint8_t *)&inputState->Tapped, keyBits[i].bit,
						  KeyboardIsTapped (keyBits[i].keycode));
		SetInputStateBit ((uint8_t *)&inputState->Released, keyBits[i].bit,
						  KeyboardIsReleased (keyBits[i].keycode));
		SetInputStateBit ((uint8_t *)&inputState->Down, keyBits[i].bit,
						  KeyboardIsDown (keyBits[i].keycode));
		SetInputStateBit ((uint8_t *)&inputState->DoubleTapped, keyBits[i].bit,
						  KeyboardIsTapped (keyBits[i].keycode));
		SetInputStateBit ((uint8_t *)&inputState->IntervalTapped,
						  keyBits[i].bit,
						  KeyboardIsTapped (keyBits[i].keycode));
	}

	SetInputStateBit ((uint8_t *)&inputState->Tapped, 99, IsMouseScrollUp);
	SetInputStateBit ((uint8_t *)&inputState->Released, 99, IsMouseScrollUp);
	SetInputStateBit ((uint8_t *)&inputState->Down, 99, IsMouseScrollUp);
	SetInputStateBit ((uint8_t *)&inputState->DoubleTapped, 99,
					  IsMouseScrollUp);
	SetInputStateBit ((uint8_t *)&inputState->IntervalTapped, 99,
					  IsMouseScrollUp);

	SetInputStateBit ((uint8_t *)&inputState->Tapped, 100, IsMouseScrollDown);
	SetInputStateBit ((uint8_t *)&inputState->Released, 100,
					  IsMouseScrollDown);
	SetInputStateBit ((uint8_t *)&inputState->Down, 100, IsMouseScrollDown);
	SetInputStateBit ((uint8_t *)&inputState->DoubleTapped, 100,
					  IsMouseScrollDown);
	SetInputStateBit ((uint8_t *)&inputState->IntervalTapped, 100,
					  IsMouseScrollDown);

	if (dwGuiDisplay->on) {
		IsMouseScrollUp = false;
		IsMouseScrollDown = false;
	}
}

float verticalRotation;
float horizontalRotation;
FUNCTION_PTR (void, __stdcall, GlutSetCursor, 0x1408B68E6, int32_t cursorID);
void
UpdateUnlockedCamera (HWND DivaWindowHandle) {
	if (IsButtonTapped (CAMERA_UNLOCK_TOGGLE)) {
		unlockedCamera = !unlockedCamera;
		GlutSetCursor (unlockedCamera ? 0x65 : 0);
		if (unlockedCamera) {
			WRITE_MEMORY (0x1401F9460, uint8_t, 0xC3);
			WRITE_MEMORY (0x1401F93F0, uint8_t, 0xC3);
			WRITE_MEMORY (0x1401F9480, uint8_t, 0xC3);
			WRITE_MEMORY (0x1401F9430, uint8_t, 0xC3);

			verticalRotation
				= (float)(atan2 (camera->FocusZ - camera->PositionZ,
								 camera->FocusX - camera->PositionX)
						  * 180.0 / M_PI)
				  + 90.0f;
			horizontalRotation = 0.0f;
			camera->Rotation = 0.0f;
			camera->HorizontalFov = 70.0f;

			RECT window;
			GetClientRect (DivaWindowHandle, &window);
			SetCursorPos (window.right / 2, window.bottom / 2);
		} else {
			WRITE_MEMORY (0x1401F9460, uint8_t, 0x8B);
			WRITE_MEMORY (0x1401F93F0, uint8_t, 0x8B);
			WRITE_MEMORY (0x1401F9480, uint8_t, 0xF3);
			WRITE_MEMORY (0x1401F9430, uint8_t, 0x80);
		}
	}

	if (!unlockedCamera)
		return;

	float speed = IsButtonDown (CAMERA_MOVE_FAST)	? 0.1f
				  : IsButtonDown (CAMERA_MOVE_SLOW) ? 0.005f
													: 0.025f;

	bool forward = IsButtonDown (CAMERA_MOVE_FORWARD);
	bool left = IsButtonDown (CAMERA_MOVE_LEFT);
	bool up = IsButtonDown (CAMERA_MOVE_UP);
	bool cw = IsButtonDown (CAMERA_ROTATE_CW);
	bool zoomIn = IsButtonDown (CAMERA_ZOOM_IN);

	if (forward ^ IsButtonDown (CAMERA_MOVE_BACKWARD)) {
		camera->PositionX
			+= -1
			   * cos (((horizontalRotation - (forward ? 0.0f : 180.0f) + 90.0f)
					   * M_PI)
					  / 180.0f)
			   * speed;
		camera->PositionZ
			+= -1
			   * sin (((horizontalRotation - (forward ? 0.0f : 180.0f) + 90.0f)
					   * M_PI)
					  / 180.0f)
			   * speed;
	}
	if (left ^ IsButtonDown (CAMERA_MOVE_RIGHT)) {
		camera->PositionX
			+= -1
			   * cos (((horizontalRotation + (left ? -90.0f : 90.0f) + 90.0f)
					   * M_PI)
					  / 180.0f)
			   * speed;
		camera->PositionZ
			+= -1
			   * sin (((horizontalRotation + (left ? -90.0f : 90.0f) + 90.0f)
					   * M_PI)
					  / 180.0f)
			   * speed;
	}
	if (up ^ IsButtonDown (CAMERA_MOVE_DOWN))
		camera->PositionY += speed * (up ? 0.5f : -0.5f);
	if (cw ^ IsButtonDown (CAMERA_ROTATE_CCW))
		camera->Rotation += speed * (cw ? -10.0f : 10.0f);
	if (zoomIn ^ IsButtonDown (CAMERA_ZOOM_OUT)) {
		camera->HorizontalFov += speed * (zoomIn ? -5.0f : 5.0f);
		if (camera->HorizontalFov < 1.0f)
			camera->HorizontalFov = 1.0f;
		if (camera->HorizontalFov > 170.0f)
			camera->HorizontalFov = 170.0f;
	}

	int mouseDeltaX = currentMouseState.Position.x - lastMouseState.Position.x;
	int mouseDeltaY = currentMouseState.Position.y - lastMouseState.Position.y;

	if (mouseDeltaX != 0 || mouseDeltaY != 0) {
		RECT window;
		GetClientRect (DivaWindowHandle, &window);
		SetCursorPos (window.right / 2, window.bottom / 2);
		currentMouseState.Position.x = window.right / 2;
		currentMouseState.Position.y = window.bottom / 2;

		horizontalRotation += mouseDeltaX * 0.25f;
		verticalRotation -= mouseDeltaY * 0.05f;
	}

	struct Keybindings RSTICK_UP = { .axis = { SDL_AXIS_RIGHT_UP } };
	struct Keybindings RSTICK_DOWN = { .axis = { SDL_AXIS_RIGHT_DOWN } };
	struct Keybindings RSTICK_LEFT = { .axis = { SDL_AXIS_RIGHT_LEFT } };
	struct Keybindings RSTICK_RIGHT = { .axis = { SDL_AXIS_RIGHT_RIGHT } };
	bool rstickUp = IsButtonDown (RSTICK_UP);
	bool rstickDown = IsButtonDown (RSTICK_DOWN);
	bool rstickLeft = IsButtonDown (RSTICK_LEFT);
	bool rstickRight = IsButtonDown (RSTICK_RIGHT);

	if (rstickUp)
		verticalRotation += 0.25f;
	if (rstickDown)
		verticalRotation -= 0.25f;
	if (rstickLeft)
		horizontalRotation -= 1.25f;
	if (rstickRight)
		horizontalRotation += 1.25f;

	if (verticalRotation < -75.0f)
		verticalRotation = -75.0f;
	if (verticalRotation > 75.0f)
		verticalRotation = 75.0f;

	camera->FocusX
		= camera->PositionX
		  + (-1 * cos (((horizontalRotation + 90.0f) * M_PI) / 180.0f));
	camera->FocusY
		= camera->PositionY
		  + (-1 * cos (((verticalRotation + 90.0f) * M_PI) / 180.0f) * 5.0f);
	camera->FocusZ
		= camera->PositionZ
		  + (-1 * sin (((horizontalRotation + 90.0f) * M_PI) / 180.0f));
}
