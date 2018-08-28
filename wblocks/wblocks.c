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

// Reads a string from config table
// Returns string pointer on success, NULL on fail
static char* toml_table_string(toml_table_t* table, char* key)
{
    const char* raw = toml_raw_in(table, key);
    if (raw) {
        char* str;
        if (toml_rtos(raw, &str)) {
            printf("Failed to parse 'wblocks.toml': Invalid string '%s'\n", key);
            return NULL;
        }
        return str;
    } else {
        return NULL;
    }
}

static inline int toml_array_int(toml_array_t* colorarr, int idx, int64_t* out)
{
    const char* raw;
    if (!(raw = toml_raw_at(colorarr, idx))) {
        printf("Failed to parse 'wblocks.toml': Invalid array index %d\n", idx);
        return 1;
    }

    if (toml_rtoi(raw, out)) {
        printf("Failed to parse 'wblocks.toml': Invalid integer at array index %d\n", idx);
        return 1;
    }

    return 0;
}

// Reads a color array from config table
// Returns 0 on success, 1 on fail
static int toml_table_color(toml_table_t* table, char* key, COLORREF* out)
{
    toml_array_t* colorarr = toml_array_in(table, key);
    if (colorarr) {
        int64_t r, g, b;
        if (toml_array_int(colorarr, 0, &r)) return 1;
        if (toml_array_int(colorarr, 1, &g)) return 1;
        if (toml_array_int(colorarr, 2, &b)) return 1;
        *out = RGB(r & 0xFF, g & 0xFF, b & 0xFF);
        return 0;
    } else {
        return 1;
    }
}

static int loadBlock(toml_table_t* block)
{
    // Create style
    struct block_BlockStyle style = { 0 };
    style.color = 0xffffff;

    // Get style info from config
    char* str;
    if (str = toml_table_string(block, "text")) {
        style.text.wstr = strWiden(str, (int)strlen(str), &style.text.wlen);
        free(str);
    }

    if (str = toml_table_string(block, "min_width")) {
        style.minWidthStr.wstr = strWiden(str, (int)strlen(str), &style.minWidthStr.wlen);
        free(str);
    }

    toml_table_color(block, "color", &style.color);

    // Finally, create block using text source
    if (str = toml_table_string(block, "script")) {
        block_addScriptBlock(str, &style);
        free(str);
    } else if (style.text.wlen) {
        block_addStaticBlock(&style);
    } else {
        printf("Failed to parse 'wblocks.toml': No block text source\n");
        return 1;
    }

    return 0;
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
    toml_table_t* block;
    for (int i = 0; (block = toml_table_at(blocks, i)); i++) {
        if (loadBlock(block)) {
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
