#include "bar.h"
#include "wblocks.h"
#include "block.h"

#include <stdlib.h>
#include <Windows.h>
#include <stdio.h>

wchar_t* strWiden(const char* inStr, int inLen, int* outLen)
{
    int wlen = *outLen = MultiByteToWideChar(CP_UTF8, 0, inStr, inLen, NULL, 0);
    wchar_t* outStr = malloc((wlen + 1) * sizeof(wchar_t));
    MultiByteToWideChar(CP_UTF8, 0, inStr, inLen, outStr, wlen);
    outStr[wlen] = 0; // Null terminator
    return outStr;
}

static int loadBlocks()
{
    FILE* fp;
    fopen_s(&fp, "wblocks.cfg", "r");
    if (!fp) {
        printf("Failed to open 'wblocks.cfg'\n");
        return 1;
    }

    int i, c;
    char line[1024] = { 0 };
    while (1) {
        for (i = 0; i < sizeof(line); i++) {
            c = getc(fp);
            if (c == EOF || c == '\n') {
                if (i > 0) {
                    line[i] = 0;
                    if (line[0] == '>') {
                        block_addStaticBlock(&line[1], i - 1);
                    } else {
                        block_addScriptBlock(line);
                    }
                }

                if (c == EOF) {
                    fclose(fp);
                    return 0;
                } else {
                    break;
                }
            } else {
                line[i] = c;
            }
        }
    }
}

int main()
{
    // Get thread id
    WBLOCKS_MESSAGE_THREAD_ID = GetCurrentThreadId();

    // Init
    bar_init();

    // Load blocks
    if (loadBlocks()) {
        bar_quit();
        return 1;
    }

    // Message loop
    MSG message;
    while (GetMessage(&message, NULL, 0, 0)) {
        TranslateMessage(&message);
        DispatchMessage(&message);

        // Handle block events
        if (message.hwnd == NULL && message.message == WBLOCKS_WM_BLOCK_EVENT) {
            struct block_Event* event = (struct block_Event*)message.lParam;
            block_eventHandler(event);
        }
    }

    bar_quit();

    system("pause");
    return 0;
}
