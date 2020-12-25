/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Carmine Foggia
   ======================================================================== */
#include "..\..\lib\lodepng.h"
#include <windows.h>

bool Quit = false;
char Filename[256] = "..\\..\\images\\walls of canterbury.png";
HDC WindowDC;
HWND Window;

struct ImageClass
{
    unsigned *Pixels;
    unsigned w;
    unsigned h;
    BITMAPINFO Info;
};

ImageClass Image;

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
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            BeginPaint(hWnd, &ps);
            RECT ClientRect;
            GetClientRect(hWnd, &ClientRect);
            int a = StretchDIBits(WindowDC,
                          0, 0,
                          ClientRect.right - ClientRect.left,
                          ClientRect.bottom - ClientRect.top,
                          0, 0,
                          Image.w, Image.h, Image.Pixels, &Image.Info,
                          DIB_RGB_COLORS, SRCCOPY);
            EndPaint(hWnd, &ps);
            return 0;
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

    unsigned char *ImageRaw;
    unsigned wRaw;
    unsigned hRaw;
    int a = lodepng_decode24_file(&ImageRaw, &wRaw, &hRaw, Filename);
    Image.w = wRaw;
    Image.h = hRaw;
    Image.Pixels = new unsigned[Image.w * Image.h];
    for(int i = 0; i < Image.h; i++)
    {
        for(int j = 0; j < Image.w; j++)
        {
            Image.Pixels[i*Image.w+j] = ((ImageRaw[3*(i*Image.w+j)] << 16) | (ImageRaw[3*(i*Image.w+j)+1] << 8) | ImageRaw[3*(i*Image.w+j)+2]);
        }
    }

    Image.Info.bmiHeader.biSize = sizeof(Image.Info.bmiHeader);
    Image.Info.bmiHeader.biWidth = Image.w;
    Image.Info.bmiHeader.biHeight = -Image.h;
    Image.Info.bmiHeader.biPlanes = 1;
    Image.Info.bmiHeader.biBitCount = 32;
    Image.Info.bmiHeader.biCompression = BI_RGB;
    
    WNDCLASS WindowClass {};
    WindowClass.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    WindowClass.lpfnWndProc   = MainWindowCallback;
    WindowClass.cbClsExtra    = 0;
    WindowClass.cbWndExtra    = 0;
    WindowClass.hInstance     = hInstance;
    WindowClass.hCursor       = LoadCursor(0, MAKEINTRESOURCE(IDC_ARROW));
    WindowClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    WindowClass.lpszClassName = "PrinterWindowClass";
    RegisterClass(&WindowClass);
    
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
    WindowDC = GetDC(Window);
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
    ReleaseDC(Window, WindowDC);
    return TRUE;
}

