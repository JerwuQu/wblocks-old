#include "block.h"
#include "bar.h"
#include "../shared/shared.h"

#define BLOCK_LUA_TIMER_INTERVAL 100

static char BLOCK_THREAD_DATA_REGKEY;
static int blockCount = 0;
static struct block_Block** blocks = NULL;

static inline struct block_BlockThreadData* getBlockThreadData(lua_State* L)
{
    lua_pushlightuserdata(L, &BLOCK_THREAD_DATA_REGKEY);
    lua_gettable(L, LUA_REGISTRYINDEX);
    return lua_touserdata(L, -1);
}

static int l_block_settext(lua_State* L)
{
    if (!lua_isstring(L, 1)) return luaL_argerror(L, 1, "not a string");

    // Create event
    struct block_ModifyEvent* event = malloc(sizeof(struct block_ModifyEvent));
    event->blockId = getBlockThreadData(L)->blockId;
    event->type = BLOCK_MEVENT_SETTEXT;
    event->wstr = strWiden(lua_tostring(L, 1), (int)lua_strlen(L, 1), &event->wstrlen);

    // Send event and abandon ownership over struct
    PostThreadMessage(WBLOCKS_MESSAGE_THREAD_ID, WBLOCKS_WM_BLOCK_MODIFY_EVENT, 0, (LPARAM)event);

    return 0;
}

static int l_block_setcolor(lua_State* L)
{
    if (!lua_isnumber(L, 1)) return luaL_argerror(L, 1, "not a number");
    if (!lua_isnumber(L, 2)) return luaL_argerror(L, 2, "not a number");
    if (!lua_isnumber(L, 3)) return luaL_argerror(L, 3, "not a number");

    // Create event
    struct block_ModifyEvent* event = malloc(sizeof(struct block_ModifyEvent));
    event->blockId = getBlockThreadData(L)->blockId;
    event->type = BLOCK_MEVENT_SETCOLOR;
    event->color = RGB(lua_tointeger(L, 1) & 0xFF, lua_tointeger(L, 2) & 0xFF, lua_tointeger(L, 3) & 0xFF);

    // Send event and abandon ownership over struct
    PostThreadMessage(WBLOCKS_MESSAGE_THREAD_ID, WBLOCKS_WM_BLOCK_MODIFY_EVENT, 0, (LPARAM)event);

    return 0;
}

static int l_timer_add(lua_State* L)
{
    if (!lua_isnumber(L, 1)) return luaL_argerror(L, 1, "not a number");
    if (!lua_isfunction(L, 2)) return luaL_argerror(L, 2, "not a function");

    // Get function ref
    lua_pushvalue(L, 2);
    int ref = luaL_ref(L, LUA_REGISTRYINDEX);

    // Get threadData
    struct block_BlockThreadData* threadData = getBlockThreadData(L);

    // Create timer
    struct ltimer* timer = malloc(sizeof(struct ltimer));
    timer->id = ++threadData->timerIdCounter;
    timer->time = GetTickCount64() + lua_tointeger(L, 1);
    timer->luaRef = ref;

    // Place in timer list
    struct ltimer* current = &threadData->timerRoot;
    while (current->next && timer->time >= current->next->time) {
        current = current->next;
    }

    timer->next = current->next;
    timer->prev = current;
    if (current->next) current->next->prev = timer;
    current->next = timer;

    // Return timer id
    lua_pushinteger(L, timer->id);
    return 1;
}

static int l_timer_remove(lua_State* L)
{
    if (!lua_isnumber(L, 1)) return luaL_argerror(L, 1, "not a number");
    lua_Integer timerId = lua_tointeger(L, 1);

    // Search for timer
    struct ltimer* current = &getBlockThreadData(L)->timerRoot;
    while (current && current->id != timerId) {
        current = current->next;
    }

    // Remove timer
    if (current) {
        current->prev->next = current->next;
        if (current->next) current->next->prev = current->prev;
        free(current);
    }

    return 0;
}

static void scriptTimerHandler(struct block_BlockThreadData* threadData)
{
    // Process timers
    uint64_t time = GetTickCount64();
    struct ltimer* tmp;
    struct ltimer* current = threadData->timerRoot.next;
    while (current && current->time <= time) {
        // Call timer
        lua_rawgeti(threadData->L, LUA_REGISTRYINDEX, current->luaRef);
        if (lua_pcall(threadData->L, 0, 0, 0)) {
            printf("Lua error: %s\n", lua_tostring(threadData->L, -1));
        }
        luaL_unref(threadData->L, LUA_REGISTRYINDEX, current->luaRef);

        // Remove timer
        current->prev->next = current->next;
        if (current->next) current->next->prev = current->prev;
        tmp = current;
        current = current->next;
        free(tmp);
    }
}

static void scriptBlockEventCall(lua_State* L, char* name, char* argTypes, ...)
{
    // Find block table
    lua_getglobal(L, "block");
    if (lua_istable(L, -1)) {
        // Find function
        lua_pushstring(L, name);
        lua_gettable(L, -2);
        if (lua_isfunction(L, -1)) {
            // Handle arguments - handles bools, ints and strings
            va_list valist;
            va_start(valist, argTypes);
            int i = 0;
            for (;argTypes[i]; i++) {
                if (argTypes[i] == 'b') {
                    lua_pushboolean(L, va_arg(valist, int));
                } else if (argTypes[i] == 'i') {
                    lua_pushinteger(L, va_arg(valist, int));
                } else if (argTypes[i] == 's') {
                    lua_pushstring(L, va_arg(valist, char*));
                } else {
                    printf("Invalid argument type '%c'\n", argTypes[i]);
                    lua_pop(L, 1);
                    return;
                }
            }
            va_end(valist);

            // Call
            if (lua_pcall(L, i, 0, 0)) {
                printf("Lua error: %s\n", lua_tostring(L, -1));
            }
            lua_pop(L, 1);
        } else {
            lua_pop(L, 2);
        }
    } else {
        lua_pop(L, 1);
    }
}

