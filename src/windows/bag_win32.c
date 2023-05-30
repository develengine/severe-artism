#include "bag_engine_config.h"
#include "bag_engine.h"

#include <windows.h>

#include <stdio.h>
#include <locale.h>
#include <wchar.h>

/* TODO
 * [X] void bagE_pollEvents();
 * [X] void bagE_swapBuffers();
 * 
 * [X] int bagE_getCursorPosition(int *x, int *y);
 * [X] void bagE_getWindowSize(int *width, int *height);
 * 
 * [X] int bagE_isAdaptiveVsyncAvailable(void);
 * 
 * [X] int bagE_setHiddenCursor(int value);
 * [X] void bagE_setFullscreen(int value);
 * [X] void bagE_setWindowTitle(char *value);
 * [X] void bagE_setSwapInterval(int value);
 * [X] void bagE_setCursorPosition(int x, int y);
 *
 * [X] bagE_EventWindowClose,
 * [X] bagE_EventWindowResize,
 *
 * [X] bagE_EventMouseMotion,
 * [X] bagE_EventMouseButtonUp,
 * [X] bagE_EventMouseButtonDown,
 * [X] bagE_EventMouseWheel,
 * [X] bagE_EventMousePosition,
 *
 * [X] bagE_EventKeyUp,
 * [X] bagE_EventKeyDown,
 * [X] bagE_EventTextUTF8,
 * [X] bagE_EventTextUTF32,
 */

/* FIXME 
 * [ ] can't change keyboard language when window selected
 * [X] cursor doesn't update to arrow when hower over client area
 */


/* Uses dedicated gpu by default but creates a dll file. */
#if 0
__declspec(dllexport) unsigned long NvOptimusEnablement = 1;
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
#endif


// #include "wglext.h"

#define WGL_CONTEXT_MAJOR_VERSION_ARB     0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB     0x2092
#define WGL_CONTEXT_FLAGS_ARB             0x2094
#define WGL_SUPPORT_OPENGL_ARB            0x2010
#define WGL_SAMPLES_ARB                   0x2042

#define WGL_CONTEXT_DEBUG_BIT_ARB         0x00000001

#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif

typedef HGLRC (WINAPI * PFNWGLCREATECONTEXTATTRIBSARBPROC) (HDC hDC, HGLRC hShareContext, const int *attribList);
static PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = 0;

typedef BOOL (WINAPI * PFNWGLSWAPINTERVALEXTPROC) (int interval);
static PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = 0;

typedef const char *(WINAPI * PFNWGLGETEXTENSIONSSTRINGARBPROC) (HDC hdc);
static PFNWGLGETEXTENSIONSSTRINGARBPROC wglGetExtensionsStringARB = 0;


#define BAGWIN32_CHECK_WGL_PROC_ADDR(identifier) {\
    void *pfn = (void*)identifier;\
    if ((pfn == 0)\
    ||  (pfn == (void*)0x1) || (pfn == (void*)0x2) || (pfn == (void*)0x3)\
    ||  (pfn == (void*)-1)\
    ) return procedureIndex;\
    ++procedureIndex;\
}


static struct bagWIN32_Program
{
    HINSTANCE instance;
    HWND window;
    HGLRC context;
    HDC deviceContext;
    int openglLoaded;
    int newContext;
    int adaptiveVsync;
    /* state */
    int processingEvents;
    int cursorHidden;
    int prevX, prevY;
    WINDOWPLACEMENT previousWP;
    int highSurrogate;
    HANDLE stdoutHandle;
    DWORD outModeOld;
    HCURSOR cursor;
    int cursorInClient;
} bagWIN32;

typedef struct bagWIN32_Program bagWIN32_Program;


static void bagWIN32_error(const char *message)
{
    MessageBoxA(NULL, message, "BAG Engine", MB_ICONERROR);
}


static int bagWIN32_loadWGLFunctionPointers()
{
    int procedureIndex = 0;

    wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)
        wglGetProcAddress("wglCreateContextAttribsARB");
    BAGWIN32_CHECK_WGL_PROC_ADDR(wglCreateContextAttribsARB);

    wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)
        wglGetProcAddress("wglSwapIntervalEXT");
    BAGWIN32_CHECK_WGL_PROC_ADDR(wglSwapIntervalEXT);

    wglGetExtensionsStringARB = (PFNWGLGETEXTENSIONSSTRINGARBPROC)
        wglGetProcAddress("wglGetExtensionsStringARB");
    BAGWIN32_CHECK_WGL_PROC_ADDR(wglGetExtensionsStringARB);

    return -1;
}


