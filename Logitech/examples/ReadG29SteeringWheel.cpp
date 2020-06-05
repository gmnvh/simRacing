#ifndef UNICODE
#define UNICODE
#endif 

#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <iostream>

/* Include Logitech library */
#pragma comment(lib, "LogitechSteeringWheelLib.lib")
#include "LogitechSteeringWheelLib.h"

/* Globals variables */
HWND mainWindow;        /* Handle for the main window - the main window needs to be on focus to read the wheel status */

HWND hlInit;                   /* Label to indicate the Logitech initialization results */
HWND hlJoy0;                   /* Label to show information about joystick 0 - reads every 10 ms*/
HWND hlJoy1;                   /* Label to show information about joystick 1 - reads every 10 ms*/
HWND hlThreadRunning;          /* Label to indicate if the working thread is running */

bool threadRunning;            /* Indicates if the working thread (the one that reads the steering wheel) should run (TRUE) or not (FALSE) */
DWORD threadId;                /* Working thread ID - returned by CreateThread */
HANDLE threadHandle = NULL;    /* Working thread handle - returned by CreateThread */


/* Local prototypes */
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void createLabels(HWND hwnd);
void createButtons(HWND hwnd);
void joyInit(HWND hwnd);
void threadInit();
void threadClose();

#define MSG_SIZE 500

/*************************************************************************************/
/*
 * Win32App
 *
 */

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR pCmdLine, int nCmdShow)
{
    // Register the window class.
    const wchar_t CLASS_NAME[] = L"Button_Example_01";

    WNDCLASS wc = { };

    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    // Create the window.

    mainWindow = CreateWindowEx(
        0,                              // Optional window styles.
        CLASS_NAME,                     // Window class
        L"Logictech G29 Example",       // Window text
        WS_OVERLAPPEDWINDOW,            // Window style

        // Size and position - X, Y, nWidth, nHeight
        CW_USEDEFAULT, CW_USEDEFAULT, 600, 350,

        NULL,       // Parent window    
        NULL,       // Menu
        hInstance,  // Instance handle
        NULL        // Additional application data
    );

    if (mainWindow == NULL)
    {
        return 0;
    }

    ShowWindow(mainWindow, nCmdShow);

    // Run the message loop.

    MSG msg = { };
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_DESTROY:
        threadClose();
        PostQuitMessage(0);
        return 0;

    case WM_CREATE:
        createLabels(hwnd);
        createButtons(hwnd);
        return 0;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
        EndPaint(hwnd, &ps);
    }
    return 0;

    case WM_COMMAND:
        switch (wParam)
        {
        case 1: /* Button was pressed */
            threadInit();
            break;
        }

        return 0;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void createLabels(HWND hwnd)
{
    /* Label Init Info */
    hlInit = CreateWindowW(L"Static", L"--",
        WS_VISIBLE | WS_CHILD | WS_BORDER | SS_LEFT,
        50, 50, 500, 150, hwnd, NULL, NULL, NULL);

    /* Label thread running */
    hlThreadRunning = CreateWindowW(L"Static", L"--",
        WS_VISIBLE | WS_CHILD | WS_BORDER | SS_CENTER,
        370, 10, 120, 30, hwnd, NULL, NULL, NULL);

    /* Info about joystick 0 */
    hlJoy0 = CreateWindowW(L"Static", L"--",
        WS_VISIBLE | WS_CHILD | WS_BORDER | SS_CENTER,
        120, 220, 150, 50, hwnd, NULL, NULL, NULL);

    /* Info about joystick 1 */
    hlJoy1 = CreateWindowW(L"Static", L"--",
        WS_VISIBLE | WS_CHILD | WS_BORDER | SS_CENTER,
        320, 220, 150, 50, hwnd, NULL, NULL, NULL);
}

void createButtons(HWND hwnd)
{
    /* Button to connect to the joystick */
    CreateWindowW(L"Button", L"Connect",
        WS_VISIBLE | WS_CHILD,
        250, 10, 100, 30, hwnd, (HMENU)1, NULL, NULL);
}


/*************************************************************************************/
/*
 * Working thread functions
 *
 * The working thread is created when the button Init is pressed and is responsible
 * to initialize and perodicaly read the steering wheel.
 */


/*
 * threadMain
 *
 * Working thread loop
 */
DWORD WINAPI threadMain(LPVOID lpParam)
{
    wchar_t msg[MSG_SIZE];
    DIJOYSTATE2ENGINES* joy;

    SetWindowTextW(hlThreadRunning, L"Running");
    joyInit(mainWindow);

    while (threadRunning)
    {
        if (LogiUpdate()) {
            joy = LogiGetStateENGINES(0);
            swprintf(msg, MSG_SIZE, L"x-axis 1: %d\r\nAccel: %d\r\nBrake: %d", joy->lX, joy->lY, joy->lRz);
            SetWindowTextW(hlJoy0, msg);

            joy = LogiGetStateENGINES(1);
            swprintf(msg, MSG_SIZE, L"x-axis 1: %d\r\nAccel: %d\r\nBrake: %d", joy->lX, joy->lY, joy->lRz);
            SetWindowTextW(hlJoy1, msg);
            Sleep(20);
        }
    }

    return 0;
}

void threadInit()
{
    threadRunning = true;

    /* Make sure thread is initialized just once */
    if (threadHandle == NULL)
    {
        threadHandle = CreateThread(0, 0, threadMain, 0, 0, &threadId);
    }
}

void threadClose()
{
    threadRunning = false;
    CloseHandle(threadHandle);
}

/*
 * joyInit
 *
 * Initialize the Logitech driver and check what device is connected.
 */
void joyInit(HWND hwnd)
{
    bool rsp;
    wchar_t msg[MSG_SIZE];
    int cx;

    rsp = LogiSteeringInitializeWithWindow(FALSE, hwnd);
    cx = swprintf(msg, MSG_SIZE, L"Init: %d\r\n", rsp);

    if (rsp == true)
    {
        /* Checks if a game controller is connected at the specified index */
        rsp = LogiIsConnected(0);
        cx += swprintf(msg + cx, MSG_SIZE - cx - 1, L"Connected 0: %d\r\n", rsp);

        rsp = LogiIsConnected(1);
        cx += swprintf(msg + cx, MSG_SIZE - cx - 1, L"Connected 1: %d\r\n", rsp);

        /* Get devince name */
        TCHAR buf[128] = { '\0' };
        LogiGetFriendlyProductName(0, buf, 128);
        cx += swprintf(msg + cx, MSG_SIZE - cx - 1, L"Device 0: %s\r\n", buf);

        LogiGetFriendlyProductName(1, buf, 128);
        cx += swprintf(msg + cx, MSG_SIZE - cx - 1, L"Device 1: %s\r\n", buf);

        /* Check type of device connected */
        rsp = LogiIsDeviceConnected(0, LOGI_DEVICE_TYPE_WHEEL);
        cx += swprintf(msg + cx, MSG_SIZE - cx - 1, L"Is device 0 a wheel? %d\r\n", rsp);

        rsp = LogiIsDeviceConnected(1, LOGI_DEVICE_TYPE_WHEEL);
        cx += swprintf(msg + cx, MSG_SIZE - cx - 1, L"Is device 1 a wheel? %d\r\n", rsp);
    }
    else
    {
        cx += swprintf(msg + cx, MSG_SIZE - cx - 1, L"Something failed\r\n");
    }

    SetWindowTextW(hlInit, msg);
}