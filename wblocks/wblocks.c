#include "bar.h"
#include "wblocks.h"
#include "block.h"
#include "../ext/tomlc99/toml.h"

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
    // Read file
    FILE* fp;
    fopen_s(&fp, "wblocks.toml", "r");
    if (!fp) {
        printf("Failed to open 'wblocks.toml'\n");
        return 1;
    }

    // Parse toml
    char tomlErr[1024];
    toml_table_t* toml = toml_parse_file(fp, tomlErr, sizeof(tomlErr));
    fclose(fp);
    if (!toml) {
        printf("Failed to parse 'wblocks.toml': %s\n", tomlErr);
        return 1;
    }

    toml_array_t* blocks = toml_array_in(toml, "block");
    if (!blocks) {
        printf("Failed to parse 'wblocks.toml': Invalid block array\n");
        toml_free(toml);
        return 1;
    }

    // Parse block array
    char* str;
    const char* raw;
    toml_table_t* block;
    for (int i = 0; (block = toml_table_at(blocks, i)); i++) {
        if (raw = toml_raw_in(block, "text")) {
            if (toml_rtos(raw, &str)) {
                printf("Failed to parse 'wblocks.toml': Invalid block text\n");
                toml_free(toml);
                return 1;
            }
            block_addStaticBlock(str);
            free(str);
        } else if (raw = toml_raw_in(block, "script")) {
            if (toml_rtos(raw, &str)) {
                printf("Failed to parse 'wblocks.toml': Invalid block script\n");
                toml_free(toml);
                return 1;
            }
            block_addScriptBlock(str);
            free(str);
        } else {
            printf("Failed to parse 'wblocks.toml': No block text source\n");
            toml_free(toml);
            return 1;
        }
    }
    toml_free(toml);

    return 0;
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
        if (message.hwnd == NULL && message.message == WBLOCKS_WM_BLOCK_MODIFY_EVENT) {
            struct block_ModifyEvent* event = (struct block_ModifyEvent*)message.lParam;
            block_barEventHandler(event);
        }
    }

    bar_quit();

    system("pause");
    return 0;
}
