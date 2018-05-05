/**
Copyright (c) 2018 fy

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

   3. This notice may not be removed or altered from any source
   distribution.

编译参数：
    g++ -m32 -lcomctl32 mb_simple_demo.cpp
    cl mb_simple_demo.cpp
*/

#include <Windows.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <stdbool.h>
#include <stdio.h>
#include <Commctrl.h>
#include <windowsx.h>
#include "wkedefine.h"

#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "comctl32.lib")


typedef struct {
    wkeWebView window;
    WCHAR url[MAX_PATH + 1];
} Application;

Application app;

// 回调：点击了关闭、返回 true 将销毁窗口，返回 false 什么都不做。
bool HandleWindowClosing(wkeWebView webWindow, void* param) {
    Application* app = (Application*)param;
    return IDYES == MessageBoxW(NULL, L"确定要退出程序吗？", L"wkexe", MB_YESNO | MB_ICONQUESTION);
}

// 回调：窗口已销毁
void HandleWindowDestroy(wkeWebView webWindow, void* param) {
    Application* app = (Application*)param;
    app->window = NULL;
    PostQuitMessage(0);
}

// 回调：文档加载成功
void HandleDocumentReady(wkeWebView webWindow, void* param) {
    wkeShowWindow(webWindow, true);
}

// 回调：页面标题改变
void HandleTitleChanged(wkeWebView webWindow, void* param, const wkeString title) {
    wkeSetWindowTitleW(webWindow, wkeGetStringW(title));
}

// 回调：创建新的页面，比如说调用了 window.open 或者点击了 <a target="_blank" .../>
wkeWebView HandleCreateView(wkeWebView webWindow, void* param, wkeNavigationType navType, const wkeString url, const wkeWindowFeatures* features) {
    wkeWebView newWindow = wkeCreateWebWindow(WKE_WINDOW_TYPE_POPUP, NULL, features->x, features->y, features->width, features->height);
    wkeShowWindow(newWindow, true);
    return newWindow;
}

bool HandleLoadUrlBegin(wkeWebView webView, void* param, const char *url, void *job) {
    if (strcmp(url, "http://hook.test/") == 0) {
        wkeNetSetMIMEType(job, (char*)"text/html");
        wkeNetSetURL(job, url);
        wkeNetSetData(job, (char*)"<li>这是个hook页面</li><a herf=\"http://www.baidu.com/\">HookRequest</a>", 
            sizeof("<li>这是个hook页面</li><a herf=\"http://www.baidu.com/\">HookRequest</a>"));
        return true;
    } else if (strcmp(url, "http://www.baidu.com/") == 0) {
        wkeNetHookRequest(job);
    }
    return false;
}

void HandleLoadUrlEnd(wkeWebView webView, void* param, const char *url, void *job, void* buf, int len) {
    const wchar_t *str = L"百度一下";
    const wchar_t *str1 = L"HOOK一下";

    int slen = ::WideCharToMultiByte(CP_UTF8, 0, str, -1, NULL, 0, NULL, NULL);
    if (slen == 0) return;

    char utf81[100];
    ::WideCharToMultiByte(CP_UTF8, 0, str, -1, &utf81[0], slen, NULL, NULL);

    slen = ::WideCharToMultiByte(CP_UTF8, 0, str1, -1, NULL, 0, NULL, NULL);
    if (slen == 0) return;

    char utf82[100];
    ::WideCharToMultiByte(CP_UTF8, 0, str1, -1, &utf82[0], slen, NULL, NULL);

    const char *b = strstr(static_cast<const char*>(buf), &utf81[0]);
    memcpy((void *)b, &utf82, slen);
    return;
}

void BlinkMaximize() {
    HWND hwnd = wkeGetWindowHandle(app.window);
    SendMessage(hwnd, WM_SYSCOMMAND, WS_MAXIMIZE, 0);
}

void BlinkMinimize() {
    HWND hwnd = wkeGetWindowHandle(app.window);
    SendMessage(hwnd, WM_SYSCOMMAND, WS_MINIMIZE, 0);
}

HWND GetHWND() {
    return wkeGetWindowHandle(app.window);
}

// 互相调用：CPP调用JS
const char* CallJSFunc(jsExecState es, char *funcName, char *param) {
    jsValue f = jsGetGlobal(es, funcName);
    jsValue val = jsString(es, param);
    jsValue callRet = jsCallGlobal(es, f, &val, 1);
    return jsToString(es, callRet);
}

