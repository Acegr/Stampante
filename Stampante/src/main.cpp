/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Carmine Foggia
   ======================================================================== */
#include <deque>
#include <stack>
#include <string.h>
#include <string>
#include <windows.h>
#include <windowsx.h>
#include <winuser.h>
#include <synchapi.h>
#include "..\res\resource.h"
#include "..\lib\lodepng.h"
#include "..\lib\files.h"

bool PortsConnected[100] = {};

enum InstructionTypeEnum : unsigned
{
    MoveUp, MoveDown, MoveRight, MoveLeft, MoveTo  
};

class Vector2D
{
public:
    int x;
    int y;
    Vector2D(int cx, int cy)
    {
        x = cx;
        y = cy;
    }
};

class Instruction
{
public:
    InstructionTypeEnum Type;
    Vector2D d;
};

std::deque<Instruction> Path;

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
HANDLE LogFileHandle;
bool SerialListInitialised = false;

void CheckActivePorts()
{
    for(int i = 1; i <= 99; i++)
    {
        char PortName[] = "\\\\.\\COM\0\0";
        sprintf((char * const)(PortName+7), "%d", i);
        HANDLE Port = CreateFile(PortName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if(Port != INVALID_HANDLE_VALUE)
        {
            PortsConnected[i] = true;
            CloseHandle(Port);
        }
    }
}

/*This function converts RGB to black and white: it does this by treating colors as points in 3d-space and assigning to each colour the nearest
 *between black (0, 0, 0) and white (255, 255, 255)
 */

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

//Our implementation of depth-first search. We do not use the OS's stack because it would overflow
void DFSRecursion(unsigned* Input, unsigned* PixelStatus, unsigned w, unsigned h, std::deque<Instruction>& Output, unsigned x, unsigned y)
{
    bool doReturn = false;
    std::stack<Vector2D> DFSStack {};
    unsigned CurrentX = x;
    unsigned CurrentY = y;
    Output.push_back({MoveTo, Vector2D(CurrentX, CurrentY)});
    Vector2D CurrentPoint(CurrentX, CurrentY);
    while(true)
    {
        //We first look down, then right, then up, then left, then try to pop the stack, if there's no stack we return
        Vector2D NextPoint(CurrentPoint.x, CurrentPoint.y);
        PixelStatus[CurrentPoint.y*w+CurrentPoint.x] = 2;
        if((CurrentPoint.y < h-1) && (PixelStatus[(CurrentPoint.y+1)*w+CurrentPoint.x] == 1))
        {
            DFSStack.push(Vector2D(CurrentPoint.x, CurrentPoint.y+1));
        }
        if((CurrentPoint.x < w-1) && (PixelStatus[CurrentPoint.y*w+CurrentPoint.x+1] == 1))
        {
            DFSStack.push(Vector2D(CurrentPoint.x+1, CurrentPoint.y));
        }
        if((CurrentPoint.y > 0) && (PixelStatus[(CurrentPoint.y-1)*w+CurrentPoint.x] == 1))
        {
            DFSStack.push(Vector2D(CurrentPoint.x, CurrentPoint.y-1));
        }
        if((CurrentPoint.x > 0) && (PixelStatus[CurrentPoint.y*w+CurrentPoint.x-1] == 1))
        {
            DFSStack.push(Vector2D(CurrentPoint.x-1, CurrentPoint.y));
        }
        while(PixelStatus[NextPoint.y*w+NextPoint.x] == 2 && DFSStack.size() != 0)
        {
            NextPoint = DFSStack.top();
            DFSStack.pop();
        }
        if(DFSStack.size() == 0)
        {
            return;
        }
        if(NextPoint.y == CurrentPoint.y)
        {
            if((NextPoint.x - CurrentPoint.x) == 1)
            {
                Output.push_back({MoveRight, Vector2D(0, 0)});
            }
            else if((NextPoint.x - CurrentPoint.x) == -1)
            {
                Output.push_back({MoveLeft, Vector2D(0, 0)});
            }
            else
            {
                Output.push_back({MoveTo, NextPoint});
            }
        }
        else if(NextPoint.x == CurrentPoint.x)
        {
            if((NextPoint.y - CurrentPoint.y) == 1)
            {
                Output.push_back({MoveDown, Vector2D(0, 0)});
            }
            else if((NextPoint.y - CurrentPoint.y) == -1)
            {
                Output.push_back({MoveUp, Vector2D(0, 0)});
            }
            else
            {
                Output.push_back({MoveTo, NextPoint});
            }
        }
        else
        {
            Output.push_back({MoveTo, NextPoint});
        }
        CurrentPoint = NextPoint;
    }
}

/*Our pathfinding algorithm. The way it works:
 *We run depth-first search on every black pixel unless it is already on the plotter's path
 *We keep track of the instructions on a deque, so that we can both add them from the end (when running the algorithm)
 *and read them from the beginning (when printing)*/
void DFSPathFinding(unsigned* Input, unsigned w, unsigned h, std::deque<Instruction>& Output)
{
    unsigned *PixelStatus = new unsigned[w*h]; //An array of the same size as the image. Every position is 0 if the corresponding pixel
                                               //is not coloured, 1 if it is but hasn't been found yet by the pathfinding, 2 if it has
    for(int y = 0; y < h; y++)
    {
        for(int x = 0; x < w; x++)
        {
            if(Input[y*w+x] == 0)
            {
                PixelStatus[y*w+x] = 1;
            }
        }
    }
    for(int y = 0; y < h; y++)
    {
        for(int x = 0; x < w; x++)
        {
            if(PixelStatus[y*w+x] == 1)
            {
                DFSRecursion(Input, PixelStatus, w, h, Output, x, y);
            }
        }
    }
    delete PixelStatus;
}

void LogInstruction(Instruction instr)
{
    char *map[] = {"U", "D", "R", "L", "T"};
    WriteToLog(map[instr.Type], LogFileHandle);
    if(instr.Type == MoveTo)
    {
        WriteToLog(" ", LogFileHandle);
        WriteToLog(std::to_string(instr.d.x).c_str(), LogFileHandle);
        WriteToLog(",", LogFileHandle);
        WriteToLog(std::to_string(instr.d.y).c_str(), LogFileHandle);
    }
    WriteToLog("\n", LogFileHandle);
}

void LogPathFinding()
{
    for(auto it = Path.cbegin(); it != Path.cend(); it++)
    {
        LogInstruction(*it);
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
            SerialListInitialised = false;
            return TRUE;
            break;
        }
        case WM_INITDIALOG:
        {
            SendDialog = hWnd;
            HWND ComboBoxHandle = GetDlgItem(hWnd, IDC_COMBO1);
            HWND EditBoxHandle = GetDlgItem(hWnd, IDC_EDIT1);
            CheckActivePorts();
            for(int i = 1; i <= 99; i++)
            {
                if(PortsConnected[i])
                {
                    char PortName[] = "COM\0\0";
                    sprintf((char * const)(PortName+3), "%d", i);
                    SendMessage(ComboBoxHandle, CB_ADDSTRING, 0,(LPARAM)PortName);        
                }
            }
            if(ComboBox_GetCount(ComboBoxHandle) > 0)
            {
                SendMessage(ComboBoxHandle, CB_SETCURSEL, 0, 0);
                Edit_SetText(EditBoxHandle, "9600");
                SerialListInitialised = true;
            }
            return TRUE;
            break;
        }
        case WM_COMMAND:
        {
            if(!(wParam & 0xFFFF0000))
            {
                switch(wParam & 0xFFFF)
                {
                    case IDC_BUTTON1:
                    {
                        if(SerialListInitialised)
                        {
                            unsigned PortID = SendMessage(GetDlgItem(hWnd, IDC_COMBO1), CB_GETCURSEL, 0, 0);
                            unsigned PortNumber = 0;
                            for(int i = 1; i <= 99; i++)
                            {
                                if(PortsConnected[i] == true)
                                {
                                    if(PortID == 0) PortNumber = i;
                                    PortID--;
                                }
                            }
                            char PortName[] = "\\\\.\\COM\0\0";
                            sprintf((char * const)(PortName+7), "%d", PortNumber);
                            HANDLE Port = CreateFile(PortName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
                            if(Port == INVALID_HANDLE_VALUE) return FALSE;
                            unsigned BytesWritten;
                            WriteFile(Port, "T", 1, (LPDWORD)&BytesWritten, NULL);
                            CloseHandle(Port);
                        }
                    }
                    default:
                    {
                        return CallWindowProc(DefWindowProc, hWnd, uMsg, wParam, lParam);
                        break;
                    }
                }
            }
            else
            {
                return CallWindowProc(DefWindowProc, hWnd, uMsg, wParam, lParam);
            }
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
//We cconvert from the library's format (24-bit RGB) to windows' (32-bit 0RGB)
                            for(int i = 0; i < Image.h; i++)
                            {
                                for(int j = 0; j < Image.w; j++)
                                {
                                    Image.Pixels[i*Image.w+j] =
                                        ((ImageRaw[3*(i*Image.w+j)] << 16) | (ImageRaw[3*(i*Image.w+j)+1] << 8) | ImageRaw[3*(i*Image.w+j)+2]);
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
                        Path.clear();
                        Log("Starting greyscale conversion", LogFileHandle);
                        GreyScaleEuclideanNorm(Image.Pixels, Image.Pixels, Image.w, Image.h);
                        Log("Greyscale conversion completed. Starting pathfinding", LogFileHandle);
                        DFSPathFinding(Image.Pixels, Image.w, Image.h, Path);
                        LogPathFinding();
                        Log("Pathfinding completed", LogFileHandle);
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
    if(!CreateLogFile(&LogFileHandle))
    {
        return FALSE;
    }
    
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
