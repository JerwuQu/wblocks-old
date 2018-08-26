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
    DWORD scriptThreadId;

    struct wtext text;
    COLORREF color;
};

enum block_EventType
{
    BLOCK_EVENT_NONE,
    BLOCK_EVENT_SETTEXT,
    BLOCK_EVENT_SETCOLOR
};

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
void block_eventHandler(struct block_Event* event);
int block_getBlockCount();
struct block_Block** block_getBlocks();
