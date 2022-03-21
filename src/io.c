#include "io.h"
#include "helpers.h"
#include "poll.h"
#include <math.h>
#include <stdio.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

bool unlockedCamera = false;
bool HasWindowFocus = false;
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

void DrawPauseMenu ();
void DrawTestMenu ();

void UpdateTouch ();
void UpdateInputUnfocused ();
void UpdateInput ();
void UpdateDwGuiInput ();
void UpdateUnlockedCamera (HWND DivaWindowHandle);
void SetSensor (int index, int value);

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

struct Keybindings SEL_UP = { .scroll = { MOUSE_SCROLL_UP } };
struct Keybindings SEL_DOWN = { .scroll = { MOUSE_SCROLL_DOWN } };

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

struct Keybindings WIREFRAME = { .keycodes = { VK_F12 } };

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

void
InitializeIO () {
	sliderState = (struct TouchSliderState *)0x14CC5DE40;
	inputState = (struct InputState *)(*(uint64_t *)(void *)0x140EDA330);
	currentTouchPanelState = (struct TouchPanelState *)0x14CC9EC30;
	targetStates = (struct TargetState *)0x140D0B688;
	dwGuiDisplay = (struct DwGuiDisplay *)*(uint64_t *)0x141190108;
	camera = (struct Camera *)0x140FBC2C0;

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

	SetConfigValue (config, "SEL_UP", &SEL_UP);
	SetConfigValue (config, "SEL_DOWN", &SEL_DOWN);

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

	SetConfigValue (config, "WIREFRAME", &WIREFRAME);

	toml_free (config);
	config = openConfig (configPath ("config.toml"));
	if (!config)
		return;

	rumbleIntensity
		= 65535 * readConfigInt (config, "rumbleIntensity", 25) / 100;

	toml_free (config);
}

void
UpdateIO (HWND DivaWindowHandle) {
	bool HadWindowFocus = HasWindowFocus;
	HasWindowFocus = DivaWindowHandle == NULL
					 || GetForegroundWindow () == DivaWindowHandle;

	currentTouchPanelState->ConnectionState = 1;
	sliderState->State = 3;

	if (HasWindowFocus) {
		UpdateUnlockedCamera (DivaWindowHandle);
		if (unlockedCamera)
			return;

		UpdateInput ();
		UpdateTouch ();
		UpdateDwGuiInput ();

		if (dwGuiDisplay->on)
			return;

		/* Wireframe */
		if (IsButtonTapped (WIREFRAME)) {
			if (*(uint8_t *)0x140500BE6 == 0xB9) {
				WRITE_MEMORY (0x140500BE6, uint8_t, 0xBA, 0x01, 0x1B, 0x00,
							  0x00, 0xB9, 0x08, 0x04, 0x00, 0x00, 0xE8, 0xE1,
							  0x5A, 0x3B, 0x00, 0x90, 0x90, 0x90, 0x90, 0x90,
							  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90);
			} else {
				WRITE_MEMORY (0x140500BE6, uint8_t, 0xB9, 0x1C, 0x00, 0x00,
							  0x00, 0xE8, 0x30, 0x7C, 0xF3, 0xFF, 0xB9, 0x1B,
							  0x00, 0x00, 0x00, 0x8B, 0xD8, 0xE8, 0x24, 0x7C,
							  0xF3, 0xFF, 0xB9, 0x1A, 0x00, 0x00, 0x00);
			}
		}

		/* Scrolling through menus */
		int *slotsToScroll = (int *)0x14CC12470;
		int *modulesToScroll = (int *)0x1418047EC;
		int pvSlotsConst = *(int *)0x14CC119C8;
		int moduleIsReccomended = *(int *)0x1418047E0;

		if (IsButtonDown (SEL_UP)) {
			if (pvSlotsConst < 26)
				*slotsToScroll -= 1;
			if (moduleIsReccomended == 0)
				*modulesToScroll -= 1;
		}
		if (IsButtonDown (SEL_DOWN)) {
			if (pvSlotsConst < 26)
				*slotsToScroll += 1;
			if (moduleIsReccomended == 0)
				*modulesToScroll += 1;
		}
	} else if (HadWindowFocus) {
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

		SetRumble (0, 0);
	}
}