static void scriptEventHandler(struct block_InteractEvent* event, struct block_BlockThreadData* threadData)
{
    if (event->type == BLOCK_IEVENT_MOUSE_DOWN) {
        scriptBlockEventCall(threadData->L, "mousedown", "");
    } else if (event->type == BLOCK_IEVENT_MOUSE_SCROLL) {
        scriptBlockEventCall(threadData->L, "mousescroll", "i", event->wheelDelta);
    }

    free(event);
}

static DWORD WINAPI threadProc(LPVOID lpParameter)
{
    struct block_BlockThreadData* threadData = (struct block_BlockThreadData*)lpParameter;

    // Create lua state
    lua_State* L = threadData->L = luaL_newstate();
    luaL_openlibs(L);
    luaJIT_setmode(L, 0, LUAJIT_MODE_ENGINE | LUAJIT_MODE_ON);

    // Store thread data ptr in lua state
    lua_pushlightuserdata(L, &BLOCK_THREAD_DATA_REGKEY);
    lua_pushlightuserdata(L, threadData);
    lua_settable(L, LUA_REGISTRYINDEX);

    // Add function tables
    lua_newtable(L);
        lua_pushstring(L, "setText");
        lua_pushcfunction(L, l_block_settext);
        lua_settable(L, -3);

        lua_pushstring(L, "setColor");
        lua_pushcfunction(L, l_block_setcolor);
        lua_settable(L, -3);
    lua_setglobal(L, "block");

    lua_newtable(L);
        lua_pushstring(L, "add");
        lua_pushcfunction(L, l_timer_add);
        lua_settable(L, -3);

        lua_pushstring(L, "remove");
        lua_pushcfunction(L, l_timer_remove);
        lua_settable(L, -3);
    lua_setglobal(L, "timer");

    // Load script
    if (luaL_dofile(L, threadData->scriptPath)) {
        printf("Lua error: %s\n", lua_tostring(L, -1));
        lua_close(L);
        return 1;
    }

    // Add timer handler
    SetTimer(0, 0, BLOCK_LUA_TIMER_INTERVAL, 0);

    // Message loop
    MSG message;
    while (GetMessage(&message, NULL, 0, 0)) {
        if (message.message == WM_TIMER) {
            scriptTimerHandler(threadData);
        } else if (message.message == WBLOCKS_WM_BLOCK_INTERACT_EVENT) {
            struct block_InteractEvent* event = (struct block_InteractEvent*)message.lParam;
            scriptEventHandler(event, threadData);
        }
    }

    return 0;
}

// Abandon ownership of style to create a blcok
static inline struct block_Block* createBlockBase(struct block_BlockStyle* style)
{
    // Create block
    struct block_Block* block = malloc(sizeof(struct block_Block));
    memset(block, 0, sizeof(struct block_Block));
    block->blockId = blockCount;

    // Copy style
    block->style = style;

    // Add to list
    // todo: expand by factor of 2
    blocks = realloc(blocks, (blockCount + 1) * sizeof(struct block_Block));
    blocks[blockCount++] = block;
    return block;
}

static inline void freeBlockStyle(struct block_BlockStyle* style)
{
    free(style->text.wstr);
    DeleteObject(style->font);
    free(style->minWidthStr.wstr);
    free(style);
}

// Only to be used in case of error when creating block - never afterwards
// todo: make this a proper function to remove any block
//     + shrink "blocks" by factor of 2
static inline void freeLastBlock()
{
    blockCount--;
    freeBlockStyle(blocks[blockCount]->style);
    free(blocks[blockCount]);
}

struct block_Block* block_addScriptBlock(char* scriptPath, struct block_BlockStyle* style)
{
    printf("- Loading: %s\n", scriptPath);
    struct block_Block* block = createBlockBase(style);
    block->blockIsScripted = 1;

    // Create thread data
    struct block_BlockThreadData* threadData = malloc(sizeof(struct block_BlockThreadData));
    memset(threadData, 0, sizeof(struct block_BlockThreadData));
    threadData->blockId = block->blockId;
    threadData->timerRoot.prev = &threadData->timerRoot;
    threadData->scriptPath = _strdup(scriptPath);
    if (!CreateThread(NULL, 0, threadProc, threadData, 0, &block->scriptThreadId)) {
        printf("Failed to create script thread!\n");
        freeLastBlock();
        free(threadData->scriptPath);
        free(threadData);
        return 0;
    }

    return block;
}

struct block_Block* block_addStaticBlock(struct block_BlockStyle* style)
{
    return createBlockBase(style);
}

void block_barEventHandler(struct block_ModifyEvent* event)
{
    if (event->blockId < 0 || event->blockId >= blockCount) return; // Block doesn't exist
    struct block_Block* block = blocks[event->blockId];

    if (event->type == BLOCK_MEVENT_SETTEXT) {
        if (block->style->text.wlen != event->wstrlen || memcmp(block->style->text.wstr, event->wstr, event->wstrlen * sizeof(wchar_t))) {
            free(block->style->text.wstr);
            block->style->text.wstr = event->wstr;
            block->style->text.wlen = event->wstrlen;
            bar_redraw();
        } else {
            free(event->wstr);
        }
    } else if (event->type == BLOCK_MEVENT_SETCOLOR) {
        if (block->style->color != event->color) {
            block->style->color = event->color;
            bar_redraw();
        }
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
