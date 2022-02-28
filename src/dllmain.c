#include "constants.h"
#include "helpers.h"
#include "io.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <windows.h>

void FastLoaderUpdate ();
void ApplyPatches ();
void ScaleUpdate (HWND DivaWindowHandle);
void Initialize ();

bool FirstUpdate = true;
HWND DivaWindowHandle = NULL;

HOOK (void, __stdcall, Update, 0x14018CC40)
{
	if (FirstUpdate)
		Initialize ();

	IOUpdate (DivaWindowHandle);
	ScaleUpdate (DivaWindowHandle);
	FastLoaderUpdate ();
}

void
Initialize ()
{
	FirstUpdate = false;
	DivaWindowHandle
		= FindWindow (0, "Hatsune Miku Project DIVA Arcade Future Tone");
	if (DivaWindowHandle == NULL)
		DivaWindowHandle = FindWindow (0, "GLUT");

	InitializeIO (DivaWindowHandle);

	/* Enable use_card */
	WRITE_MEMORY (0x1411A8850, uint8_t, 0x01);
	/* Allow selecting modules */
	WRITE_MEMORY (0x1405869AD, uint8_t, 0xB0, 0x01);
	WRITE_MEMORY (0x140583B45, uint8_t, 0x85);
	WRITE_MEMORY (0x140583C8C, uint8_t, 0x85);
	/* Unlock all modules and items */
	memset ((void *)0x1411A8990, 0xFF, 128);
	memset ((void *)0x1411A8B08, 0xFF, 128);
}

FUNCTION_PTR (void, __stdcall, UpdateTask, UPDATE_TASKS_ADDRESS);
void
FastLoaderUpdate ()
{
	if (*(uint32_t *)CURRENT_GAME_STATE_ADDRESS != 0)
		return;

	for (int i = 0; i < 3; i++)
		UpdateTask ();

	*(int *)DATA_INIT_STATE_ADDRESS = 3;
	*(int *)SYSTEM_WARNING_ELAPSED_ADDRESS = 3939;
}

void
ApplyPatches ()
{
	/* Just completely ignore all SYSTEM_STARTUP errors */
	WRITE_MEMORY (0x1403F5080, uint8_t, 0xC3);
	/* Always exit TASK_MODE_APP_ERROR on the first frame */
	WRITE_NOP (0x1403F73A7, 2);
	WRITE_MEMORY (0x1403F73C3, uint8_t, 0x89, 0xD1, 0x90);
	/* Clear framebuffer at all resolutions */
	WRITE_NOP (0x140501480, 2);
	WRITE_NOP (0x140501515, 2);
	/* TODO: Fix these two */
	/* Write ram files to ram/ instead of Y:/SBZV/ram/ */
	WRITE_MEMORY (0x14066CF09, uint8_t, 0xE9, 0xD8, 0x00);
	/* Change mdata path from "C:/Mount/Option" to "mdata/" */
	WRITE_MEMORY (0x140A8CA18, uint8_t, 'm', 'd', 'a', 't', 'a', '/', '\0');
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
	// WRITE_NOP (0x1405CB14A, 6);
	// WRITE_NOP (0x140136CFA, 6);
	/* Enable modselector without use_card */
	// WRITE_MEMORY (0x1405C513B, uint8_t, 0x01);
	/* Fix back button */
	WRITE_MEMORY (0x140578FB8, uint8_t, 0xE9, 0x73, 0xFF, 0xFF, 0xFF);
	/* Hide Keychip ID */
	WRITE_NULL (0x1409A5918, 14);
	WRITE_NULL (0x1409A5928, 14);
	/* Fix TouchReaction */
	WRITE_MEMORY (0x1406A1F81, uint8_t, 0x0F, 0x59, 0xC1, 0x0F, 0x5E, 0xC2,
				  0x66, 0x0F, 0xD6, 0x44, 0x24, 0x10, 0xEB, 0x06, 0xEB, 0x67);
	WRITE_MEMORY (0x1406A1FE2, uint8_t, 0x7E);
	WRITE_MEMORY (0x1406A1FEF, uint8_t, 0x66, 0x0F, 0xD6, 0x44, 0x24, 0x6C,
				  0xC7, 0x44, 0x24, 0x74, 0x00, 0x00, 0x00, 0x00, 0xEB, 0x0E,
				  0x66, 0x48, 0x0F, 0x6E, 0xC2, 0xEB, 0x5D, 0x0F, 0x2A, 0x0D,
				  0xB8, 0x6A, 0x31, 0x00, 0x0F, 0x12, 0x51, 0x1C, 0xE9, 0x14,
				  0xFF, 0xFF, 0xFF);
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
}

void
ScaleUpdate (HWND DivaWindowHandle)
{
	RECT hWindow;
	if (GetClientRect (DivaWindowHandle, &hWindow) == 0)
		return;

	*(float *)UI_ASPECT_RATIO = (float)hWindow.right / (float)hWindow.bottom;
	*(double *)FB_ASPECT_RATIO
		= (double)hWindow.right / (double)hWindow.bottom;
	*(float *)UI_WIDTH_ADDRESS = hWindow.right;
	*(float *)UI_HEIGHT_ADDRESS = hWindow.bottom;
	*(int *)FB1_WIDTH_ADDRESS = hWindow.right;
	*(int *)FB1_HEIGHT_ADDRESS = hWindow.bottom;

	*(int *)0x00000001411AD608 = 0;

	*(int *)0x0000000140EDA8E4 = *(int *)0x0000000140EDA8BC;
	*(int *)0x0000000140EDA8E8 = *(int *)0x0000000140EDA8C0;

	*(float *)0x00000001411A1900 = 0;
	*(float *)0x00000001411A1904 = *(int *)0x0000000140EDA8BC;
	*(float *)0x00000001411A1908 = *(int *)0x0000000140EDA8C0;
}

BOOL WINAPI
DllMain (HMODULE mod, DWORD cause, void *ctx)
{
	if (cause == DLL_PROCESS_DETACH)
		DiposeIO ();
	if (cause != DLL_PROCESS_ATTACH)
		return 1;

	ApplyPatches ();
	INSTALL_HOOK (Update);

	return 1;
}
