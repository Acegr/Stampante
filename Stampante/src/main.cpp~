/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Carmine Foggia
   ======================================================================== */
#include "..\res\resource.h"
#include "..\lib\lodepng.h"
#include <windows.h>
#include <winuser.h>
#include <synchapi.h>

struct ImageClass
{
    char FileName[256];
    unsigned *Pixels;
    unsigned w;
    unsigned h;
    BITMAPINFO Info;
};

bool Quit = false;
HWND Window;
HDC MainDC;
HWND SendDialog;
bool isSendOpen = false;
ImageClass Image;
bool isAnImageLoaded = false;

void GreyScaleEuclideanNorm(unsigned *input, unsigned* output, unsigned w, unsigned h)
{

    for(int y = 0; y < h; y++)
    {
        for(int x = 0; x < w; x++)
        {
            unsigned currentPixel = input[y*w+x];
            unsigned R = currentPixel & 0xFF;
            unsigned B = (currentPixel & 0xFF00) >> 8;
            unsigned G = (currentPixel & 0xFF0000) >> 16;
            unsigned NormToBlack = sqrt(pow(R,2) + pow(B,2) + pow(G,2));
            unsigned NormToWhite = sqrt(pow(255-R,2) + pow(255-B,2) + pow(255-G,2));
            if(NormToWhite < NormToBlack)
            {
                output[y*w+x] = 0xFFFFFF;
            }
            else
            {
                output[y*w+x] = 0;
            }
        }
    }
}

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
                        OpenFile.lpstrFile = Image.FileName;
                        OpenFile.nMaxFile = 255;
                        OpenFile.Flags = OFN_FILEMUSTEXIST;
                        OpenFile.lpstrDefExt = TEXT("png");
                        bool isCancelled = !GetOpenFileName(&OpenFile);
                        if(isAnImageLoaded && !isCancelled)
                        {
                            delete Image.Pixels;
                        }

                        if(!isCancelled)
                        {
                            isAnImageLoaded = true;
                            unsigned char *ImageRaw;
                            unsigned wRaw;
                            unsigned hRaw;
                            lodepng_decode24_file(&ImageRaw, &wRaw,
                                                  &hRaw, Image.FileName);
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
                        }
                        return 0;
                        break;
                    }
                    case ID_PROCESS:
                    {
                        GreyScaleEuclideanNorm(Image.Pixels, Image.Pixels, Image.w, Image.h);
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

        case WM_PAINT:
        {
            if(isAnImageLoaded)
            {
                PAINTSTRUCT ps;
                BeginPaint(hWnd, &ps);
                RECT ClientRect;
                GetClientRect(hWnd, &ClientRect);
                StretchDIBits(MainDC,
                              0, 0,
                              ClientRect.right - ClientRect.left,
                              ClientRect.bottom - ClientRect.top,
                              0, 0,
                              Image.w, Image.h, Image.Pixels, &Image.Info,
                              DIB_RGB_COLORS, SRCCOPY);
                EndPaint(hWnd, &ps);
                return 0;
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
    MainDC = GetDC(Window);
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
    ReleaseDC(Window, MainDC);
    return TRUE;
}
