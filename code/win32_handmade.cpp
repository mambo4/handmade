//
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

#include <Windows.h>
#include <stdint.h> //for access to unit8_t type
#include <xinput.h> //for xbox controller

// these #defines reuse 'static' with more clarfied intent
#define internal static
#define local_persist static
#define global_variable static

// these typedefs redefine types from stdint.h
// for easier typing than 'unsigned char' etc
typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

struct win32_offscreen_buffer
{
	BITMAPINFO Info;
	void *Memory;
	int Width;
	int Height;
	int Pitch;
	// int BytesPerPixel; (always 4 bytes 32 bits memoryt order BB GG RR XX)
};

struct win32_window_dimension
{
	int Width;
	int Height;
};

/*
loading XInputGetState  & XInputSetState directly from
"C:\Program Files (x86)\Windows Kits\10\Include\10.0.17763.0\um\Xinput.h"
https://youtu.be/J3y1x54vyIQ?t=1255

l33t pointer to a function defined elsewhwere macro crap I don't quite get

https://youtu.be/J3y1x54vyIQ?t=1745

*/

#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState) //macro creates fns 'name' with the signature args
typedef X_INPUT_GET_STATE(x_input_get_state);// create a 'type' x_input_get_state(DWORD dwUserIndex, XINPUT_STATE *pState)
X_INPUT_GET_STATE(XInputGetStateStub)// create XInputGetStateStub(DWORD dwUserIndex, XINPUT_STATE *pState){ return(0); }
{
	return (0);
}
global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;//define a pointer to XInputGetStateStub
#define XInputGetState XInputGetState_ //call the pointer 'XInputGetState'

#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);

X_INPUT_SET_STATE(XInputSetStateStub)
{
	return (0);
}
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_


