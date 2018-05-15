#include "bar.h"
#include "wblocks.h"
#include "block.h"

#include <stdlib.h>
#include <Windows.h>
#include <stdio.h>

int main()
{
	// Get thread id
	WBLOCKS_MESSAGE_THREAD_ID = GetCurrentThreadId();

	// Init
	bar_init();

	// Load blocks
	block_addBlock("test.lua");

	// Message loop
	MSG message;
	while (GetMessage(&message, NULL, 0, 0)) {
		TranslateMessage(&message);
		DispatchMessage(&message);

		if (message.hwnd == NULL && message.message == WBLOCKS_WM_BLOCK_EVENT) {
			struct block_Event* event = (struct block_Event*)message.lParam;
			block_eventHandler(event);
		}
	}

	bar_quit();

	system("pause");
	return 0;
}
