/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Carmine Foggia
   ======================================================================== */
/*TODO:
 *Pause does not work
 *This does not reset correctly after finishing a drawing
 *This once stalled in the middle of a drawing, was it the PC's standby or a bug?
 *Add option to show progress monitor in fullscreen
 *Test paper size
 *Refactor, polish all those hacks
 *TODO:
 *Add stop button to change pencil
 *Implement technical drawing
 *Half-tone, palettes
 *Come up with a decent way to make technical drawing files, do we make
 *    our own interface? Is there a file format we can hang onto?
 *    SVG? GeoGebra?
 */
#include <deque>
#include <stack>
#include <string.h>
#include <string>
#include <windows.h>
#include <windowsx.h>
#include <winuser.h>
#include <synchapi.h>
#include <thread>
#include <math.h>
#include "..\res\resource.h"
#include "..\lib\lodepng.h"
#include "..\lib\files.h"


const int PaperMaxX = 47600; //as measured by calibrating the printer for an A4 sheet
const int PaperMaxY = 33700; //
const int ImagePacketSize = 32;

bool PortsConnected[100] = {};

enum InstructionTypeEnum : unsigned
  {
    MoveUp = 0, MoveDown = 1, MoveRight = 2, MoveLeft = 3, NoOp = 4, Lift = 5
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
  Instruction(InstructionTypeEnum t):
    Type(t), d(Vector2D(0,0))
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

char InstructionLetters[] = {'U', 'D', 'R', 'L', 'N', 'Z'};

//Runtime global variables
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
bool Pause = false;
bool SendPacket = false;
bool QuitSending = false;

//Status variables for the "debugging" window
HDC CustomWindowDC = 0;
ImageClass DebuggerImage {};
bool isDebuggerInitialised = false;
Vector2D CurrentPosition = {0, 0};
bool PenUp = true;

Vector2D LastPoint = {0,0};

//Waits for a single byte response
void WaitForResponse(HANDLE Port, char ExpectedResponse)
{
  unsigned a;
  while(true)
    {
      char Response[10] = {};
      ReadFile(Port, &Response, 1, (LPDWORD)&a, NULL);
      if(Response[0] == ExpectedResponse) return;
    }
}

/* The function that sends the image's instruction (for now only in non-technical drawing mode) to Arduino, running in a separate thread
 * It works like this: each "packet" is exactly 32 bytes
 * There are six instructions: Z for lift/lower, U, D, R, L for moving respectively up, down, right, left by 1 pixel with pen down,
 * N for doing nothing
 * Each instruction is 32-byte long
 * After the PC has sent the 32 bytes, Arduino sends a C to confirm reception. 
 * If Arduino fails to send a C within 1 second, 
 * the whole packet is resent (NOTE do we want this? It's probably what makes pause not work correctly)
 * When the printer has completed all the instructions in a packet, it sends a D, allowing the next packet to be sent
 * To signal the start of instructions in non-technical mode, a packet is sent entirely composed of I's, 
 * to which Arduino responds with a single I
 * Before the first instruction, a P-packet is sent, with syntax PXXXXYYYY, 
 * where X and Y are the image's hexadecimal width and height in pixels
 * When the instructions are over, a packet is sent entirely composed of Q's (NOTE or it should be, I still haven't managed to make it work)
 */
//TODO Finish this, organise it into functions
void Send(HANDLE Port)
{
  /*  unsigned a; //The length of messages sent, we don't care about it, windows needs us to pass this to it
    
  char BeginMessage[] = {'I', 'I', 'I', 'I', 'I', 'I', 'I', 'I', 'I'};
  WriteFile(Port, BeginMessage, 9, (LPDWORD)&a, NULL);
  Sleep(100);
  WaitForResponse(Port, 'I');
  Log("Board connected.", LogFileHandle);
    
  char PacketImageInfo[10] = {}; //10 so we can use sprintf (it adds a null terminator)
  Sleep(100);
  sprintf(PacketImageInfo, "P%4X%4X", Image.w, Image.h);


    
        
  while(true)
    {
      WriteFile(Port, PacketImageInfo, 9, (LPDWORD)&a, NULL);
      char Response[10] = {};
      ReadFile(Port, &Response, 1, (LPDWORD)&a, NULL);
      if(Response[0] == 'C') break;
    }
  */
  char PixelSizeString[256] = {};
  sprintf(PixelSizeString, "Pixel sizes: x=%d, y=%d", PaperMaxX / Image.w, PaperMaxY / Image.h);
    
  SetDlgItemText(SendDialog, IDC_PIXELSIZE, PixelSizeString);

  char InstructionNumberString[256] = {};
  sprintf(InstructionNumberString, "Instructions: %d", Path.size());

  SetDlgItemText(SendDialog, IDC_INSTRUCTIONNUMBER, InstructionNumberString);
    
  Log("Instructions starting", LogFileHandle);

  PenUp = true;
  while(!Path.empty() && (!Pause || SendPacket) && !QuitSending)
    {
      //We first make the packet...
      unsigned PacketPosition = 0; //A packet is 9 bytes; we keep track of the bytes we have already filled
      char Packet[ImagePacketSize+1] = {}; //We use sprintf
      while(PacketPosition < ImagePacketSize)
        {
	  Instruction CurrentInstruction = (Path.empty()) ? Instruction(NoOp, Vector2D(0, 0)) : Path.front(); //We have to check whether the deque is empty
	  
	  if(!Path.empty())
	    {
	      Path.pop_front();
	    }
	  else
	    {
	      break;
	    }
	  Packet[PacketPosition] = InstructionLetters[CurrentInstruction.Type];
	  PacketPosition++;
	  

	  //TODO: remake instructions completely, make them be (x, y, lift)
	  switch(CurrentInstruction.Type)
	    {
	    case MoveUp:
	      {
		CurrentPosition.y -= 1;
		break;
	      }
	    case MoveDown:
	      {
		CurrentPosition.y += 1;
		break;
	      }
	    case MoveRight:
	      {
		CurrentPosition.x += 1;
		break;
	      }
	    case MoveLeft:
	      {
		CurrentPosition.x -= 1;
		break;
	      }
	    case Lift:
	      {
		PenUp ^= true;
		break;
	      }
	    }
	  if(!PenUp)
	    {
	      DebuggerImage.Pixels[CurrentPosition.y*DebuggerImage.w+CurrentPosition.x] = 0;
	    }
        }

      //Then we send it...
      /*  bool Success = false;
      while(!Success)
	{
	  WriteFile(Port, Packet, ImagePacketSize, (LPDWORD)&a, NULL);
	  char Response = 0;
	  Sleep(100);
	  ReadFile(Port, &Response, 1, (LPDWORD)&a, NULL);
	  if(Response == 'C')
	    {
	      Success = true;
	    }
	}


        
      //And finally we wait for the command to complete
      WaitForResponse(Port, 'D');
      */
      char LogMessage[] = {"Sent packet XXXXXXXXX. Instructions left:           "};
      sprintf(LogMessage, "Sent packet %s. Instructions left: %d.", Packet, Path.size());
      Log(LogMessage, LogFileHandle);
      
      SendPacket = false;
    }

  /* char EndMessage[] = {'Q', 'Q', 'Q', 'Q', 'Q', 'Q', 'Q', 'Q', 'Q', 'Q'};
  WriteFile(Port, EndMessage, 9, (LPDWORD)&a, NULL);
  Log("Ending.", LogFileHandle);
  */
  isSending = false;
  QuitSending = false;
  //CloseHandle(Port);
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

void AddMoveTo(std::deque<Instruction>& Output, Vector2D StartPoint, Vector2D EndPoint)
{
  InstructionTypeEnum XDirection, YDirection;
  if(StartPoint.x < EndPoint.x) XDirection = MoveRight; else XDirection = MoveLeft;
  if(StartPoint.y < EndPoint.y) YDirection = MoveDown; else YDirection = MoveUp;
  Output.push_back(Instruction(Lift));
  for(int i = 0; i < abs(EndPoint.x - StartPoint.x); i++)
    {
      Output.push_back(Instruction(XDirection));
    }
  for(int i = 0; i < abs(EndPoint.y - StartPoint.y); i++)
    {
      Output.push_back(Instruction(YDirection));
    }
  Output.push_back(Instruction(Lift));
}

//Our implementation of depth-first search. We do not use the OS's stack because it would overflow
void DFSRecursion(unsigned* Input, unsigned* PixelStatus, unsigned w, unsigned h, std::deque<Instruction>& Output, unsigned x, unsigned y)
{
  std::stack<Vector2D> DFSStack {};
  unsigned CurrentX = x;
  unsigned CurrentY = y;
  //Output.push_back({MoveTo, Vector2D(CurrentX, CurrentY)});
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
	  break;
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
	      AddMoveTo(Output, CurrentPoint, NextPoint);
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
	      AddMoveTo(Output, CurrentPoint, NextPoint);
            }
        }
      else
	{
	  AddMoveTo(Output, CurrentPoint, NextPoint);
        }
      CurrentPoint = NextPoint;
    }
  LastPoint = CurrentPoint;
}