void
UpdateTouch () {
	if (dwGuiDisplay->active || dwGuiDisplay->on)
		return;

	currentTouchPanelState->XPosition = (float)GetMouseRelativePosition ().x;
	currentTouchPanelState->YPosition = (float)GetMouseRelativePosition ().y;

	currentTouchPanelState->ContactType = KeyboardIsDown (VK_LBUTTON) ? 2
										  : KeyboardIsReleased (VK_LBUTTON)
											  ? 1
											  : 0;

	currentTouchPanelState->Pressure
		= (float)(currentTouchPanelState->ContactType != 0);
}

float sliderIncrement = 16.6f / 750.0f;
float sensorStep = (1.0f / 32);

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

FUNCTION_PTR (void, __stdcall, ChangeGameState, 0x1401953D0,
			  uint32_t gameState);
void
UpdateInput () {
	if (IsButtonDown (ADVERTISE))
		ChangeGameState (1);
	if (IsButtonDown (GAME))
		ChangeGameState (2);
	if (IsButtonDown (DATA_TEST))
		ChangeGameState (3);
	if (IsButtonDown (TEST_MODE))
		ChangeGameState (4);
	if (IsButtonDown (APP_ERROR))
		ChangeGameState (5);

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
		SetRumble (0, 0);
		return;
	}
	for (int i = 0; i < 64; ++i) {
		if (targetStates[i].tgtRemainingTime < 0.13f
			&& targetStates[i].tgtRemainingTime > -0.13f
			&& targetStates[i].tgtRemainingTime != 0
			&& (targetStates[i].tgtType == 15 || targetStates[i].tgtType == 16)
			&& targetStates[i].tgtHitState == 21) {
			SetRumble (rumbleIntensity * 2, rumbleIntensity);
			return;
		}
	}
	SetRumble (0, 0);
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

	inputState->MouseX = (int)GetMouseRelativePosition ().x;
	inputState->MouseY = (int)GetMouseRelativePosition ().y;
	inputState->MouseDeltaX
		= GetMousePosition ().x - GetLastMousePosition ().x;
	inputState->MouseDeltaY
		= GetMousePosition ().y - GetLastMousePosition ().y;
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

	SetInputStateBit ((uint8_t *)&inputState->Tapped, 99, GetMouseScrollUp ());
	SetInputStateBit ((uint8_t *)&inputState->Released, 99,
					  GetMouseScrollUp ());
	SetInputStateBit ((uint8_t *)&inputState->Down, 99, GetMouseScrollUp ());
	SetInputStateBit ((uint8_t *)&inputState->DoubleTapped, 99,
					  GetMouseScrollUp ());
	SetInputStateBit ((uint8_t *)&inputState->IntervalTapped, 99,
					  GetMouseScrollUp ());

	SetInputStateBit ((uint8_t *)&inputState->Tapped, 100,
					  GetMouseScrollDown ());
	SetInputStateBit ((uint8_t *)&inputState->Released, 100,
					  GetMouseScrollDown ());
	SetInputStateBit ((uint8_t *)&inputState->Down, 100,
					  GetMouseScrollDown ());
	SetInputStateBit ((uint8_t *)&inputState->DoubleTapped, 100,
					  GetMouseScrollDown ());
	SetInputStateBit ((uint8_t *)&inputState->IntervalTapped, 100,
					  GetMouseScrollDown ());
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

	int mouseDeltaX = GetMousePosition ().x - GetLastMousePosition ().x;
	int mouseDeltaY = GetMousePosition ().y - GetLastMousePosition ().y;

	if (mouseDeltaX != 0 || mouseDeltaY != 0) {
		RECT window;
		POINT updatedPosition;
		GetClientRect (DivaWindowHandle, &window);
		SetCursorPos (window.right / 2, window.bottom / 2);
		updatedPosition.x = window.right / 2;
		updatedPosition.y = window.bottom / 2;
		SetMousePosition (updatedPosition);

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
