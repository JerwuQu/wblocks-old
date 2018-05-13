#include "bar.h"

#include <stdlib.h>
#include <Windows.h>
#include <stdio.h>

int main()
{
	bar_init();

	// Message loop
	MSG message;
	while (GetMessage(&message, NULL, 0, 0)) {
		TranslateMessage(&message);
		DispatchMessage(&message);
	}

	bar_quit();

	system("pause");
	return 0;
}