/*Our pathfinding algorithm. The way it works:
 *We run depth-first search on every black pixel unless it is already on the plotter's path
 *We keep track of the instructions on a deque, so that we can both add them from the end (when running the algorithm)
 *and read them from the beginning (when printing)*/
void DFSPathFinding(unsigned* Input, unsigned w, unsigned h, std::deque<Instruction>& Output)
{
  Output.clear();
  LastPoint = Vector2D(0,0);
  unsigned *PixelStatus = new unsigned[w*h]; /*An array of the same size as the image. Every position is 0 if the corresponding pixel
					      *is not coloured, 1 if it is but hasn't been found yet by the pathfinding, 2 if it has
					      */
  memset(PixelStatus, 0, 4*w*h);
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
	      AddMoveTo(Output, LastPoint, Vector2D(x, y)); 
	      DFSRecursion(Input, PixelStatus, w, h, Output, x, y);
            }
        }
    }
  AddMoveTo(Output, LastPoint, Vector2D(0, 0));
  Output.pop_front(); //MoveTo adds a lift both as the first and last instruction, which we do not want
  Output.pop_back();
  delete[] PixelStatus;
}

void LogInstruction(Instruction instr)
{
  char ToSend[2] = {};
  ToSend[0] = InstructionLetters[instr.Type];
  WriteToLog(ToSend,
	     LogFileHandle);
}

