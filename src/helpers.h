#pragma once
#include <MinHook.h>

#define FUNCTION_PTR(returnType, callingConvention, function, location, ...)  \
	returnType (callingConvention *function) (__VA_ARGS__)                    \
		= (returnType (callingConvention *) (__VA_ARGS__)) (location)

#define PROC_ADDRESS(libraryName, procName)                                   \
	GetProcAddress (LoadLibrary (TEXT (libraryName)), procName)

#define HOOK(returnType, callingConvention, functionName, location, ...)      \
	typedef returnType callingConvention (*functionName) (__VA_ARGS__);       \
	functionName original##functionName = NULL;                               \
	void *where##functionName = (void *)location;                             \
	returnType callingConvention implOf##functionName (__VA_ARGS__)

#define INSTALL_HOOK(functionName)                                            \
	{                                                                         \
		MH_Initialize ();                                                     \
		MH_CreateHook ((void *)where##functionName,                           \
					   (void *)implOf##functionName,                          \
					   (void **)(&original##functionName));                   \
		MH_EnableHook ((void *)where##functionName);                          \
	}

#define WRITE_MEMORY(location, type, ...)                                     \
	{                                                                         \
		const type data[] = { __VA_ARGS__ };                                  \
		DWORD oldProtect;                                                     \
		VirtualProtect ((void *)(location), sizeof (data),                    \
						PAGE_EXECUTE_READWRITE, &oldProtect);                 \
		memcpy ((void *)(location), data, sizeof (data));                     \
		VirtualProtect ((void *)(location), sizeof (data), oldProtect,        \
						&oldProtect);                                         \
	}

#define WRITE_NOP(location, count)                                            \
	{                                                                         \
		DWORD oldProtect;                                                     \
		VirtualProtect ((void *)(location), (size_t)(count),                  \
						PAGE_EXECUTE_READWRITE, &oldProtect);                 \
		for (size_t i = 0; i < (size_t)(count); i++)                          \
			*((uint8_t *)(location) + i) = 0x90;                              \
		VirtualProtect ((void *)(location), (size_t)(count), oldProtect,      \
						&oldProtect);                                         \
	}

#define WRITE_NULL(location, count)                                           \
	{                                                                         \
		DWORD oldProtect;                                                     \
		VirtualProtect ((void *)(location), (size_t)(count),                  \
						PAGE_EXECUTE_READWRITE, &oldProtect);                 \
		for (size_t i = 0; i < (size_t)(count); i++)                          \
			*((uint8_t *)(location) + i) = 0x00;                              \
		VirtualProtect ((void *)(location), (size_t)(count), oldProtect,      \
						&oldProtect);                                         \
	}

#define COUNTOFARR(arr) sizeof (arr) / sizeof (arr[0])