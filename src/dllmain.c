#include "helpers.h"
#include "io.h"
#include "poll.h"
#include <math.h>
#include <windows.h>

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

void UpdateFastLoader ();
void UpdateScale (HWND DivaWindowHandle);
void ApplyPatches ();

bool FirstUpdate = true;
HWND DivaWindowHandle = NULL;

int fps = 0;
float fspeed_error = 0.0;
float fspeed_error_next = 0.0;
float fspeed_last_result = 0;

bool isPaused = false;
bool giveUp = false;

struct Keybindings PAUSE_ENABLE
	= { .keycodes = { VK_RETURN },
		.buttons = { SDL_CONTROLLER_BUTTON_START } };
struct Keybindings PAUSE_UP = { .keycodes = { VK_UP },
								.buttons = { SDL_CONTROLLER_BUTTON_DPAD_UP },
								.axis = { SDL_AXIS_LEFT_UP } };
struct Keybindings PAUSE_DOWN
	= { .keycodes = { VK_DOWN },
		.buttons = { SDL_CONTROLLER_BUTTON_DPAD_DOWN },
		.axis = { SDL_AXIS_LEFT_DOWN } };
struct Keybindings PAUSE_SELECT
	= { .keycodes = { VK_RETURN, 'D', 'L' },
		.buttons = { SDL_CONTROLLER_BUTTON_START, SDL_CONTROLLER_BUTTON_B,
					 SDL_CONTROLLER_BUTTON_DPAD_RIGHT } };
struct Keybindings PAUSE_HIDE
	= { .keycodes = { 'A', 'J' },
		.buttons
		= { SDL_CONTROLLER_BUTTON_X, SDL_CONTROLLER_BUTTON_DPAD_LEFT } };

void
Initialize () {
	FirstUpdate = false;
	DivaWindowHandle
		= FindWindow (0, "Hatsune Miku Project DIVA Arcade Future Tone");
	if (DivaWindowHandle == NULL)
		DivaWindowHandle = FindWindow (0, "GLUT");

	InitializeIO ();
	InitializePoll (DivaWindowHandle);

	/* Enable use_card */
	WRITE_MEMORY (0x1411A8850, uint8_t, 0x01);
	/* Enable modifiers */
	WRITE_MEMORY (0x1411A9685, uint8_t, 0x01);
	/* Allow selecting modules and items */
	WRITE_MEMORY (0x1405869AD, uint8_t, 0xB0, 0x01);
	WRITE_MEMORY (0x140583B45, uint8_t, 0x85);
	WRITE_MEMORY (0x140583C8C, uint8_t, 0x85);
	/* Unlock all modules and items */
	memset ((void *)0x1411A8990, 0xFF, 128);
	memset ((void *)0x1411A8B08, 0xFF, 128);

	if (fps == 0)
		return;
	/* Fix high FPS */
	VirtualProtect ((void *)0x1409A0A58, sizeof (float),
					PAGE_EXECUTE_READWRITE, 0);
	WRITE_NOP (0x140192D7B, 3);
	WRITE_MEMORY (0x140338F2F, uint8_t, 0xF3, 0x0F, 0x10, 0x0D, 0x61, 0x18,
				  0xBA, 0x00, 0x48, 0x8B, 0x8F, 0x80, 0x01, 0x00, 0x00);
	WRITE_MEMORY (0x140338EBE, uint8_t, 0xF3, 0x0F, 0x10, 0x0D, 0xD2, 0x18,
				  0xBA, 0x00, 0x48, 0x8B, 0x05, 0xFB, 0xB1, 0xE5, 0x00, 0x48,
				  0x8B, 0x88, 0x80, 0x01, 0x00, 0x00);
	WRITE_MEMORY (0x140170394, uint8_t, 0xF3, 0x0F, 0x5E, 0x05, 0x34, 0xA3,
				  0xD6, 0x00);
	WRITE_MEMORY (0x140192D30, uint8_t, 0xF3, 0x0F, 0x10, 0x05, 0x5C, 0x02,
				  0x00, 0x00);
	WRITE_MEMORY (0x14053C901, uint8_t, 0xF3, 0x0F, 0x5E, 0x05, 0xC7, 0xDD,
				  0x99, 0x00, 0xE9, 0x68, 0x01, 0x00, 0x00);
	WRITE_MEMORY (0x14053CA71, uint8_t, 0xEB, 0x3F);
	WRITE_MEMORY (0x14053CAb2, uint8_t, 0xF3, 0x0F, 0x10, 0x05, 0xFA, 0x53,
				  0x46, 0x00, 0xE9, 0x42, 0xFE, 0xFF, 0xFF);
	/* This can be removed if you dont care about ageage module hair */
	WRITE_MEMORY (0x14054352F, uint8_t, 0x49, 0xB9);
	WRITE_MEMORY (0x140543531, uint64_t, (uint64_t)&fspeed_last_result);
	WRITE_MEMORY (0x140543539, uint8_t, 0xF3, 0x41, 0x0F, 0x59, 0x19, 0xEB,
				  0xB0);
}