internal void Win32LoadXInput(void)
{
	HMODULE XInputLibrary = LoadLibraryA("XInput1_4.dll");
	if (XInputLibrary)
	{
		XInputGetState = (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
		XInputSetState = (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
	}
}

// end l33t crap


win32_window_dimension Win32GetWindowDimension(HWND Window)
{

	RECT ClientRect;
	win32_window_dimension Result;
	GetClientRect(Window, &ClientRect);
	Result.Width = ClientRect.right - ClientRect.left;
	Result.Height = ClientRect.bottom - ClientRect.top;
	return (Result);
}

global_variable bool GlobalRunning;
global_variable win32_offscreen_buffer GlobalBackBuffer;
const float PI = 3.14159265359;
const uint32 STARTCOLOR = 0x00000000;

internal void RenderWeirdGradient(win32_offscreen_buffer *Buffer, int XOffset, int YOffset, int RedOffset)
{
	uint8 *Row = (uint8 *)Buffer->Memory; // cast the void pointer BitmapMemory to unsigned 8 bit int
	for (int Y = 0; Y < Buffer->Height; ++Y)
	{
		uint32 *Pixel = (uint32 *)Row; // pointer to first RGBA 32bit pixel of Row: 0xAARRGGBB
		for (int X = 0; X < Buffer->Width; ++X)
		{
			uint8 A = 0x00;												// Alpha
			uint8 B = (uint8)(X + XOffset);								// Blue
			uint8 G = (uint8)(Y + YOffset);								// Green
			uint8 R = (uint8)(RedOffset);								// red

			
			uint32 BGRA = (uint32)((B) | (G << 8) | (R << 16) | (A << 24)); // Comine 8 bit components by bitwise shift and bitwise OR
			*Pixel = BGRA;
			++Pixel;
		}
		Row += Buffer->Pitch;
	}
}
internal void RenderGrid(win32_offscreen_buffer *Buffer)
{
	const uint32 BLACK = 0x00000000;
	const uint32 DARK_GREY = 0xff111111;
	const uint32 MED_GREY = 0xff333333;
	const uint32 LIGHT_GREY = 0xff888888;
	const uint32 WHITE = 0xffffffff;

	const int Y_ORIGIN = (int)Buffer->Height / 2;
	const int X_ORIGIN = (int)Buffer->Width / 2;

	const int DIV1 = 100;
	const int DIV2 = 10;

	uint8 *Row = (uint8 *)Buffer->Memory; // cast the void pointer BitmapMemory to unsigned 8 bit int
	for (int Y = 0; Y < Buffer->Height; ++Y)
	{
		uint32 *Pixel = (uint32 *)Row; // pointer to first RGBA 32bit pixel of Row: 0xAARRGGBB
		for (int X = 0; X < Buffer->Width; ++X)
		{

			const int X_COORD = X - X_ORIGIN;
			const int Y_COORD = Y - Y_ORIGIN;

			uint32 color = BLACK;

			if (Y > 0 && X > 0)
			{ // avoid div by 0

				if (Y_COORD % DIV2 == 0 || X_COORD % DIV2 == 0)
				{
					color = DARK_GREY;
				}

				if (Y_COORD % DIV1 == 0 || X_COORD % DIV1 == 0)
				{
					color = MED_GREY;
				}

				if (Y == Y_ORIGIN || X == X_ORIGIN)
				{
					color = LIGHT_GREY;
				}
			}

			// draw Pixel and increment
			*Pixel = color;
			++Pixel;
		}

		Row += Buffer->Pitch;
	}
}

internal void Win32ResizeDIBSection(win32_offscreen_buffer *Buffer, int Width, int Height)
{
	if (Buffer->Memory)
	{
		VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
	}

	Buffer->Width = Width;
	Buffer->Height = Height;
	int BytesPerPixel = 4;

	Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
	Buffer->Info.bmiHeader.biWidth = Buffer->Width;
	Buffer->Info.bmiHeader.biHeight = -Buffer->Height; // negative to yield a 'top down'
	Buffer->Info.bmiHeader.biPlanes = 1;
	Buffer->Info.bmiHeader.biBitCount = 32;
	Buffer->Info.bmiHeader.biCompression = BI_RGB;

	int BitmapMemorySize = (Buffer->Width * Buffer->Height) * BytesPerPixel;
	Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);

	Buffer->Pitch = Width * BytesPerPixel; // Pitch is the difference between rows of pixels in Bytes
}

internal void Win32DisplayBufferInWindow(win32_offscreen_buffer *Buffer, HDC DeviceContext, int WindowWidth, int WindowHeight)
{
	// TODO: Aspect Ratio Correction
	StretchDIBits(DeviceContext,
				  0, 0, WindowWidth, WindowHeight,	   // Xpos,Ypos,W,H
				  0, 0, Buffer->Width, Buffer->Height, // Xpos,Ypos,W,H
				  Buffer->Memory,
				  &Buffer->Info,
				  DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK Win32MainWindowCallback(
	HWND Window,   // handle to a window
	UINT Message,  // window message we want to handle
	WPARAM WParam, // width
	LPARAM LParam) // Height
{
	LRESULT Result = 0;

	switch (Message)
	{

	case WM_CLOSE:
	{
		// todo: handle with message to user
		GlobalRunning = false;
		OutputDebugStringA("WM_CLOSE\n");
	}
	break;

	case WM_ACTIVATEAPP:
	{
		OutputDebugStringA("WM_ACTIVATEAPP\n");
	}
	break;

	case WM_DESTROY:
	{
		// handle as error, recreate window
		GlobalRunning = false;
		OutputDebugStringA("WM_DESTROY\n");
	}
	break;

	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
	case WM_KEYDOWN:
	case WM_KEYUP:
	{
		uint32 VKCode = WParam;
		bool WasDown = ((LParam & (1 << 30)) != 0);
		bool IsDown = ((LParam & (1 << 30)) == 0);
		if (WasDown != IsDown)
		{
			if (VKCode == 'W')
			{
			}
			else if (VKCode == 'A')
			{
			}
			else if (VKCode == 'S')
			{
			}
			else if (VKCode == 'D')
			{
			}
			else if (VKCode == 'Q')
			{
			}
			else if (VKCode == 'E')
			{
			}
			else if (VKCode == VK_UP)
			{
			}
			else if (VKCode == VK_DOWN)
			{
			}
			else if (VKCode == VK_LEFT)
			{
			}
			else if (VKCode == VK_RIGHT)
			{
			}
			else if (VKCode == VK_ESCAPE)
			{
				OutputDebugStringA("ESCAPE:");
				if (IsDown)
				{
					OutputDebugStringA("IsDown");
				}
				if (WasDown)
				{
					OutputDebugStringA("WasDown");
				}
				OutputDebugStringA("\n");
			}
			else if (VKCode == VK_SPACE)
			{
			}
		}
	}
	break;

	case WM_PAINT:
	{
		PAINTSTRUCT Paint;
		HDC DeviceContext = BeginPaint(Window, &Paint);
		int X = Paint.rcPaint.left;
		int Y = Paint.rcPaint.top;
		int Width = Paint.rcPaint.right - Paint.rcPaint.left;
		int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;
		win32_window_dimension Dimension = Win32GetWindowDimension(Window);
		Win32DisplayBufferInWindow(&GlobalBackBuffer, DeviceContext, Dimension.Width, Dimension.Height);
		EndPaint(Window, &Paint);
	}
	break;

	default:
	{

		// OutputDebugStringA("DEFAULT\n");
		Result = DefWindowProc(Window, Message, WParam, LParam);
	}
	break;
	}
	return (Result);
}

int CALLBACK WinMain(
	HINSTANCE Instance,
	HINSTANCE PrevInstance,
	LPSTR Command,
	int ShowCode)
{

	Win32LoadXInput();
	WNDCLASSA WindowClass = {}; //

	Win32ResizeDIBSection(&GlobalBackBuffer, 1280, 720);

	WindowClass.style = CS_HREDRAW | CS_VREDRAW;	   // bitfield flags to define windowstyle see MSDN. "if the window resizes H or V, redraw the whole thing"
	WindowClass.lpfnWndProc = Win32MainWindowCallback; // pointer to a function that defines window's response to events
	WindowClass.hInstance = Instance;				   // reference to the instance of this window, from WinMain function.(Could also use GetModuleHandle)
	// WindowClass.hIcon = ; // icon for window
	WindowClass.lpszClassName = "handmadeHeroWindowClass";

	if (RegisterClassA(&WindowClass))
	{
		HWND Window = CreateWindowExA(
			0,
			WindowClass.lpszClassName,
			"Handmade Hero",
			WS_OVERLAPPEDWINDOW | WS_VISIBLE,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			0,
			0,
			Instance,
			0);
		if (Window)
		{
			int XOffset = 0;
			int YOffset = 0;
			int RedOffset = 0;
			GlobalRunning = true;
			/*******************************************************
			 *  GAME LOOP
			 ********************************************************/
			while (GlobalRunning)
			{
				//
				MSG Message;
				while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
				{
					if (Message.message == WM_QUIT)
					{
						GlobalRunning = false;
					}

					TranslateMessage(&Message);
					DispatchMessageA(&Message);
				}
				// todo: should we poll this more frequently?
				/*******************************************************
				 *  GAME LOOP : input
				 ********************************************************/
				DWORD dwResult;
				for (DWORD i = 0; i < XUSER_MAX_COUNT; i++)
				{
					XINPUT_STATE state;
					ZeroMemory(&state, sizeof(XINPUT_STATE));

					// Simply get the state of the controller from XInput.
					dwResult = XInputGetState(i, &state);
					if (dwResult == ERROR_SUCCESS)
					{
						// Controller is connected
						XINPUT_GAMEPAD *Pad = &state.Gamepad;

						bool Up = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
						bool Down = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
						bool Left = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
						bool Right = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
						bool Start = (Pad->wButtons & XINPUT_GAMEPAD_START);
						bool Back = (Pad->wButtons & XINPUT_GAMEPAD_BACK);
						bool AButton = (Pad->wButtons & XINPUT_GAMEPAD_A);
						bool BButton = (Pad->wButtons & XINPUT_GAMEPAD_B);
						bool XButton = (Pad->wButtons & XINPUT_GAMEPAD_X);
						bool YButton = (Pad->wButtons & XINPUT_GAMEPAD_Y);

						int16 LStickX = Pad->sThumbLX;
						int16 LStickY = Pad->sThumbLY;
						int16 RStickX = Pad->sThumbRX;
						int16 RStickY = Pad->sThumbRY;

						/*******************************************************
						 *  GAME LOOP :simulate/update
						 ********************************************************/

						XOffset-=LStickX>>12;
						YOffset+=LStickY>>12;
						if(AButton)
						{
							RedOffset=255;
						}else{
							RedOffset=0;
						}
						
					}
					else
					{
						// Controller is not connected
					}
				}
				/********************************************************
				 *  GAME LOOP : Render
				 ********************************************************/

				/*
				// rumble test
				XINPUT_VIBRATION Vibration;
				Vibration.wLeftMotorSpeed=1000;
				Vibration.wRightMotorSpeed=1000;
				XInputSetState(0,&Vibration);
				*/

				// RenderGrid(GlobalBackBuffer);
				RenderWeirdGradient(&GlobalBackBuffer, XOffset, YOffset,RedOffset);

				HDC DeviceContext = GetDC(Window);
				win32_window_dimension Dimension = Win32GetWindowDimension(Window);
				Win32DisplayBufferInWindow(&GlobalBackBuffer, DeviceContext, Dimension.Width, Dimension.Height);
				ReleaseDC(Window, DeviceContext);

				// ++XOffset;
				// ++YOffset;
				// ++RedOffset;
			}
		}
		else
		{
			// todo: logging
		}
	}
	else
	{
		// todo: logging
	}

	return (0);
}