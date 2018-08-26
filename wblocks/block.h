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
struct block_t_Block
{
    int blockId;
    char* scriptPath;
    lua_State* L;
    struct ltimer timer;
};

// Only to be accessed from bar/main thread
struct block_Block
{
    int blockId;
    DWORD threadId;
    struct block_t_Block* tBlock;

    struct wtext text;
    COLORREF color;
};

struct block_Event
{
    int blockId;
    int type;

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

struct block_Block* block_addScriptBlock(char* scriptBlock);
struct block_Block* block_addStaticBlock(char* str, int len);
void block_eventHandler(struct block_Event* event);
int block_getBlockCount();
struct block_Block** block_getBlocks();