HOOK (void, __cdecl, Update, 0x14018CC40) {
	if (FirstUpdate)
		Initialize ();

	UpdatePoll (DivaWindowHandle);

	/* Exit */
	if (KeyboardIsDown (VK_ESCAPE))
		*(bool *)0x140EDA6B0 = true;

	if (*(uint32_t *)0x140EDA82C == 13 && !isPaused)
		isPaused = IsButtonTapped (PAUSE_ENABLE);

	if (!isPaused)
		UpdateIO (DivaWindowHandle);
	UpdateScale (DivaWindowHandle);
	UpdateFastLoader ();

	if (fps == 0)
		return;
	/* High FPS */
	if (*(uint32_t *)0x140EDA810 == 2)
		*(float *)0x140EDA7CC = 60.0 * (fps / 60.0);
	else
		*(float *)0x140EDA7CC = 60.0;

	*(bool *)0x140EDA79C = true;
	if (*(uint32_t *)0x140EDA82C == 13 || *(uint32_t *)0x140EDA82C == 6)
		*(float *)0x0140EDA6CC = *(float *)0x140EDA7CC;
	else
		*(float *)0x0140EDA6CC = 60.0;
}

float expectedFrameDuration = 1000 / 60;
uint64_t nextUpdate;
HOOK (int64_t, __fastcall, EngineUpdate, 0x140194CD0, int64_t a1) {
	uint64_t curTime = GetTickCount64 ();

	if (curTime < nextUpdate)
		return 0;

	nextUpdate += expectedFrameDuration;

	if (nextUpdate < curTime)
		nextUpdate = curTime;

	return originalEngineUpdate (a1);
}

FUNCTION_PTR (void, __stdcall, DivaDrawText, 0x140198500,
			  struct DrawParams *drawParam, uint32_t flags, const char *text,
			  int64_t len);
FUNCTION_PTR (struct FontInfo *, __thiscall, GetFontInfoFromID, 0x140196510,
			  struct FontInfo *fontInfo, uint32_t id);
FUNCTION_PTR (void, __stdcall, FillRectangle, 0x140198D80,
			  struct DrawParams *drawParam, const struct Rectangle *rect);

void
DrawFullscreenMenu (int index, const char **items, int items_len) {
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
	drawParam.colour = 0xFFFFFFF;
	for (int i = 0; i < items_len; i++) {
		char buf[32];
		sprintf (buf, "%s\n", items[i]);
		drawParam.colour = (i == index ? 0xFF00FFFF : 0xFFFFFFFF);
		DivaDrawText (&drawParam, 0x5, buf, 32);
	}
}

HOOK (bool, __stdcall, GiveUp, 0x14010EF00, void *cls) {
	if (giveUp) {
		giveUp = false;
		return true;
	}
	return originalGiveUp (cls);
}

const char *PauseMenuItems[] = { "RESUME", "RESTART", "GIVE UP" };
bool firstPauseFrame = true;
bool hidePauseMenu = false;
int pauseIndex = 0;
uint8_t aetMoveOriginal[8];
uint8_t framespeedOriginal[4];
uint8_t ageageHairOriginal[3];
bool playstates[8];

FUNCTION_PTR (void, __stdcall, PauseDSC, 0x1401295C0);
FUNCTION_PTR (void, __stdcall, UnPauseDSC, 0x140129590);
FUNCTION_PTR (void, __stdcall, LoadPV, 0x1400FDDC0, uint64_t cls);