// 互相调用：JS调用CPP
jsValue JS_CALL js_msgBox(jsExecState es) {
    const wchar_t* text = jsToStringW(es, jsArg(es, 0));
    const wchar_t* title = jsToStringW(es, jsArg(es, 1));
    MessageBoxW(NULL, text, title, 0);

    //return jsUndefined();
    //return jsInt(1234);
    return jsStringW(es, L"C++返回字符串");
}

void setAppIcon(HWND hwnd) {
    wchar_t szFilePath[MAX_PATH];
    if (GetModuleFileNameW(NULL, szFilePath, MAX_PATH)) {
        HICON iconLarge, iconSmall;
        UINT nIcons = ExtractIconExW((LPCWSTR)&szFilePath, -1, NULL, NULL, 0);

        ExtractIconExW((LPCWSTR)&szFilePath, 0, &iconLarge, &iconSmall, nIcons);
        SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)iconLarge);
        SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)iconSmall);
    }
}

// 以下设置窗口拖动区域
static bool is_moving = false;
static RECT move_rect = {0};

void SetMoveWindowArea(int x, int y, int w, int h) {
    move_rect.left = x;
    move_rect.top = y;
    move_rect.right = x + w;
    move_rect.bottom = y + h;
	is_moving = true;
}

LRESULT CALLBACK SubClassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    if (is_moving && (uMsg == WM_NCHITTEST)) {
        POINT pt = {(LONG)GET_X_LPARAM(lParam), (LONG)GET_Y_LPARAM(lParam)};
        ScreenToClient(hWnd, &pt);
        // printf("pt: %d %d\n", pt.x, pt.y);
        // printf("rect: %d %d %d %d\n", move_rect.left, move_rect.top, move_rect.right, move_rect.bottom);
        if (PtInRect(&move_rect, pt)) {
            return HTCAPTION;
        }
    }
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}
// -

// 创建主页面窗口
bool CreateWebWindow(Application* app) {
    // app->window = wkeCreateWebWindow(WKE_WINDOW_TYPE_TRANSPARENT, NULL, 0, 0, 640, 480);  // 无边框窗体 borderless window
    app->window = wkeCreateWebWindow(WKE_WINDOW_TYPE_POPUP, NULL, 0, 0, 640, 480);

    if (!app->window)
        return false;

    SetWindowSubclass(wkeGetWindowHandle(app->window), SubClassProc, 0, 0);
    SetMoveWindowArea(0, 0, 640, 30); // 设置窗口可拖动区域，用于无边框窗体

    wkeSetWindowTitleW(app->window, L"这个API设置窗口标题，不过后面有个 callback 会把这里的设置覆盖掉。");
    wkeOnWindowClosing(app->window, HandleWindowClosing, app);
    wkeOnWindowDestroy(app->window, HandleWindowDestroy, app);
    wkeOnDocumentReady(app->window, HandleDocumentReady, app);
    wkeOnTitleChanged(app->window, HandleTitleChanged, app);
    wkeOnCreateView(app->window, HandleCreateView, app);
    wkeOnLoadUrlBegin(app->window, HandleLoadUrlBegin, app);
    wkeOnLoadUrlEnd(app->window, HandleLoadUrlEnd, app);

    jsBindFunction("msgBox", &js_msgBox, 2);
    wkeMoveToCenter(app->window);
    wkeLoadURLW(app->window, app->url);

    return true;
}

void PrintHelpAndQuit(Application* app) {
    PostQuitMessage(0);
}

void RunMessageLoop(Application* app) {
    MSG msg = { 0 };
    setAppIcon(GetHWND());
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

void QuitApplication(Application* app) {
    if (app->window) {
        wkeDestroyWebWindow(app->window);
        app->window = NULL;
    }
}

void RunApplication(Application* app) {
    memset(app, 0, sizeof(Application));

#ifdef _MSC_VER
    wcsncpy_s(app->url, MAX_PATH, L"http://www.baidu.com", MAX_PATH);
#else
    wcsncpy(app->url, L"http://www.baidu.com", MAX_PATH);
#endif

    if (!CreateWebWindow(app)) {
        PrintHelpAndQuit(app);
        return;
    }

    RunMessageLoop(app);
}

bool TestOneInstance() {
    HANDLE mutex = CreateMutex(NULL, TRUE, "Hello Miniblink");
    if ((mutex != NULL) && (GetLastError() == ERROR_ALREADY_EXISTS)) {
        ReleaseMutex(mutex);
        return false;
    }
    return true;
}

int WINAPI WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nShowCmd
) {
    if (!TestOneInstance()) {
        MessageBoxW(NULL, L"该进程已经启动", L"错误", MB_OK);
        return 0;
    }
    wkeInitialize();
    RunApplication(&app);
    wkeFinalize();
    return 0;
}
