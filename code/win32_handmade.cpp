

#include <Windows.h>
#include <stdint.h> //for accss to unit8_t type

// these #defines reuse 'static' with more clarfied intent
#define internal static
#define local_persist static
#define global_variable static

// these typedefs redefine types from stdint.h
//for easier typing than 'unsigned char' etc
typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

global_variable bool Running;

struct win32_offscreen_buffer{
	BITMAPINFO Info;
	void *Memory;
	int Width;
	int Height;
	int Pitch;
	int BytesPerPixel;
};


internal void RenderWeirdGradient(win32_offscreen_buffer Buffer, int XOffset,int YOffset)
{
	uint8 *Row = (uint8 *)Buffer.Memory; // cast the void pointer BitmapMemory to unsigned 8 bit int
	for (int Y = 0; Y < Buffer.Height; ++Y)
	{
		//uint32 *Pixel = (uint32 *)Row; //pointer to first pixel of Row
		uint8 *Pixel = (uint8 *)Row; //pointer to first byte of first pixel of Row

		for (int X = 0; X <Buffer.Width; ++X) 
		{

			*Pixel = (uint8)(X+XOffset); //Blue
			++Pixel;

			*Pixel = (uint8)(Y+YOffset); //Green
			++Pixel;

			*Pixel = 0; //Red
			++Pixel;

			*Pixel = 0;
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
	Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
	Buffer->Info.bmiHeader.biWidth = Buffer->Width;
	Buffer->Info.bmiHeader.biHeight = -Buffer->Height; // negative top yield a 'top down'
	Buffer->Info.bmiHeader.biPlanes = 1;
	Buffer->Info.bmiHeader.biBitCount = 32;
	Buffer->Info.bmiHeader.biCompression = BI_RGB;

	int BitmapMemorySize = (Buffer->Width * Buffer->Height) * Buffer->BytesPerPixel;
	Buffer->Memory = VirtualAlloc(0,BitmapMemorySize,MEM_COMMIT,PAGE_READWRITE);

	Buffer->Pitch = Width * Buffer->BytesPerPixel;  // Pitch is the difference between rows of pixels in Bytes
}

internal void Win32CopyBufferInWindow(HDC DeviceContext, RECT ClientRect, win32_offscreen_buffer Buffer, int x, int y, int Width, int Height)
{
	int WindowWidth = ClientRect.right - ClientRect.left; 
	int WindowHeight = ClientRect.bottom - ClientRect.top;

	StretchDIBits(DeviceContext,
				  0, 0, Buffer.Width, Buffer.Height,
				  0, 0, WindowWidth, WindowHeight,
				  Buffer.Memory,
				  &Buffer.Info,
				  DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK Win32MainWindowCallback(
	HWND Window,   // handle to a window
	UINT Message,  // window message we want to handle
	WPARAM WParam, // width
	LPARAM LParam) //Height
{
	LRESULT Result = 0;

	switch (Message)
	{
	case WM_SIZE:
	{
		RECT ClientRect;
		GetClientRect(Window, &ClientRect);
		int Width = ClientRect.right - ClientRect.left;
		int Height = ClientRect.bottom - ClientRect.top;
		Win32ResizeDIBSection(Width, Height);
		OutputDebugStringA("WM_SIZE\n");
	}
	break;

	case WM_DESTROY:
	{
		//handle as error, recreate window
		Running = false;
		OutputDebugStringA("WM_DESTROY\n");
	}
	break;

	case WM_CLOSE:
	{
		//todo: handle with message to user
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
		RECT ClientRect;
		GetClientRect(Window, &ClientRect);
		Win32CopyBufferToWindow(DeviceContext, ClientRect, X, Y, Width, Height);
		EndPaint(Window, &Paint);
	}
	break;

	default:
	{

		//OutputDebugStringA("DEFAULT\n");
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
	WindowClass.style = CS_HREDRAW | CS_VREDRAW; // bitfield flags to define windowstyle see MSDN
	WindowClass.lpfnWndProc = Win32MainWindowCallback;// pointer to a function that defines window's response to events
	WindowClass.hInstance = Instance; // reference to the instance of this window, from WinMain function.(Could also use GetModuleHandle)
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
			int XOffset=0;
			int YOffset=0;

			Running = true;
			while (Running)
			{
				//https://youtu.be/w7ay7QXmo_o?t=922
				MSG Message;
				while(PeekMessage(&Message, 0, 0, 0,PM_REMOVE))
				{
					if (Message.message == WM_QUIT)
					{
						Running=false;
					}

					TranslateMessage(&Message);
					DispatchMessageA(&Message);
				}
				RenderWeirdGradient(XOffset,YOffset);

				RECT ClientRect;
				HDC DeviceContext = GetDC(Window);
				GetClientRect(Window, &ClientRect);
				int WindowWidth = ClientRect.right - ClientRect.left;
				int WindowHeight = ClientRect.bottom - ClientRect.top;
				Win32CopyBufferToWindow( DeviceContext, ClientRect, 0, 0, WindowWidth, WindowHeight);
				ReleaseDC(Window,DeviceContext);

				++XOffset;
				++YOffset;
			}
		}
		else
		{
			//todo: logging
		}
	}
	else
	{
		//todo: logging
	}

	return (0);
}