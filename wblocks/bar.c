#include "bar.h"
#include "wblocks.h"
#include "block.h"

#include <time.h>
#include <Windows.h>
#include <Windowsx.h>

#define BAR_TEXT_FLAGS (DT_NOCLIP | DT_NOPREFIX | DT_SINGLELINE | DT_RIGHT | DT_VCENTER)
#define HOOK_PING_TIME 1
#define HOOK_DEAD_TIME (HOOK_PING_TIME * 3)
#define BAR_WIDTH 500

static HCURSOR cursor = NULL;
static HFONT font = NULL;
static HDC displayDC = NULL;

static HMODULE dll = NULL;
static HHOOK hook = NULL;

static HWND barWnd = NULL;
static time_t lastPong;

static HWND taskbarWnd = NULL;
static int taskbarWidth, taskbarHeight;
static COLORREF taskbarColor;

static void printWin32Error()
{
    wchar_t buf[1024];
    FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buf, 1024, NULL);
    printf("%ls\n", buf);
}

static int loadResources()
{
    // Get display DC
    displayDC = CreateDC("DISPLAY", NULL, NULL, NULL);
    if (!displayDC) {
        printf("Failed to get display dc!\n");
        printWin32Error();
        return 1;
    }

    // Load cursor
    cursor = LoadCursor(NULL, IDC_ARROW);
    if (!cursor) {
        printf("Failed to load cursor!\n");
        printWin32Error();
        return 1;
    }

    // Load font
    // todo: don't use Open Sans, allow config to decide
    font = CreateFont(-16, 0, 0, 0, FW_NORMAL, 0, 0, 0, 0, 0, 0, 0, 0, "Open Sans");
    if (!font) {
        printf("Failed to load title font!\n");
        printWin32Error();
        return 1;
    }

    return 0;
}

static void freeResources()
{
    DeleteDC(displayDC);
    displayDC = NULL;
    DestroyCursor(cursor);
    cursor = NULL;
    DeleteObject(font);
    font = NULL;
}

static HWND getTaskbar()
{
    HWND temp = FindWindow("Shell_TrayWnd", NULL);
    if (!temp) {
        printf("Failed to find Shell_TrayWnd\n");
        return 0;
    }

    temp = FindWindowEx(temp, NULL, "ReBarWindow32", NULL);
    if (!temp) {
        printf("Failed to find ReBarWindow32\n");
        return 0;
    }

    return temp;
}


static void updateWindow()
{
    if (barWnd == NULL) return;
    taskbarWnd = getTaskbar();
    if (taskbarWnd == NULL) return;

    // Set taskbar as parent
    SetParent(barWnd, taskbarWnd);

    // Get taskbar size
    RECT taskbarRect;
    GetWindowRect(taskbarWnd, &taskbarRect);
    taskbarWidth = taskbarRect.right - taskbarRect.left;
    taskbarHeight = taskbarRect.bottom - taskbarRect.top;

    // Get taskbar color
    taskbarColor = GetPixel(displayDC, taskbarWidth / 2 - 1, taskbarRect.top + taskbarHeight / 2);

    // Set colorkey
    SetLayeredWindowAttributes(barWnd, taskbarColor, 0, LWA_COLORKEY);

    // Set pos
    SetWindowPos(barWnd, NULL,
        taskbarWidth - BAR_WIDTH, 0, BAR_WIDTH, taskbarHeight,
        SWP_FRAMECHANGED | SWP_NOZORDER | SWP_NOOWNERZORDER);
}

static void removeHook()
{
    UnhookWindowsHookEx(hook);
    hook = NULL;
    FreeLibrary(dll);
    dll = NULL;
}

static int injectHook()
{
    if (!taskbarWnd) return 1;
    if (hook) removeHook();

    dll = LoadLibraryA("wblocks-dll.dll");
    if (!dll) {
        printf("DLL failed to load!\n");
        printWin32Error();
        return 1;
    }

    HOOKPROC addr = (HOOKPROC)GetProcAddress(dll, "wndProcHook");
    if (!addr) {
        printf("DLL function not found!\n");
        printWin32Error();
        return 1;
    }

    DWORD tid = GetWindowThreadProcessId(taskbarWnd, 0);
    if (!tid) {
        printf("Window Thread ID not found!\n");
        printWin32Error();
        return 1;
    }

    hook = SetWindowsHookEx(WH_CALLWNDPROC, addr, dll, tid);
    if (!hook) {
        printf("WH_CALLWNDPROC hook failed!\n");
        printWin32Error();
        return 1;
    }

    return 0;
}

