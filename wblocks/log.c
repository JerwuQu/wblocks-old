#include "log.h"

#include <stdio.h>
#include <Windows.h>

void log_text(const char* format, ...)
{
	va_list vargs;
	va_start(vargs, format);
	vprintf(format, vargs);
	va_end(vargs);
}

void log_win32_error()
{
	wchar_t buf[256];
	FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buf, 256, NULL);
	log_text("%ls\n", buf);
}
