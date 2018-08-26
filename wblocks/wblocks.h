#pragma once

#include "log.h"
#include "../shared/shared.h"

#include <Windows.h>

DWORD WBLOCKS_MESSAGE_THREAD_ID;

wchar_t* strWiden(const char* inStr, int inLen, int* outLen);