static LRESULT CALLBACK wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_PAINT) {
        // Begin
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps); // todo use buffer

        // Draw rect
        RECT drawRect;
        drawRect.left = 0;
        drawRect.top = 0;
        drawRect.right = BAR_WIDTH;
        drawRect.bottom = taskbarHeight;

        // Clear
        SelectObject(hdc, GetStockObject(DC_PEN));
        SelectObject(hdc, GetStockObject(DC_BRUSH));
        SetDCPenColor(hdc, taskbarColor);
        SetDCBrushColor(hdc, taskbarColor);
        Rectangle(hdc, 0, 0, taskbarWidth, taskbarHeight);

        // Draw text
        SetBkMode(hdc, TRANSPARENT);
        SelectObject(hdc, font);

        // Get blocks
        struct block_Block** blocks = block_getBlocks();
        int blockCount = block_getBlockCount();
        SIZE textSize;
        for (int i = blockCount - 1; i >= 0; i--) {
            SetTextColor(hdc, blocks[i]->color);
            DrawTextW(hdc, blocks[i]->text.wstr, blocks[i]->text.wlen, &drawRect, BAR_TEXT_FLAGS);
            GetTextExtentPoint32W(hdc, blocks[i]->text.wstr, blocks[i]->text.wlen, &textSize);
            drawRect.right -= textSize.cx;
            blocks[i]->bar_xpos = drawRect.right;
            blocks[i]->bar_width = textSize.cx;
        }

        // End
        EndPaint(hwnd, &ps);
        return 0;
    } else if (msg == WM_LBUTTONDOWN) {
        RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE);

        // Find affected block
        int mx = GET_X_LPARAM(lParam);
        struct block_Block** blocks = block_getBlocks();
        int blockCount = block_getBlockCount();
        for (int i = 0; i < blockCount; i++) {
            if (mx >= blocks[i]->bar_xpos && mx < blocks[i]->bar_xpos + blocks[i]->bar_width) {
                if (blocks[i]->blockIsScripted) {
                    // Create event
                    struct block_Event* event = malloc(sizeof(struct block_Event));
                    event->type = BLOCK_EVENT_MOUSE_DOWN;

                    // Send event and abandon ownership over struct
                    PostThreadMessage(blocks[i]->scriptThreadId, WBLOCKS_WM_BLOCK_SCRIPT_EVENT, 0, (LPARAM)event);
                }
                break;
            }
        }
    } else if (msg == WBLOCKS_WM_PONG) {
        lastPong = time(0);
    } else if (msg == WBLOCKS_WM_SIZE) {
        updateWindow();
    } else if (msg == WM_NCDESTROY) {
        barWnd = NULL;
        removeHook();
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

static int registerClass()
{
    WNDCLASSEX wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = wndProc;
    wc.lpszClassName = WBLOCKS_BAR_CLASS;
    wc.hCursor = cursor;
    if (!RegisterClassEx(&wc)) {
        printf("Failed to register bar class!\n");
        printWin32Error();
        return 1;
    }

    return 0;
}

static int createWindow()
{
    // Create notification window
    barWnd = CreateWindowEx(WS_EX_LAYERED, WBLOCKS_BAR_CLASS, "", 0, 0, 0, 0, 0, 0, 0, 0, 0);
    if (!barWnd) {
        printf("Failed to create window!\n");
        printWin32Error();
        return 1;
    }

    // Remove default window styling
    LONG lStyle = GetWindowLong(barWnd, GWL_STYLE);
    lStyle &= ~WS_OVERLAPPEDWINDOW;
    lStyle |= WS_CHILD;
    SetWindowLong(barWnd, GWL_STYLE, lStyle);

    // Show window
    ShowWindow(barWnd, SW_SHOW);

    // Update
    updateWindow();

    return 0;
}

static void destroyWindow()
{
    DestroyWindow(barWnd);
    barWnd = NULL;
}

static VOID CALLBACK aliveTimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
    // Check when we got last pong
    if (time(0) - lastPong >= HOOK_DEAD_TIME) {
        printf("Bar dead!\n");
        if (barWnd == NULL) {
            createWindow();
        } else {
            updateWindow();
        }
        injectHook();
    }

    // Send ping
    SendMessage(taskbarWnd, WBLOCKS_WM_PING, 0, 0);
}

int bar_init()
{
    if (loadResources() || registerClass() || createWindow() || injectHook()) return 1;

    // Create repeating timer
    lastPong = time(0);
    SetTimer(0, 0, HOOK_PING_TIME * 1000, aliveTimerProc);

    return 0;
}

void bar_redraw()
{
    if (!barWnd) return;
    RedrawWindow(barWnd, NULL, NULL, RDW_INVALIDATE);
}

void bar_quit()
{
    freeResources();
    destroyWindow();
    removeHook();
}
