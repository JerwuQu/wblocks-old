#include "block.h"
#include "bar.h"
#include "../../shared.h"

#define BLOCK_EVENT_SETTEXT 1
#define BLOCK_EVENT_SETCOLOR 2
#define BLOCK_LUA_TBLOCK_GLOBAL "_T_BLOCK_"
#define BLOCK_LUA_TIMER_INTERVAL 100

static int blockCount = 0;
static struct block_Block** blocks = NULL; 

static int lSetText(lua_State* L)
{
	if (!lua_isstring(L, 1)) return luaL_argerror(L, 1, "not a string");

	// Get tBlock
	lua_getglobal(L, BLOCK_LUA_TBLOCK_GLOBAL);
	struct block_t_Block* tBlock = lua_touserdata(L, -1);

	// Create event
	struct block_Event* event = malloc(sizeof(struct block_Event));
	event->blockId = tBlock->blockId;
	event->type = BLOCK_EVENT_SETTEXT;
	event->wstr = strWiden(lua_tostring(L, 1), (int)lua_strlen(L, 1), &event->wstrlen);

	// Send event and abandon ownership over struct
	PostThreadMessage(WBLOCKS_MESSAGE_THREAD_ID, WBLOCKS_WM_BLOCK_EVENT, 0, (LPARAM)event);

	return 0;
}

static int lSetColor(lua_State* L)
{
	if (!lua_isnumber(L, 1)) return luaL_argerror(L, 1, "not a number");
	if (!lua_isnumber(L, 2)) return luaL_argerror(L, 2, "not a number");
	if (!lua_isnumber(L, 3)) return luaL_argerror(L, 3, "not a number");

	// Get tBlock
	lua_getglobal(L, BLOCK_LUA_TBLOCK_GLOBAL);
	struct block_t_Block* tBlock = lua_touserdata(L, -1);

	// Create event
	struct block_Event* event = malloc(sizeof(struct block_Event));
	event->blockId = tBlock->blockId;
	event->type = BLOCK_EVENT_SETCOLOR;
	event->color = RGB(lua_tointeger(L, 1) & 0xFF, lua_tointeger(L, 2) & 0xFF, lua_tointeger(L, 3) & 0xFF);

	// Send event and abandon ownership over struct
	PostThreadMessage(WBLOCKS_MESSAGE_THREAD_ID, WBLOCKS_WM_BLOCK_EVENT, 0, (LPARAM)event);

	return 0;
}

static int lAddTimer(lua_State* L)
{
	if (!lua_isnumber(L, 1)) return luaL_argerror(L, 1, "not a number");
	if (!lua_isfunction(L, 2)) return luaL_argerror(L, 2, "not a function");

	// Get function ref
	lua_pushvalue(L, 2);
	int ref = luaL_ref(L, LUA_REGISTRYINDEX);

	// Get tBlock
	lua_getglobal(L, BLOCK_LUA_TBLOCK_GLOBAL);
	struct block_t_Block* tBlock = lua_touserdata(L, -1);

	// Create timer
	struct ltimer* timer = malloc(sizeof(struct ltimer));
	timer->time = GetTickCount64() + lua_tointeger(L, 1);
	timer->luaRef = ref;

	// Place in timer list
	struct ltimer* current = &tBlock->timer;
	while (current->next && timer->time >= current->next->time) {
		current = current->next;
	}

	timer->next = current->next;
	timer->prev = current;
	if (current->next) current->next->prev = timer;
	current->next = timer;

	return 0;
}

static void scriptTimerHandler(struct block_t_Block* tBlock)
{
	// Process timers
	uint64_t time = GetTickCount64();
	struct ltimer* tmp;
	struct ltimer* current = tBlock->timer.next;
	while (current && current->time <= time) {
		// Call timer
		lua_rawgeti(tBlock->L, LUA_REGISTRYINDEX, current->luaRef);
		if (lua_pcall(tBlock->L, 0, 0, 0)) {
			printf("Lua error: %s\n", lua_tostring(tBlock->L, -1));
		}
		luaL_unref(tBlock->L, LUA_REGISTRYINDEX, current->luaRef);

		// Remove timer
		current->prev->next = current->next;
		if (current->next) current->next->prev = current->prev;
		tmp = current;
		current = current->next;
		free(tmp);
	}
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
	lua_pushcfunction(L, lSetColor);
	lua_setglobal(L, "SetColor");
	lua_pushcfunction(L, lAddTimer);
	lua_setglobal(L, "AddTimer");

	// Load script
	if (luaL_dofile(L, tBlock->scriptPath)) {
		printf("Lua error: %s\n", lua_tostring(L, -1));
		return 1;
	}

	// Add timer handler
	SetTimer(0, 0, BLOCK_LUA_TIMER_INTERVAL, 0);

	// Message loop
	MSG message;
	while (GetMessage(&message, NULL, 0, 0)) {
		if (message.message == WM_TIMER) {
			scriptTimerHandler(tBlock);
		}
	}

	return 0;
}

struct block_Block* block_addScriptBlock(char* scriptPath)
{
	printf("Loading: %s\n", scriptPath);

	// Create bar block
	struct block_Block* bBlock = malloc(sizeof(struct block_Block));
	memset(bBlock, 0, sizeof(struct block_Block));
	bBlock->blockId = blockCount;
	bBlock->color = 0xffffff;

	// Create thread block
	struct block_t_Block* tBlock = malloc(sizeof(struct block_t_Block));
	memset(tBlock, 0, sizeof(struct block_t_Block));
	bBlock->tBlock = tBlock;
	tBlock->blockId = blockCount;
	tBlock->timer.prev = &tBlock->timer;
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

struct block_Block* block_addStaticBlock(char* str, int len)
{
	// Create bar block
	struct block_Block* bBlock = malloc(sizeof(struct block_Block));
	memset(bBlock, 0, sizeof(struct block_Block));
	bBlock->blockId = blockCount;
	bBlock->color = 0xffffff;

	// Set string
	bBlock->text.wstr = strWiden(str, len, &bBlock->text.wlen);

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
	} else if (event->type == BLOCK_EVENT_SETCOLOR) {
		bBlock->color = event->color;
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