void
DrawPauseMenu () {
	if (*(uint32_t *)0x140EDA82C != 13)
		isPaused = false;
	if (IsButtonTapped (PAUSE_HIDE) && !firstPauseFrame)
		hidePauseMenu = !hidePauseMenu;
	if (!isPaused || hidePauseMenu)
		return;

	if (IsButtonTapped (PAUSE_UP))
		pauseIndex--;
	if (IsButtonTapped (PAUSE_DOWN))
		pauseIndex++;

	if (pauseIndex > 2)
		pauseIndex = 0;
	if (pauseIndex < 0)
		pauseIndex = 2;

	if (firstPauseFrame) {
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
	} else if (IsButtonTapped (PAUSE_SELECT)) {
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

	DrawFullscreenMenu (pauseIndex, PauseMenuItems, 3);
}

const char *DataTestNames[] = {
	"MISC TEST",
	"OBJECT TEST",
	"STAGE TEST",
	"MOTION TEST",
	"COLLISION TEST",
	"SPRITE TEST",
	"2DAUTH TEST",
	"3DAUTH TEST",
	"CHARA TEST",
	"ITEM TEST",
	"PERFORMANCE TEST",
	"PVSCRIPT TEST",
	"PRINT TEST",
	"CARD TEST",
	"OPD TEST",
	"SLIDER TEST",
	"GLITTER TEST",
	"GRAPHICS TEST",
	"COLLECTION CARD TEST",
};
int dataTestIndex = 21;
FUNCTION_PTR (void, __stdcall, ChangeSubState, 0x140195260, uint32_t gameState,
			  uint32_t subState);

void
DrawTestMenu () {
	if (*(uint32_t *)0x140EDA810 != 3 || *(uint32_t *)0x140EDA82C != 19)
		return;

	if (IsButtonTapped (PAUSE_UP))
		dataTestIndex--;
	if (IsButtonTapped (PAUSE_DOWN))
		dataTestIndex++;

	if (dataTestIndex > 39)
		dataTestIndex = 21;
	if (dataTestIndex < 21)
		dataTestIndex = 39;

	if (IsButtonTapped (PAUSE_SELECT)) {
		ChangeSubState (3, dataTestIndex - 1);
	}

	DrawFullscreenMenu (dataTestIndex - 21, DataTestNames, 19);
}

HOOK (void, __cdecl, Update2D, 0x0140501F70, void *a1) {
	if (fps > 0) {
		fspeed_error = fspeed_error_next;
		fspeed_error_next = 0;
	}
	DrawTestMenu ();
	DrawPauseMenu ();
	originalUpdate2D (a1);
}

HOOK (float, __stdcall, GetFrameSpeed, 0x140192D50) {
	float frameSpeed = originalGetFrameSpeed ();

	frameSpeed += fspeed_error;
	float speed_rounded;
	float speed_remainder = modff (frameSpeed, &speed_rounded);

	if (fspeed_error_next == 0) {
		fspeed_error_next = speed_remainder;
		fspeed_last_result = speed_rounded;
	}

	return speed_rounded;
}

int32_t internalResX;
int32_t internalResY;
bool fullscreen = false;

HOOK (int64_t, __fastcall, ParseParameters, 0x140193630, int32_t a1,
	  int64_t *a2) {
	if (internalResX != 0 && internalResY != 0)
		*(uint32_t *)0x140EDA5D4 = 15;

	*(bool *)0x140EDA5D1 = fullscreen;

	return originalParseParameters (a1, a2);
}

FUNCTION_PTR (void, __stdcall, UpdateTask, 0x14019B980);
void
UpdateFastLoader () {
	if (*(uint32_t *)0x140EDA810 != 0)
		return;

	for (int i = 0; i < 3; i++)
		UpdateTask ();

	*(int32_t *)0x140EDA7A8 = 3;
	*(int32_t *)0x1411A1498 = 3939;
}

void
UpdateScale (HWND DivaWindowHandle) {
	RECT hWindow;
	if (GetClientRect (DivaWindowHandle, &hWindow) == 0)
		return;

	*(float *)0x14CC621D0 = (float)hWindow.right / (float)hWindow.bottom;
	*(float *)0x14CC621E4 = hWindow.right;
	*(float *)0x14CC621E8 = hWindow.bottom;
	*(double *)0x140FBC2E8 = (double)hWindow.right / (double)hWindow.bottom;
	*(int32_t *)0x1411AD5F8 = hWindow.right;
	*(int32_t *)0x1411AD5FC = hWindow.bottom;

	*(int32_t *)0x1411AD608 = 0;
	*(int32_t *)0x140EDA8E4 = *(int32_t *)0x140EDA8BC;
	*(int32_t *)0x140EDA8E8 = *(int32_t *)0x140EDA8C0;
	*(float *)0x1411A1900 = 0;
	*(float *)0x1411A1904 = *(int32_t *)0x140EDA8BC;
	*(float *)0x1411A1908 = *(int32_t *)0x140EDA8C0;
}

void
ApplyPatches () {
	/* Just completely ignore all SYSTEM_STARTUP errors */
	WRITE_MEMORY (0x1403F5080, uint8_t, 0xC3);
	/* Always exit TASK_MODE_APP_ERROR on the first frame */
	WRITE_NOP (0x1403F73A7, 2);
	WRITE_MEMORY (0x1403F73C3, uint8_t, 0x89, 0xD1, 0x90);
	/* Clear framebuffer at all resolutions */
	WRITE_NOP (0x140501480, 2);
	WRITE_NOP (0x140501515, 2);
	/* Write ram files to ram/ instead of Y:/SBZV/ram/ */
	WRITE_MEMORY (0x14066CF09, uint8_t, 0xE9, 0xD8, 0x00);
	/* Change mdata path from "C:/Mount/Option" to "mdata/" */
	WRITE_MEMORY (0x140A8CA18, uint8_t, "mdata/");
	/* Length of string at 0x140A8CA18 */
	WRITE_MEMORY (0x14066CEAE, uint8_t, 0x06);
	/* Skip parts of the network check state */
	WRITE_MEMORY (0x1406717B1, uint8_t, 0xE9, 0x22, 0x03, 0x00);
	/* Set the initial DHCP WAIT timer value to 0 */
	WRITE_NULL (0x1406724E7, 2);
	/* Ignore SYSTEM_STARTUP Location Server checks */
	WRITE_NOP (0x1406732A2, 2);
	/* Toon Shader Fix by lybxlpsv */
	WRITE_NOP (0x14050214F, 2);
	WRITE_MEMORY (0x140641102, uint8_t, 0x01);
	/* Skip keychip checks */
	WRITE_MEMORY (0x140210820, uint8_t, 0xB8, 0x00, 0x00, 0x00, 0x00, 0xC3);
	WRITE_MEMORY (0x14066E820, uint8_t, 0xB8, 0x01, 0x00, 0x00, 0x00, 0xC3);
	/* Disable glutFitWindowSizeToDesktop */
	WRITE_NOP (0x140194E06, 5);
	/* Enable Modifiers */
	WRITE_NOP (0x1405CB1B3, 13);
	WRITE_NOP (0x1405CA0F5, 2);
	WRITE_NOP (0x1405CB14A, 6);
	WRITE_NOP (0x140136CFA, 6);
	/* Enable modselector without use_card */
	WRITE_MEMORY (0x1405C513B, uint8_t, 0x01);
	/* Fix back button */
	WRITE_MEMORY (0x140578FB8, uint8_t, 0xE9, 0x73, 0xFF, 0xFF, 0xFF);
	/* Hide Keychip ID */
	WRITE_NULL (0x1409A5918, 14);
	WRITE_NULL (0x1409A5928, 14);
	/* Fix TouchReaction */
	WRITE_MEMORY (0x1406A1F81, uint8_t, 0x0F, 0x59, 0xC1, 0x0F, 0x5E, 0xC2,
				  0x66, 0x0F, 0xD6, 0x44, 0x24, 0x10, 0xEB, 0x06, 0xCC, 0xEB,
				  0x67);
	WRITE_MEMORY (0x1406A1FE2, uint8_t, 0x7E);
	WRITE_MEMORY (0x1406A1FE9, uint8_t, 0x66, 0x0F, 0xD6, 0x44, 0x24, 0x6C,
				  0xC7, 0x44, 0x24, 0x74, 0x00, 0x00, 0x00, 0x00, 0xEB, 0x0E,
				  0x66, 0x48, 0x0F, 0x6E, 0xC2, 0xEB, 0x5D);
	WRITE_MEMORY (0x1406A205D, uint8_t, 0x0F, 0x2A, 0x0D, 0xB8, 0x6A, 0x31,
				  0x00, 0x0F, 0x12, 0x51, 0x1C, 0xE9, 0x14, 0xFF, 0xFF, 0xFF);
	/* Enable "FREE PLAY" mode */
	WRITE_MEMORY (0x140393610, uint8_t, 0xB0, 0x01, 0xC3, 0x90, 0x90, 0x90);
	WRITE_MEMORY (0x1403BABEA, uint8_t, 0x75);
	/* Show Cursor */
	WRITE_MEMORY (0x14019341B, uint8_t, 0x00);
	WRITE_MEMORY (0x1403012B5, uint8_t, 0xEB);
	/* Disable Motion Blur */
	WRITE_NULL (0x1411AB67C, 1);
	WRITE_NOP (0x1404AB11D, 3);
	WRITE_NULL (0x1401063CE, 1);
	WRITE_NULL (0x14048FBA9, 1);
	/* Pause Timer */
	WRITE_NOP (0x140566B9E, 3);
	WRITE_NOP (0x1405BDFBF, 1);
	WRITE_NOP (0x1405BDFC0, 3);
	/* Patch PSEUDO modules */
	WRITE_NULL (0x140A21770, 1);
	WRITE_NULL (0x140A21780, 1);
	WRITE_NULL (0x140A21790, 1);
	WRITE_NULL (0x140A217A0, 1);
	WRITE_NULL (0x140A217B0, 1);
	/* Skip slider update */
	WRITE_NOP (0x14061579B, 3);
	WRITE_MEMORY (0x14061579E, uint8_t, 0x8B, 0x42, 0xE0);
	WRITE_NOP (0x1406157A1, 3);
	/* Prevent DATA_TEST crash */
	WRITE_NULL (0x140284B01, 1);
	/* Enable dw_gui */
	WRITE_NULL (0x140192601, 1);
	WRITE_MEMORY (0x140302600, uint8_t, 0xB0, 0x01);
	WRITE_MEMORY (0x140302610, uint8_t, 0xB0, 0x01);
	WRITE_MEMORY (0x140192D00, uint8_t, 0xB8, 0x01, 0x00, 0x00, 0x00, 0xC3);
	/* Prevent resetting playerdata */
	WRITE_MEMORY (0x1404A7370, uint8_t, 0xC3);
	/* Enable Scaling */
	WRITE_MEMORY (0x1404ACD24, uint8_t, 0x44, 0x8B, 0x0D, 0xD1, 0x08, 0xD0,
				  0x00);
	WRITE_MEMORY (0x1404ACD2B, uint8_t, 0x44, 0x8B, 0x05, 0xC6, 0x08, 0xD0,
				  0x00);
	WRITE_NOP (0x1405030A0, 6);
	/* Prevent DATA_TEST from exiting */
	WRITE_NULL (0x140284B01, 1);
}

#define WRITE_MEMORY_CONFIG_STRING(address, table)                            \
	{                                                                         \
		toml_datum_t data = toml_string_in (patch, "data");                   \
		if (data.ok) {                                                        \
			WRITE_MEMORY_STRING (address, data.u.s, strlen (data.u.s) + 1);   \
			free (data.u.s);                                                  \
		}                                                                     \
	}

#define WRITE_MEMORY_CONFIG_INT(address, table, type)                         \
	WRITE_MEMORY (address, type,                                              \
				  readConfigInt (table, "data", READ_MEMORY (address, type)))

#define WRITE_MEMORY_CONFIG_INT_ARR(address, table, type)                     \
	{                                                                         \
		toml_array_t *dataArray = toml_array_in (table, "data");              \
		if (!dataArray) {                                                     \
			free (data_type.u.s);                                             \
			continue;                                                         \
		}                                                                     \
		for (int idx = 0;; idx++) {                                           \
			toml_datum_t dataEntry = toml_int_at (dataArray, idx);            \
			if (!dataEntry.ok)                                                \
				break;                                                        \
			WRITE_MEMORY (address + idx, type, (type)dataEntry.u.i);          \
		}                                                                     \
	}

BOOL WINAPI
DllMain (HMODULE mod, DWORD cause, void *ctx) {
	if (cause == DLL_PROCESS_DETACH)
		DisposePoll ();
	if (cause != DLL_PROCESS_ATTACH)
		return 1;

	ApplyPatches ();
	INSTALL_HOOK (Update);
	INSTALL_HOOK (EngineUpdate);
	INSTALL_HOOK (Update2D);
	INSTALL_HOOK (GiveUp);
	INSTALL_HOOK (ParseParameters);

	toml_table_t *config = openConfig (configPath ("keyconfig.toml"));
	if (!config)
		goto config;

	SetConfigValue (config, "PAUSE_ENABLE", &PAUSE_ENABLE);
	SetConfigValue (config, "PAUSE_UP", &PAUSE_UP);
	SetConfigValue (config, "PAUSE_DOWN", &PAUSE_DOWN);
	SetConfigValue (config, "PAUSE_SELECT", &PAUSE_SELECT);
	SetConfigValue (config, "PAUSE_HIDE", &PAUSE_HIDE);

	toml_free (config);

config:
	config = openConfig (configPath ("config.toml"));
	if (!config)
		goto patches;
	fps = readConfigInt (config, "fps", 0);
	if (fps > 0) {
		expectedFrameDuration = 1000 / fps;
		INSTALL_HOOK (GetFrameSpeed);
	}
	fullscreen = readConfigBool (config, "fullscreen", false);

	toml_table_t *internalResSection
		= openConfigSection (config, "internalRes");
	if (!internalResSection) {
		toml_free (config);
		goto patches;
	}

	internalResX = readConfigInt (internalResSection, "x", 0);
	internalResY = readConfigInt (internalResSection, "y", 0);
	if (internalResX != 0 && internalResY != 0) {
		if (internalResX == -1 || internalResY == -1) {
			internalResX = GetSystemMetrics (SM_CXSCREEN);
			internalResY = GetSystemMetrics (SM_CYSCREEN);
		}

		WRITE_MEMORY (0x1409B8B68, int32_t, internalResX, internalResY);
	}

	toml_free (config);

patches:;
	WIN32_FIND_DATAA fd;
	HANDLE file = FindFirstFileA (configPath ("patches\\*.toml"), &fd);
	if (file == 0)
		return 1;

	do {
		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			continue;

		char filepath[MAX_PATH];
		strcpy (filepath, configPath ("patches\\"));
		strcat (filepath, fd.cFileName);
		toml_table_t *patchesConfig = openConfig (filepath);
		if (!patchesConfig)
			return 1;

		if (!readConfigBool (patchesConfig, "enabled", false)) {
			toml_free (patchesConfig);
			continue;
		}

		toml_array_t *patchesArray = toml_array_in (patchesConfig, "patch");
		for (int i = 0;; i++) {
			toml_table_t *patch = toml_table_at (patchesArray, i);
			if (!patch)
				break;
			void *address = (void *)readConfigInt (patch, "address", 0);
			if (address < (void *)0x140000000)
				continue;
			toml_datum_t data_type = toml_string_in (patch, "data_type");
			if (!data_type.ok)
				continue;

			/* Switch dosent work for char* BTW
			 This was as boring for me to write as it will be for you
			 to read */

			if (strcmp (data_type.u.s, "string") == 0)
				WRITE_MEMORY_CONFIG_STRING (address, patch)
			else if (strcmp (data_type.u.s, "i8") == 0)
				WRITE_MEMORY_CONFIG_INT (address, patch, int8_t)
			else if (strcmp (data_type.u.s, "i16") == 0)
				WRITE_MEMORY_CONFIG_INT (address, patch, int16_t)
			else if (strcmp (data_type.u.s, "i32") == 0)
				WRITE_MEMORY_CONFIG_INT (address, patch, int32_t)
			else if (strcmp (data_type.u.s, "i64") == 0)
				WRITE_MEMORY_CONFIG_INT (address, patch, int64_t)
			else if (strcmp (data_type.u.s, "u8") == 0)
				WRITE_MEMORY_CONFIG_INT (address, patch, uint8_t)
			else if (strcmp (data_type.u.s, "u16") == 0)
				WRITE_MEMORY_CONFIG_INT (address, patch, uint16_t)
			else if (strcmp (data_type.u.s, "u32") == 0)
				WRITE_MEMORY_CONFIG_INT (address, patch, uint32_t)
			else if (strcmp (data_type.u.s, "i8_arr") == 0)
				WRITE_MEMORY_CONFIG_INT_ARR (address, patch, int8_t)
			else if (strcmp (data_type.u.s, "i16_arr") == 0)
				WRITE_MEMORY_CONFIG_INT_ARR (address, patch, int16_t)
			else if (strcmp (data_type.u.s, "i32_arr") == 0)
				WRITE_MEMORY_CONFIG_INT_ARR (address, patch, int32_t)
			else if (strcmp (data_type.u.s, "i64_arr") == 0)
				WRITE_MEMORY_CONFIG_INT_ARR (address, patch, int64_t)
			else if (strcmp (data_type.u.s, "u8_arr") == 0)
				WRITE_MEMORY_CONFIG_INT_ARR (address, patch, uint8_t)
			else if (strcmp (data_type.u.s, "u16_arr") == 0)
				WRITE_MEMORY_CONFIG_INT_ARR (address, patch, uint16_t)
			else if (strcmp (data_type.u.s, "u32_arr") == 0)
				WRITE_MEMORY_CONFIG_INT_ARR (address, patch, uint32_t)

			free (data_type.u.s);
		}

		toml_free (patchesConfig);
	} while (FindNextFileA (file, &fd));

	return 1;
}