void LogPathFinding()
{
  int i = 0;
  for(auto it = Path.cbegin(); it != Path.cend(); it++)
    {
      LogInstruction(*it);
      i++;
      if(i % 32 == 0)
        {
	  WriteToLog("\n", LogFileHandle);
        }
    }
  WriteToLog("\n", LogFileHandle);
}

LRESULT CALLBACK StatusCallback
(HWND   hWnd,
 UINT   uMsg,
 WPARAM wParam,
 LPARAM lParam)
{
  switch(uMsg)
    {
    case WM_SHOWWINDOW:
      {
	if(!isDebuggerInitialised && Processed)
	  {
	    isDebuggerInitialised = true;
	    DebuggerImage.Info = Image.Info;
	    memcpy(DebuggerImage.FileName, Image.FileName, 256);
	    DebuggerImage.w = Image.w;
	    DebuggerImage.h = Image.h;
	    DebuggerImage.Pixels = new unsigned[Image.w * Image.h];
	    memcpy(DebuggerImage.Pixels, Image.Pixels, sizeof(unsigned) * Image.w * Image.h);
	    for(int i = 0; i < Image.w * Image.h; i++)
	      {
		if(Image.Pixels[i] == 0) DebuggerImage.Pixels[i] = 0x808080;
	      }
	  }
	return CallWindowProc(DefWindowProc, hWnd, uMsg, wParam, lParam);
	break;
      }
    case WM_CLOSE:
    case WM_QUIT:
    case WM_DESTROY:
      {
	isDebuggerInitialised = false;
	delete[] DebuggerImage.Pixels;
	return 0;
	break;
      }
    case WM_PAINT:
      {
	if(isDebuggerInitialised)
	  {
	    InvalidateRect(hWnd, NULL, FALSE);
	    PAINTSTRUCT ps;
	    HDC DCToUse = BeginPaint(hWnd, &ps);
	    RECT ClientRect;
	    GetClientRect(hWnd, &ClientRect);
	    int a = StretchDIBits(DCToUse,
				  0, 0,
				  ClientRect.right - ClientRect.left,
				  ClientRect.bottom - ClientRect.top,
				  0, 0,
				  DebuggerImage.w, DebuggerImage.h, DebuggerImage.Pixels, &DebuggerImage.Info,
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
	DestroyWindow(GetDlgItem(hWnd, IDC_CUSTOM1));
	DestroyWindow(hWnd);
	isSendOpen = false;
	SendDialog = NULL;
	SerialListInitialised = false;
	ReleaseDC(GetDlgItem(hWnd, IDC_CUSTOM1), CustomWindowDC);
	return TRUE;
	break;
      }
    case WM_INITDIALOG:
      {
	SendDialog = hWnd;
	HWND ComboBoxHandle = GetDlgItem(hWnd, IDC_COMBO1);
	HWND EditBoxHandle = GetDlgItem(hWnd, IDC_PIXELSIZE);
	CustomWindowDC = GetDC(GetDlgItem(hWnd, IDC_CUSTOM1));
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
	      case IDC_SEND:
		{
		  /*		  if(Processed && SerialListInitialised && !isSending)
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
		      SetCommTimeouts(Port, &cto);*/
		  std::thread SendingThread (Send, INVALID_HANDLE_VALUE /*Port*/);
		      SendingThread.detach();
		      return TRUE;
		      /*}*/
		}
	      case IDC_PAUSE:
		{
		  Pause ^= true;
		  break;
		}
	      case IDC_SENDPACKET:
		{
		  SendPacket = true;
		  break;
		}
	      case IDC_QUIT:
		{
		  QuitSending = true;
		  break;
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
		      delete[] Image.Pixels;
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
		      //We convert from the library's format (24-bit RGB) to windows' (32-bit 0RGB)
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
		      LogPathFinding();
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
      if(isSendOpen) RedrawWindow(GetDlgItem(SendDialog, IDC_CUSTOM1), NULL, NULL, RDW_INTERNALPAINT);
      timeBeginPeriod(1);
      Sleep(16);
      timeEndPeriod(1);
    }
  ReleaseDC(Window, MainDC);
  return TRUE;
}
