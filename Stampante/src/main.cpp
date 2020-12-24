/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Carmine Foggia
   ======================================================================== */
#include "..\res\resource.h"
#include <windows.h>
#include <winuser.h>
#include <synchapi.h>

bool Quit = false;
HWND Window;
HWND SendDialog;
bool isSendOpen = false;
char Image[256];

LRESULT CALLBACK StatusCallback
(HWND   hWnd,
 UINT   uMsg,
 WPARAM wParam,
 LPARAM lParam)
{
    switch(uMsg)
    {
        default:
        {
            return CallWindowProc(DefWindowProc, hWnd, uMsg, wParam, lParam);
        }
    }
}

INT_PTR SendDialogProc
(HWND   hWnd,
 UINT   uMsg,
 WPARAM wParam,
 LPARAM lParam)
{
    switch(uMsg)
    {
        case WM_CLOSE:
        case WM_DESTROY:
        case WM_QUIT:
        {
            DestroyWindow(hWnd);
            isSendOpen = false;
            SendDialog = NULL;
            return TRUE;
            break;
        }
        case WM_INITDIALOG:
        {
            SendDialog = hWnd;
            return TRUE;
            break;
        }
        case WM_DRAWITEM:
        {
            return CallWindowProc(DefWindowProc, hWnd, uMsg, wParam, lParam);
            break;
        }
        default:
        {
            return FALSE;
        }
    }
}

LRESULT CALLBACK MainWindowCallback
(HWND   hWnd,
 UINT   uMsg,
 WPARAM wParam,
 LPARAM lParam)
{
    switch(uMsg)
    {
        case WM_CLOSE:
        case WM_DESTROY:
        case WM_QUIT:
        {
            Quit = true;
            return 0;
            break;
        }
        
        case WM_COMMAND:
        {
            if(!(wParam & 0xFFFF0000))
            {
                switch(wParam & 0xFFFF)
                {
                    case ID_IMAGE:
                    {
                        OPENFILENAME OpenFile {};
                        OpenFile.lStructSize = sizeof(OPENFILENAME);
                        OpenFile.hwndOwner = hWnd;
                        OpenFile.hInstance = GetModuleHandle(0);
                        OpenFile.lpstrFilter = TEXT("PNG images\0*.png\0\0");
                        OpenFile.lpstrFile = Image;
                        OpenFile.nMaxFile = 255;
                        OpenFile.Flags = OFN_FILEMUSTEXIST;
                        OpenFile.lpstrDefExt = TEXT("png");
                        GetOpenFileName(&OpenFile);
                        return 0;
                        break;
                    }
                    case ID_SEND:
                    {
                        if(!isSendOpen)
                        {
                            CreateDialog(GetModuleHandle(NULL),
                                                      MAKEINTRESOURCE(IDD_SENDDIALOG),
                                                      hWnd, SendDialogProc);
                                ShowWindow(SendDialog, SW_SHOW);
                                isSendOpen = true;
                        }
                        return 0;
                        break;
                    }
                    default:
                    {
                        return CallWindowProc(DefWindowProc, hWnd, uMsg, wParam, lParam);
                    }
                }
            }
            else
            {
                return CallWindowProc(DefWindowProc, hWnd, uMsg, wParam, lParam);
            }
            break;
        }
        
        default:
        {
            return CallWindowProc(DefWindowProc, hWnd, uMsg, wParam, lParam);
        }
    }
}

int CALLBACK WinMain
(HINSTANCE hInstance,
 HINSTANCE hPrevInstance,
 LPSTR     lpCmdLine,
 int       nCmdShow)
{
    WNDCLASS WindowClass {};
    WindowClass.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    WindowClass.lpfnWndProc   = MainWindowCallback;
    WindowClass.cbClsExtra    = 0;
    WindowClass.cbWndExtra    = 0;
    WindowClass.hInstance     = hInstance;
    WindowClass.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(TOOLBAR_ICON));
    WindowClass.hCursor       = LoadCursor(0, MAKEINTRESOURCE(IDC_ARROW));
    WindowClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    WindowClass.lpszMenuName  = MAKEINTRESOURCE(IDR_MAINMENU);
    WindowClass.lpszClassName = "PrinterWindowClass";
    RegisterClass(&WindowClass);

    WNDCLASS StatusClass {};
    StatusClass.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    StatusClass.lpfnWndProc   = StatusCallback;
    StatusClass.cbClsExtra    = 0;
    StatusClass.cbWndExtra    = 0;
    StatusClass.hInstance     = hInstance;
    StatusClass.hIcon         = NULL;
    StatusClass.hCursor       = LoadCursor(0, MAKEINTRESOURCE(IDC_ARROW));
    StatusClass.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
    StatusClass.lpszMenuName  = NULL;
    StatusClass.lpszClassName = "StatusClass";
    RegisterClass(&StatusClass);
    
    Window = CreateWindow("PrinterWindowClass",
                          "La stampante pazza sgravata",
                          WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                          CW_USEDEFAULT,
                          CW_USEDEFAULT,
                          CW_USEDEFAULT,
                          CW_USEDEFAULT,
                          NULL,
                          NULL,
                          hInstance,
                          NULL);
    while(!Quit)
    {
        MSG Message;
        while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&Message);
            DispatchMessage(&Message);
        }
        timeBeginPeriod(1);
        Sleep(16);
        timeEndPeriod(1);
    }
    return TRUE;
}
