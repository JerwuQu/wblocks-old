#pragma once

#include "wblocks.h"

#include <stdint.h>
#include <luajit.h>
#include <lualib.h>
#include <lauxlib.h>

struct wtext
{
    wchar_t* wstr;
    int wlen;
};

struct ltimer
{
    lua_Integer id;
    uint64_t time;
    int luaRef;
    struct ltimer* prev;
    struct ltimer* next;
};

// Only to be accessed from script thread after creation
struct block_BlockThreadData
{
    int blockId;
    char* scriptPath;
    lua_State* L;
    struct ltimer timerRoot;
    lua_Integer timerIdCounter;
};

enum block_BlockTextAlign
{
    BLOCK_ALIGN_LEFT,
    BLOCK_ALIGN_CENTER,
    BLOCK_ALIGN_RIGHT
};

struct block_BlockStyle
{
    struct wtext text;
    HFONT font;
    COLORREF color;
    enum block_BlockTextAlign textAlign;
    struct wtext minWidthStr;
};

// _NOT_ to be accessed from script thread
struct block_Block
{
    int blockId;
    struct block_BlockStyle* style;

    char blockIsScripted;
    DWORD scriptThreadId;

    int bar_xpos, bar_width;
};

enum block_ModifyEventType
{
    BLOCK_MEVENT_NONE,
    BLOCK_MEVENT_SETTEXT,
    BLOCK_MEVENT_SETCOLOR,
    BLOCK_MEVENT_MOUSE_DOWN
};

struct block_ModifyEvent
{
    int blockId;
    enum block_ModifyEventType type;

    union
    {
        wchar_t* wstr;
    };

    union
    {
        int wstrlen; // length in characters, not bytes
        COLORREF color;
    };
};

enum block_InteractEventType
{
    BLOCK_IEVENT_NONE,
    BLOCK_IEVENT_MOUSE_DOWN
};

struct block_InteractEvent
{
    enum block_InteractEventType type;
};

struct block_Block* block_addScriptBlock(char* scriptPath, struct block_BlockStyle* style);
struct block_Block* block_addStaticBlock(struct block_BlockStyle* style);
void block_barEventHandler(struct block_ModifyEvent* event);
int block_getBlockCount();
struct block_Block** block_getBlocks();
