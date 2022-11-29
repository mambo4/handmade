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
	int BytesPerPixel;
};

struct win32_window_dimension
{
	int Width;
	int Height;
};

/*
loading XInputGetState  & XInputSetState directly from xinput.h dll
https://youtu.be/J3y1x54vyIQ?t=1255

l33t pointer to a function defined elsewhwere macro crap
I don't quite get

https://youtu.be/J3y1x54vyIQ?t=1745

*/

#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub)
{
	return (0);
}

#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);

X_INPUT_SET_STATE(XInputSetStateStub)
{
	return (0);
}

global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;

#define XInputGetState XInputGetState_
#define XInputSetState XInputSetState_

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

global_variable bool Running;
global_variable win32_offscreen_buffer GlobalBackBuffer;
const float PI = 3.14159265359;
const uint32 STARTCOLOR = 0x00000000;

internal void RenderWeirdGradient(win32_offscreen_buffer Buffer, int BlueOffset, int GreenOffset, int RedOffset)
{
	uint8 *Row = (uint8 *)Buffer.Memory; // cast the void pointer BitmapMemory to unsigned 8 bit int
	for (int Y = 0; Y < Buffer.Height; ++Y)
	{
		uint32 *Pixel = (uint32 *)Row; // pointer to first RGBA 32bit pixel of Row: 0xAARRGGBB
		for (int X = 0; X < Buffer.Width; ++X)
		{
			uint8 A = 0x00;
			uint8 B = (uint8)(X + BlueOffset);								// Blue
			uint8 G = (uint8)(Y + GreenOffset);								// Green
			uint8 R = (uint8)(X + RedOffset);								// red
			uint32 ARGB = (uint32)((A) | (R << 8) | (G << 16) | (B << 24)); // Comine 8 bit components by bitwise shift and bitwise OR
			*Pixel = ARGB;
			++Pixel;
		}
		Row += Buffer.Pitch;
	}
}
internal void RenderGrid(win32_offscreen_buffer Buffer)
{
	const uint32 BLACK = 0x00000000;
	const uint32 DARK_GREY = 0xff111111;
	const uint32 MED_GREY = 0xff333333;
	const uint32 LIGHT_GREY = 0xff888888;
	const uint32 WHITE = 0xffffffff;

	const int Y_ORIGIN = (int)Buffer.Height / 2;
	const int X_ORIGIN = (int)Buffer.Width / 2;

	const int DIV1 = 100;
	const int DIV2 = 10;

	uint8 *Row = (uint8 *)Buffer.Memory; // cast the void pointer BitmapMemory to unsigned 8 bit int
	for (int Y = 0; Y < Buffer.Height; ++Y)
	{
		uint32 *Pixel = (uint32 *)Row; // pointer to first RGBA 32bit pixel of Row: 0xAARRGGBB
		for (int X = 0; X < Buffer.Width; ++X)
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

		Row += Buffer.Pitch;
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
	Buffer->BytesPerPixel = 4;

	Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
	Buffer->Info.bmiHeader.biWidth = Buffer->Width;
	Buffer->Info.bmiHeader.biHeight = -Buffer->Height; // negative to yield a 'top down'
	Buffer->Info.bmiHeader.biPlanes = 1;
	Buffer->Info.bmiHeader.biBitCount = 32;
	Buffer->Info.bmiHeader.biCompression = BI_RGB;

	int BitmapMemorySize = (Buffer->Width * Buffer->Height) * Buffer->BytesPerPixel;
	Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);

	Buffer->Pitch = Width * Buffer->BytesPerPixel; // Pitch is the difference between rows of pixels in Bytes
}

internal void Win32DisplayBufferInWindow(HDC DeviceContext, int WindowWidth, int WindowHeight, win32_offscreen_buffer Buffer, int x, int y, int Width, int Height)
{
	// TODO: Aspect Ratio Correction
	StretchDIBits(DeviceContext,
				  0, 0, WindowWidth, WindowHeight,
				  0, 0, Buffer.Width, Buffer.Height,
				  Buffer.Memory,
				  &Buffer.Info,
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
	case WM_SIZE:
	{
	}
	break;

	case WM_DESTROY:
	{
		// handle as error, recreate window
		Running = false;
		OutputDebugStringA("WM_DESTROY\n");
	}
	break;

	case WM_CLOSE:
	{
		// todo: handle with message to user
		Running = false;
		OutputDebugStringA("WM_CLOSE\n");
	}
	break;

	case WM_ACTIVATEAPP:
	{
		OutputDebugStringA("WM_ACTIVATEAPP\n");
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
		Win32DisplayBufferInWindow(DeviceContext, Dimension.Width, Dimension.Height, GlobalBackBuffer, X, Y, Width, Height);
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

	WNDCLASS WindowClass = {}; // declares a WNDCLASS instance 'windowClass', with members initialized to 0.

	Win32ResizeDIBSection(&GlobalBackBuffer, 1280, 720);

	WindowClass.style = CS_HREDRAW | CS_VREDRAW;	   // bitfield flags to define windowstyle see MSDN. "if the window resizes H or V, redraw the whole thing"
	WindowClass.lpfnWndProc = Win32MainWindowCallback; // pointer to a function that defines window's response to events
	WindowClass.hInstance = Instance;				   // reference to the instance of this window, from WinMain function.(Could also use GetModuleHandle)
	// WindowClass.hIcon = ; // icon for window
	WindowClass.lpszClassName = "handmadeHeroWindowClass";

	if (RegisterClass(&WindowClass))
	{
		HWND Window = CreateWindowEx(
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
			int BlueOffset = 0;
			int GreenOffset = 0;
			int RedOffset = 0;

			Running = true;
			while (Running)
			{
				//
				MSG Message;
				while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
				{
					if (Message.message == WM_QUIT)
					{
						Running = false;
					}

					TranslateMessage(&Message);
					DispatchMessageA(&Message);
				}
				// todo: should we poll this more frequently?

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
					}
					else
					{
						// Controller is not connected
					}
				}

				// RenderGrid(GlobalBackBuffer);
				RenderWeirdGradient(GlobalBackBuffer, RedOffset, BlueOffset, GreenOffset);

				HDC DeviceContext = GetDC(Window);
				win32_window_dimension Dimension = Win32GetWindowDimension(Window);
				Win32DisplayBufferInWindow(DeviceContext, Dimension.Width, Dimension.Height, GlobalBackBuffer, 0, 0, Dimension.Width, Dimension.Height);
				ReleaseDC(Window, DeviceContext);

				++BlueOffset;
				++GreenOffset;
				--RedOffset;
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


