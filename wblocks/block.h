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
};

// _NOT_ to be accessed from script thread
struct block_Block
{
    int blockId;
    char blockIsScripted;
    DWORD scriptThreadId;
    int bar_xpos, bar_width;

    struct wtext text;
    COLORREF color;
};

enum block_EventType
{
    BLOCK_EVENT_NONE,       // none
    BLOCK_EVENT_SETTEXT,    // to bar
    BLOCK_EVENT_SETCOLOR,   // to bar
    BLOCK_EVENT_MOUSE_DOWN  // to script
};

// todo: use separate struct for script events
struct block_Event
{
    int blockId;
    enum block_EventType type;

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

struct block_Block* block_addScriptBlock(char* scriptPath);
struct block_Block* block_addStaticBlock(char* str, int len);
void block_barEventHandler(struct block_Event* event);
int block_getBlockCount();
struct block_Block** block_getBlocks();
