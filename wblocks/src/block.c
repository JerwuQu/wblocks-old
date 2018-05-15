#include "block.h"
#include "bar.h"
#include "../../shared.h"

#define BLOCK_EVENT_SETTEXT 1
#define BLOCK_LUA_TBLOCK_GLOBAL "__t_block__"

static int blockCount = 0;
static struct block_Block** blocks = NULL; 

static int lSetText(lua_State* L)
{
	if (!lua_isstring(L, 1)) return luaL_argerror(L, 1, "not a string");

	struct block_Event* event = malloc(sizeof(struct block_Event));
	lua_getglobal(L, BLOCK_LUA_TBLOCK_GLOBAL);
	struct block_t_Block* tBlock = lua_touserdata(L, -1);
	event->blockId = tBlock->blockId;
	event->type = BLOCK_EVENT_SETTEXT;
	event->wstr = strWiden(lua_tostring(L, 1), (int)lua_strlen(L, 1), &event->wstrlen);

	PostThreadMessage(WBLOCKS_MESSAGE_THREAD_ID, WBLOCKS_WM_BLOCK_EVENT, 0, (LPARAM)event);

	return 0;
}

static DWORD WINAPI threadProc(LPVOID lpParameter)
{
	struct block_t_Block* tBlock = (struct block_t_Block*)lpParameter;

	// Create state
	lua_State* L = tBlock->L = luaL_newstate();
	luaL_openlibs(tBlock->L);
	luaJIT_setmode(L, 0, LUAJIT_MODE_ENGINE | LUAJIT_MODE_ON);

	// Store tBlock in lua state
	lua_pushlightuserdata(L, tBlock);
	lua_setglobal(L, BLOCK_LUA_TBLOCK_GLOBAL);

	// Add functions
	lua_pushcfunction(L, lSetText);
	lua_setglobal(L, "SetText");

	// Load script
	lua_getglobal(L, "dofile");
	lua_pushstring(L, tBlock->scriptPath);
	if (lua_pcall(L, 1, 0, 0)) {
		printf("Lua error: %s\n", lua_tostring(L, -1));
		return 1;
	}

	return 0;
}

struct block_Block* block_addBlock(char* scriptPath)
{
	printf("Loading: %s\n", scriptPath);

	// Create bar block
	struct block_Block* bBlock = malloc(sizeof(struct block_Block));
	memset(bBlock, 0, sizeof(struct block_Block));

	// Create thread block
	struct block_t_Block* tBlock = malloc(sizeof(struct block_t_Block));
	bBlock->tBlock = tBlock;
	bBlock->blockId = tBlock->blockId = blockCount;
	tBlock->scriptPath = _strdup(scriptPath);
	if (!CreateThread(NULL, 0, threadProc, tBlock, 0, &bBlock->threadId)) {
		log_text("Failed to create script thread!\n");
		log_win32_error();
		free(bBlock);
		free(tBlock);
		free(tBlock->scriptPath);
		return 0;
	}

	// Add to list
	blocks = realloc(blocks, (blockCount + 1) * sizeof(struct block_Block));
	blocks[blockCount++] = bBlock;
	return bBlock;
}

void block_eventHandler(struct block_Event* event)
{
	if (event->blockId >= blockCount) return; // Block doesn't exist anymore
	struct block_Block* bBlock = blocks[event->blockId];

	if (event->type == BLOCK_EVENT_SETTEXT) {
		free(bBlock->text.wstr);
		bBlock->text.wstr = event->wstr;
		bBlock->text.wlen = event->wstrlen;
		bar_redraw();
	}

	free(event);
}

int block_getBlockCount()
{
	return blockCount;
}

struct block_Block** block_getBlocks()
{
	return blocks;
}
