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
#include <thread>
#include "..\res\resource.h"
#include "..\lib\lodepng.h"
#include "..\lib\files.h"


const int PaperMaxX = 47660; //as measured by calibrating the printer for an A4 sheet
const int PaperMaxY = 33700; //

bool PortsConnected[100] = {};

enum InstructionTypeEnum : unsigned
{
    MoveUp = 0, MoveDown = 1, MoveRight = 2, MoveLeft = 3, NoOp = 4, MoveTo  
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
    Instruction(InstructionTypeEnum t, Vector2D v):
            Type(t), d(v)
    {
    }
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

char ToHex[] = {'0', '1', '2', '3', '4', '5', '6', '7',
                '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'}; //We are lazy and do not want to come up with better ways to convert

char InstructionLetters[] = {'U', 'D', 'R', 'L', 'N'};

bool Quit = false;
HWND Window;
HDC MainDC;
HWND SendDialog;
bool isSendOpen = false;
ImageClass Image;
bool isAnImageLoaded = false;
bool Processed = false;
HANDLE LogFileHandle;
bool SerialListInitialised = false;
bool isSending = false;

//Waits for a single byte response
void WaitForResponse(HANDLE Port, char ExpectedResponse)
{
    unsigned a;
    while(true)
    {
        char Response[10] = {};
        ReadFile(Port, &Response, 10, (LPDWORD)&a, NULL);
        if(Response[0] == ExpectedResponse) return;
    }
}

/* The function that sends the image's instruction (for now only in non-technical drawing mode) to Arduino, running in a separate thread
 * It works like this: each "packet" is exactly 9 bytes
 * There are six instructions: M for move (lifting the pen up), U, D, R, L for moving respectively up, down, right, left by 1 pixel with pen down,
 * N for doing nothing
 * U, D, R, L, N are 1-byte long, while M is 9-bytes long, of the form MXXXXYYYY, where X and Y are the destination coordinates
 * given in base-16, with the units being pixels
 * No instruction crosses packet boundaries: if an M starts in the middle of a packet, the program pads the packet with N's and puts
 * the M in the next packet
 * After the PC has sent the 9 bytes, Arduino sends a C to confirm reception. If Arduino fails to send a C within 1 second, the whole packet is resent
 * When the printer has completed all the instructions in a packet, it sends a D, allowing the next packet to be sent
 * To signal the start of instructions in non-technical mode, a packet is sent entirely composed of I's, to which Arduino responds with a single I
 * Before the first instruction, a P-packet is sent, with syntax PXXXXYYYY, where X and Y are the image's hexadecimal width and height in pixels
 * When the instructions are over, a packet is sent entirely composed of Q's
 */
//TODO Finish this, organise it into functions
void Send(HANDLE Port)
{
    unsigned a; //The length of messages sent, we don't care about it, windows needs us to pass this to it
    
    char BeginMessage[] = {'I', 'I', 'I', 'I', 'I', 'I', 'I', 'I', 'I', 'I'};
    WriteFile(Port, BeginMessage, 10, (LPDWORD)&a, NULL);
    WaitForResponse(Port, 'I');

    char PacketImageInfo[10] = {}; //10 so we can use sprintf (it adds a null terminator)
    sprintf(PacketImageInfo, "P%.4X%.4X", Image.w, Image.h);
    
    while(true)
    {
        WriteFile(Port, PacketImageInfo, 9, (LPDWORD)&a, NULL);
        char Response[10] = {};
        ReadFile(Port, &Response, 10, (LPDWORD)&a, NULL);
        if(Response[0] == 'C') return;   
    }
    
    while(Path.size()>0)
    {
        //We first make the packet...
        unsigned PacketPosition = 0; //A packet is 9 bytes; we keep track of the bytes we have already filled
        char Packet[10] = {}; //We use sprintf
        while(PacketPosition < 9)
        {
            Instruction CurrentInstruction = (Path.size() > 0) ? Path.front() : Instruction(NoOp, Vector2D(0, 0)); //We have to check whether the deque is empty
            if(Path.front().Type == MoveTo) //Instructions never cross packet boundaries
            {
                if(PacketPosition != 0)
                {
                    for(int i = PacketPosition; i < 9; i++)
                    {
                        Packet[i] = 'N';
                    }
                }
                else
                {
                    Path.pop_front();
                    if(CurrentInstruction.Type == MoveTo)
                    {
                        unsigned XCoordinate = CurrentInstruction.d.x; //TODO: check direction signs on arduino
                        unsigned YCoordinate = CurrentInstruction.d.y; // are the same as here
                        sprintf(Packet, "M%.4X%.4X", XCoordinate, YCoordinate);
                    }            
                }
                PacketPosition = 9;
            }
            else
            {
                Path.pop_front();
                Packet[PacketPosition] = InstructionLetters[CurrentInstruction.Type];
                PacketPosition++;
            }
        }



        //Then we send it...
        bool Success = false;
        while(!Success)
        {
            WriteFile(Port, Packet, 10, (LPDWORD)&a, NULL);
            char Response = 0;
            ReadFile(Port, &Response, 1, (LPDWORD)&a, NULL);
            if(Response == 'C')
            {
                Success = true;
            }
        }


        
        //And finally we wait for the command to complete
        WaitForResponse(Port, 'D');
        
    }

    char EndMessage[] = {'Q', 'Q', 'Q', 'Q', 'Q', 'Q', 'Q', 'Q', 'Q', 'Q'};
    WriteFile(Port, EndMessage, 10, (LPDWORD)&a, NULL);
    
    isSending = false;
    CloseHandle(Port);
}

//We try to open all port to see which ones are active
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
    Output.clear();
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
                        if(Processed && SerialListInitialised && !isSending)
                        {
                            isSending = true;
                            unsigned PortID = SendMessage(GetDlgItem(hWnd, IDC_COMBO1), CB_GETCURSEL, 0, 0);
                            unsigned PortNumber = 0;
                            for(int i = 1; i <= 99; i++) //Sadly the current selection is a pretty bad way to identify a port, we have to iterate 
                                                         //over the available ports using it as an index
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
                            DCB CurrentCommState;
                            GetCommState(Port, &CurrentCommState);
                            CurrentCommState.BaudRate = 9600;
                            CurrentCommState.Parity = NOPARITY;
                            CurrentCommState.StopBits = ONESTOPBIT;
                            SetCommState(Port, &CurrentCommState);
                            COMMTIMEOUTS cto = {};
                            cto.ReadIntervalTimeout = 100;
                            SetCommTimeouts(Port, &cto);
                            std::thread SendingThread (Send, Port);
                            SendingThread.detach();
                            return TRUE;
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
                        if(isAnImageLoaded)
                        {
                            Path.clear();
                            Log("Starting greyscale conversion", LogFileHandle);
                            GreyScaleEuclideanNorm(Image.Pixels, Image.Pixels, Image.w, Image.h);
                            Log("Greyscale conversion completed. Starting pathfinding", LogFileHandle);
                            DFSPathFinding(Image.Pixels, Image.w, Image.h, Path);
//                          LogPathFinding();
                            Log("Pathfinding completed", LogFileHandle);
                            Processed = true;
                            return 0;
                        }
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
