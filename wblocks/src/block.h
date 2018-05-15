#pragma once

#include "wblocks.h"

#include <luajit.h>
#include <lualib.h>
#include <lauxlib.h>

// Only to be accessed from script thread after creation
struct block_t_Block
{
	int blockId;
	char* scriptPath;
	lua_State* L;
};

// Only to be accessed from bar/main thread
struct block_Block
{
	int blockId;
	DWORD threadId;
	wchar_t* wstr;
	int wstrlen;
	struct block_t_Block* tBlock;
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
	};
};

void block_addBlock(char* scriptBlock);
void block_eventHandler(struct block_Event* event);
int block_getBlockCount();
struct block_Block** block_getBlocks();