static int bagWIN32_isWGLExtensionSupported(const char *extList, const char *extension)
{
    const char *start;
    const char *where, *terminator;
  
    where = strchr(extension, ' ');
    if (where || *extension == '\0')
        return 0;

    for (start=extList;;) {
        where = strstr(start, extension);

        if (!where)
            break;

        terminator = where + strlen(extension);

        if (where == start || *(where - 1) == ' ') {
            if (*terminator == ' ' || *terminator == '\0')
                return 1;
        }

        start = terminator;
    }

    return 0;
}


LRESULT CALLBACK bagWIN32_windowProc(
        HWND windowHandle,
        UINT message,
        WPARAM wParam,
        LPARAM lParam
);


#ifdef BAGE_WINDOWS_CONSOLE
int main(int argc, char *argv[]) {
    (void)argc; (void)argv;

    bagWIN32.instance = GetModuleHandle(NULL);
    
    DWORD outMode = 0;
    bagWIN32.stdoutHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    
    if (bagWIN32.stdoutHandle == INVALID_HANDLE_VALUE) {
        bagWIN32_error("Failed to retrieve stdout handle!");
        exit(-1);
    }
    
    if (!GetConsoleMode(bagWIN32.stdoutHandle, &outMode)) {
        bagWIN32_error("Failed to retrieve console mode!");
        exit(-1);
    }
    
    bagWIN32.outModeOld = outMode;
    outMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    
    if (!SetConsoleMode(bagWIN32.stdoutHandle, outMode)) {
        bagWIN32_error("Failed to set console mode!");
        exit(-1);
    }
    
#else
int WinMain(
        HINSTANCE hinstance,
        HINSTANCE previousInstance,
        LPSTR commandLine,
        int nShowCmd
) {
    bagWIN32.instance = hinstance;
#endif
    setlocale(LC_CTYPE, ""); // NOTE: Might need to be LC_ALL
    bagWIN32.openglLoaded = 0;
    bagWIN32.cursor = LoadCursorA(NULL, IDC_ARROW);

    /* WIN32 */

    WNDCLASSW windowClass = {
        .style = CS_OWNDC,
        .lpfnWndProc = &bagWIN32_windowProc,
        .hInstance = bagWIN32.instance,
        .lpszClassName = L"bag engine"
    };

    ATOM registerOutput = RegisterClassW(&windowClass);
    if (!registerOutput) {
        bagWIN32_error("Failed to register window class!");
        exit(-1);
    }

    wchar_t windowTitle[256];
    int errorRet = mbstowcs_s(
            NULL,
            windowTitle,
            256,
            bagE_defaultTitle,
            256
    );
    if (errorRet) 
        fprintf(stderr, "Problems converting default window title");


    bagWIN32.window = CreateWindowExW(
            0,
            windowClass.lpszClassName,
            windowTitle,
            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            /* NOTE: We add pixels to take borders into consideration
             *       and the default values to correspond to client area. */
            bagE_defaultWindowWidth + 16,
            bagE_defaultWindowHeight + 39,
            0,
            0,
            bagWIN32.instance,
            0
    );

    if (!bagWIN32.window) {
        bagWIN32_error("Failed to create window!");
        exit(-1);
    }

    bagWIN32.cursorHidden = 0;
    bagWIN32.previousWP.length = sizeof(bagWIN32.previousWP);
    bagWIN32.prevX = 0;
    bagWIN32.prevY = 0;

    
    /* Raw input */

    RAWINPUTDEVICE rawInputDevice = {
        .usUsagePage = 0x01,  // HID_USAGE_PAGE_GENERIC
        .usUsage = 0x02,  // HID_USAGE_GENERIC_MOUSE
        .dwFlags = 0,
        .hwndTarget = bagWIN32.window
    };

    if (!RegisterRawInputDevices(&rawInputDevice, 1, sizeof(rawInputDevice))) {
        bagWIN32_error("Failed to register raw input device!");
        exit(-1);
    }


    TRACKMOUSEEVENT trackMouseEvent = {0};
    trackMouseEvent.cbSize = sizeof(trackMouseEvent);
    trackMouseEvent.dwFlags = TME_LEAVE;
    trackMouseEvent.hwndTrack = bagWIN32.window;
    trackMouseEvent.dwHoverTime = HOVER_DEFAULT;

    if (!TrackMouseEvent(&trackMouseEvent)) {
        bagWIN32_error("Failed start tracking the mouse events!");
        exit(-1);
    }

    /* WGL context creation */

    // TODO: expose to the user
    PIXELFORMATDESCRIPTOR pfd = {
        .nSize = sizeof(PIXELFORMATDESCRIPTOR),
        .nVersion = 1,
        .dwFlags = PFD_DOUBLEBUFFER | PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW,
        .iPixelType = PFD_TYPE_RGBA,
        .cColorBits = 32,
        .cDepthBits = 24,
        .cStencilBits = 8,
        .iLayerType = PFD_MAIN_PLANE
    };

    bagWIN32.deviceContext = GetDC(bagWIN32.window);
    if (!bagWIN32.deviceContext) {
        bagWIN32_error("Failed to retrieve device context!");
        exit(-1);
    }

    int pfIndex = ChoosePixelFormat(bagWIN32.deviceContext, &pfd);
    if (pfIndex == 0) {
        bagWIN32_error("Failed to chooe pixel format!");
        exit(-1);
    }

    BOOL bResult = SetPixelFormat(bagWIN32.deviceContext, pfIndex, &pfd);
    if (!bResult) {
        bagWIN32_error("Failed to set pixel format!");
        exit(-1);
    }

    HGLRC tContext = wglCreateContext(bagWIN32.deviceContext);
    if (!tContext) {
        bagWIN32_error("Failed to create temporary OpenGL context!");
        exit(-1);
    }

    bResult = wglMakeCurrent(bagWIN32.deviceContext, tContext);
    if (!bResult) {
        bagWIN32_error("Failed to make tomporary OpenGL context current!");
        exit(-1);
    }

    int procIndex = -1;
    if ((procIndex = bagWIN32_loadWGLFunctionPointers()) != -1) {
        fprintf(stderr, "Failed to retrieve function number %d\n", procIndex);
        bagWIN32.context = tContext;
        bagWIN32.newContext = 0;
        bagWIN32.adaptiveVsync = 0;
    } else {
        // TODO: expose to the user
        
        int contextFlags = 0;
#ifdef _DEBUG
        contextFlags |= WGL_CONTEXT_DEBUG_BIT_ARB;
#endif

        int attribs[] = {
            WGL_CONTEXT_MAJOR_VERSION_ARB, bagE_oglContextMajorVersion,
            WGL_CONTEXT_MINOR_VERSION_ARB, bagE_oglContextMinorVersion,
            WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
            WGL_CONTEXT_FLAGS_ARB, contextFlags,
            // WGL_SAMPLES_ARB, 4,
            0
        };
    
        bagWIN32.context = wglCreateContextAttribsARB(bagWIN32.deviceContext, 0, attribs);
        if (!bagWIN32.context) {
            bagWIN32_error("Failed to create OpenGL context!");
            exit(-1);
        }
    
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(tContext);
        wglMakeCurrent(bagWIN32.deviceContext, bagWIN32.context);
        bagWIN32.newContext = 1;

        const char *extensionString = wglGetExtensionsStringARB(bagWIN32.deviceContext);

        bagWIN32.adaptiveVsync = bagWIN32_isWGLExtensionSupported(
                extensionString,
                "WGL_EXT_swap_control_tear"
        );
    }

    if (!gladLoaderLoadGL()) {
        bagWIN32_error("glad failed to load OpenGL!");
        exit(-1);
    }
    bagWIN32.openglLoaded = 1;


    /* Running the main function */

    int programReturn = bagE_main(__argc, __argv);

    ReleaseDC(bagWIN32.window, bagWIN32.deviceContext);
    wglDeleteContext(bagWIN32.context);

    if (!SetConsoleMode(bagWIN32.stdoutHandle, bagWIN32.outModeOld)) {
        bagWIN32_error("Failed to restore console mode!");
        exit(-1);
    }
    
    return programReturn;
}


int bagWIN32_utf8FromCodePoint(unsigned int codePoint, unsigned char string[]) {
    if (codePoint < 0x80) {
        string[0] = (unsigned char)codePoint;
        return 1;
    }
    if (codePoint < 0x800) {
        string[0] = (unsigned char)(0xC0 | (codePoint >> 6));
        string[1] = (unsigned char)(0x80 | (codePoint & 0x3F));
        return 2;
    }
    if (codePoint < 0x10000) {
        string[0] = (unsigned char)(0xE0 | (codePoint >> 12));
        string[1] = (unsigned char)(0x80 | ((codePoint >> 6) & 0x3F));
        string[2] = (unsigned char)(0x80 | (codePoint & 0x3F));
        return 3;
    }
    if (codePoint < 0x110000) {
        string[0] = (unsigned char)(0xF0 | (codePoint >> 18));
        string[1] = (unsigned char)(0x80 | ((codePoint >> 12) & 0x3F));
        string[2] = (unsigned char)(0x80 | ((codePoint >> 6) & 0x3F));
        string[3] = (unsigned char)(0x80 | (codePoint & 0x3F));
        return 4;
    }

    return 0;
}


static void bagWIN32_updateCursor(void)
{
    SetCursor(bagWIN32.cursor);
}


LRESULT CALLBACK bagWIN32_windowProc(
        HWND windowHandle,
        UINT message,
        WPARAM wParam,
        LPARAM lParam
) {
    #define BAGWIN32_EVENT_COUNT 2

    if (!bagWIN32.openglLoaded)
        return DefWindowProcW(windowHandle, message, wParam, lParam);

    LRESULT result = 0;
    bagE_Event events[BAGWIN32_EVENT_COUNT];
    bagE_Event *event = events;

    for (int i = 0; i < BAGWIN32_EVENT_COUNT; i++)
        events[i].type = bagE_EventNone;

    switch (message)
    {
        case WM_CLOSE:
            event->type = bagE_EventWindowClose;
            break;

        case WM_WINDOWPOSCHANGED: {
            unsigned int winFlags = ((WINDOWPOS*)lParam)->flags;

            if ((winFlags & SWP_NOSIZE) && (winFlags & SWP_NOMOVE))
                break;

            RECT winRect;
            GetClientRect(windowHandle, &winRect);

            if (!(winFlags & SWP_NOSIZE)) {
                event->type = bagE_EventWindowResize;
                event->data.windowResize.width = winRect.right - winRect.left;
                event->data.windowResize.height = winRect.bottom - winRect.top;
            }

            if (bagWIN32.cursorHidden)
                ClipCursor(&winRect);
        } break;

        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYUP: {
            event->type = message == WM_KEYDOWN || message == WM_SYSKEYDOWN
                        ? bagE_EventKeyDown : bagE_EventKeyUp;
            event->data.key.key = (unsigned)wParam;
            event->data.key.repeat = lParam & 0xFF;
        } break;

        case WM_MOUSEMOVE: {
            event->type = bagE_EventMousePosition;
            event->data.mouse.x = (signed short)(lParam & 0xFFFF);
            event->data.mouse.y = (signed short)(lParam >> 16);

            if (!bagWIN32.cursorInClient) {
                bagWIN32.cursorInClient = 1;

                bagWIN32_updateCursor();

                TRACKMOUSEEVENT trackMouseEvent = {0};
                trackMouseEvent.cbSize = sizeof(trackMouseEvent);
                trackMouseEvent.dwFlags = TME_LEAVE;
                trackMouseEvent.hwndTrack = bagWIN32.window;
                trackMouseEvent.dwHoverTime = HOVER_DEFAULT;

                if (!TrackMouseEvent(&trackMouseEvent)) {
                    bagWIN32_error("Failed start tracking the mouse events!");
                    exit(-1);
                }
            }
        } break;

        case WM_LBUTTONUP:
            event->data.mouseButton.button = bagE_ButtonLeft;
            goto buttonup;
        case WM_MBUTTONUP:
            event->data.mouseButton.button = bagE_ButtonMiddle;
            goto buttonup;
        case WM_RBUTTONUP:
            event->data.mouseButton.button = bagE_ButtonRight;
        buttonup:
            event->type = bagE_EventMouseButtonUp;
            goto buttonall;

        case WM_LBUTTONDOWN:
            event->data.mouseButton.button = bagE_ButtonLeft;
            goto buttondown;
        case WM_MBUTTONDOWN:
            event->data.mouseButton.button = bagE_ButtonMiddle;
            goto buttondown;
        case WM_RBUTTONDOWN:
            event->data.mouseButton.button = bagE_ButtonRight;
        buttondown:
            event->type = bagE_EventMouseButtonDown;

        buttonall:
            event->data.mouseButton.x = (signed short)(lParam & 0xFFFF);
            event->data.mouseButton.y = (signed short)(lParam >> 16);
            break;

        case WM_MOUSEWHEEL:
            event->type = bagE_EventMouseWheel;
            event->data.mouseWheel.scrollUp = ((signed short)(wParam >> 16)) / 120;
            event->data.mouseWheel.x = (signed short)(lParam & 0xFFFF);
            event->data.mouseWheel.y = (signed short)(lParam >> 16);
            break;

        case WM_INPUT: {
            unsigned int dataSize = 0;
            HRAWINPUT rawInput = (HRAWINPUT)lParam;
            RAWINPUT *data = NULL;

            GetRawInputData(rawInput, RID_INPUT, NULL,
                    &dataSize, sizeof(RAWINPUTHEADER));

            unsigned char *rawData = malloc(dataSize);
            if (GetRawInputData(
                    rawInput, RID_INPUT, rawData,
                    &dataSize, sizeof(RAWINPUTHEADER)) == (unsigned int)-1
            ) {
                free(rawData);
                break;
            }

            data = (RAWINPUT*)rawData;

            float dx, dy;
            int absMode = (data->data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE) == MOUSE_MOVE_ABSOLUTE;
            if (absMode) {
                int isVirt = (data->data.mouse.usFlags & MOUSE_VIRTUAL_DESKTOP) != 0;
                int width = GetSystemMetrics(isVirt ? SM_CXVIRTUALSCREEN : SM_CXSCREEN);
                int height = GetSystemMetrics(isVirt ? SM_CYVIRTUALSCREEN : SM_CYSCREEN);
                int x = (int)((data->data.mouse.lLastX / 65535.0f) * width);
                int y = (int)((data->data.mouse.lLastY / 65535.0f) * height);
                dx = (float)(x - bagWIN32.prevX);
                dy = (float)(y - bagWIN32.prevY);
                bagWIN32.prevX = x;
                bagWIN32.prevY = y;
            } else {
                dx = (float)data->data.mouse.lLastX;
                dy = (float)data->data.mouse.lLastY;
            }

            event->type = bagE_EventMouseMotion;
            event->data.mouseMotion.x = dx;
            event->data.mouseMotion.y = dy;

            free(rawData);
        } break;

        case WM_CHAR:
        case WM_SYSCHAR: {
            int utfVal = 0;

            if (bagWIN32.highSurrogate) {
                utfVal = (wParam & 0x3FF) | ((bagWIN32.highSurrogate & 0x3FF) << 10);
                bagWIN32.highSurrogate = 0;
            } else if ((int)wParam >= 0xD800 && (int)wParam <= 0xDBFF) {
                bagWIN32.highSurrogate = (int)wParam;
                break;
            } else {
                utfVal = (int)wParam;
            }

            event->type = bagE_EventTextUTF32;
            event->data.textUTF32.codePoint = utfVal;

            events[1].type = bagE_EventTextUTF8;
            int length = bagWIN32_utf8FromCodePoint(utfVal, events[1].data.textUTF8.text);
            events[1].data.textUTF8.text[length] = 0;
        } break;

        case WM_MOUSELEAVE: {
            bagWIN32.cursorInClient = 0;
        } break;

        default:
            result = DefWindowProcW(windowHandle, message, wParam, lParam);
    }

    for (int i = 0; i < BAGWIN32_EVENT_COUNT; i++) {
        if (events[i].type != bagE_EventNone) {
            if (bagE_eventHandler(events+i))
                bagWIN32.processingEvents = 0;
        }
    }

    return result;
}


void bagE_pollEvents()
{
    MSG message;

    bagWIN32.processingEvents = 1;

    while  (bagWIN32.processingEvents
         && PeekMessage(&message, bagWIN32.window, 0, 0, PM_REMOVE)
    ) {
        if (message.message == WM_QUIT) {
            bagE_Event event = { .type = bagE_EventWindowClose };

            if (bagE_eventHandler(&event))
                return;

            continue;
        }

        TranslateMessage(&message);
        DispatchMessage(&message);
    }
}


void bagE_swapBuffers()
{
    SwapBuffers(bagWIN32.deviceContext);
}


int bagE_getCursorPosition(int *x, int *y)
{
    POINT point;
    BOOL ret = GetCursorPos(&point);

    if (!ret)
        return 0;

    ret = ScreenToClient(bagWIN32.window, &point);

    if (!ret)
        return 0;

    *x = point.x;
    *y = point.y;

    return 1;
}


void bagE_getWindowSize(int *width, int *height)
{
    RECT rect;
    GetClientRect(bagWIN32.window, &rect);
    *width = rect.right - rect.left;
    *height = rect.bottom - rect.top;
}


int bagE_isAdaptiveVsyncAvailable(void)
{
    return bagWIN32.adaptiveVsync;
}


void bagE_setWindowTitle(char *value)
{
    SetWindowTextA(bagWIN32.window, value);
}


static void bagE_clipCursor(void)
{
        RECT windowRect;
        GetClientRect(bagWIN32.window, &windowRect);
        ClientToScreen(bagWIN32.window, (POINT*)&windowRect.left);
        ClientToScreen(bagWIN32.window, (POINT*)&windowRect.right);

// FIXME: this should be dealt with properly instead of whatever this is
#if 0
        windowRect.left += 1;
        windowRect.top += 1;
        windowRect.right -= 2;
        windowRect.bottom -= 2;
#else
        int width  = (windowRect.right  - windowRect.left) / 2;
        int height = (windowRect.bottom - windowRect.top)  / 2;
        windowRect.left   += width;
        windowRect.top    += height;
        windowRect.right  = windowRect.left;
        windowRect.bottom = windowRect.top;
#endif
        ClipCursor(&windowRect);
}


void bagE_setFullscreen(int value)
{
    DWORD style = GetWindowLong(bagWIN32.window, GWL_STYLE);
    int isOverlapped = style & WS_OVERLAPPEDWINDOW;

    if (isOverlapped && value) {
        MONITORINFO monitorInfo = { sizeof(monitorInfo) };

        if (GetWindowPlacement(bagWIN32.window, &bagWIN32.previousWP) &&
                GetMonitorInfo(
                        MonitorFromWindow(bagWIN32.window, MONITOR_DEFAULTTOPRIMARY),
                        &monitorInfo
                )
        ) {
            SetWindowLong(bagWIN32.window, GWL_STYLE, style & ~WS_OVERLAPPEDWINDOW);
            SetWindowPos(
                    bagWIN32.window,
                    HWND_TOP,
                    monitorInfo.rcMonitor.left,
                    monitorInfo.rcMonitor.top,
                    monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
                    monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
                    SWP_NOOWNERZORDER | SWP_FRAMECHANGED
            );
        }
    } else if (!(isOverlapped || value)) {
        SetWindowLong(bagWIN32.window, GWL_STYLE, style | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(bagWIN32.window, &bagWIN32.previousWP);
        SetWindowPos(
                bagWIN32.window,
                NULL, 0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                SWP_NOOWNERZORDER | SWP_FRAMECHANGED
        );
    }

    if (bagWIN32.cursorHidden)
        bagE_clipCursor();
}


int bagE_setHiddenCursor(int value)
{
    if (!bagWIN32.cursorHidden && value) {
        bagWIN32.cursorHidden = 1;
        bagE_clipCursor();
        SetCursor(NULL);
    } else if (bagWIN32.cursorHidden && !value) {
        bagWIN32.cursorHidden = 0;
        ClipCursor(NULL);
        SetCursor(LoadCursor(NULL, IDC_ARROW));
    }

    return 1;
}


void bagE_setCursorPosition(int x, int y)
{
    POINT point = { x, y };
    ClientToScreen(bagWIN32.window, &point);
    SetCursorPos(point.x, point.y);
}


void bagE_setSwapInterval(int value)
{
    if (bagWIN32.newContext)
        wglSwapIntervalEXT(value);
}

static inline LPSTR bagWIN32_convertCursor(bagE_Cursor cursor)
{
    switch (cursor) {
        case bagE_CursorDefault:   return IDC_ARROW;
        case bagE_CursorHandPoint: return IDC_HAND;
        case bagE_CursorMoveAll:   return IDC_SIZEALL;
        case bagE_CursorMoveHori:  return IDC_SIZEWE;
        case bagE_CursorMoveVert:  return IDC_SIZENS;
        case bagE_CursorWait:      return IDC_WAIT;
        case bagE_CursorWrite:     return IDC_IBEAM;
        case bagE_CursorCross:     return IDC_CROSS;

        default: return NULL;
    }
}


void bagE_setCursor(bagE_Cursor cursor)
{
    LPSTR id = bagWIN32_convertCursor(cursor);
    bagWIN32.cursor = LoadCursorA(NULL, id == NULL ? IDC_ARROW : id);
    bagWIN32_updateCursor();
}


