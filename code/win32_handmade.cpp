

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

struct win32_offscreen_buffer{
	BITMAPINFO Info;
	void *Memory;
	int Width;
	int Height;
	int Pitch;
	int BytesPerPixel;
};

struct win32_window_dimension{
	int Width;
	int Height;
};

win32_window_dimension Win32GetWindowDimension(HWND Window){

	RECT ClientRect;
	win32_window_dimension Result;
	GetClientRect(Window, &ClientRect);
	Result.Width = ClientRect.right - ClientRect.left;
	Result.Height = ClientRect.bottom - ClientRect.top;
	return(Result);
	
}

global_variable bool Running;
global_variable win32_offscreen_buffer GlobalBackBuffer;

internal void RenderWeirdGradient(win32_offscreen_buffer Buffer, int BlueOffset,int GreenOffset)
{
	uint8 *Row = (uint8 *)Buffer.Memory; // cast the void pointer BitmapMemory to unsigned 8 bit int
	for (int Y = 0; Y < Buffer.Height; ++Y)
	{
		//uint32 *Pixel = (uint32 *)Row; //pointer to first pixel of Row
		uint8 *Pixel = (uint8 *)Row; //pointer to first byte of first pixel of Row

		for (int X = 0; X <Buffer.Width; ++X) 
		{

			*Pixel = (uint8)(X+BlueOffset); //Blue
			++Pixel;

			*Pixel = (uint8)(Y+GreenOffset); //Green
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
	Buffer->BytesPerPixel = 4;

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

internal void Win32DisplayBufferInWindow(HDC DeviceContext, int WindowWidth, int WindowHeight, win32_offscreen_buffer Buffer, int x, int y, int Width, int Height)
{
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
		win32_window_dimension Dimension = Win32GetWindowDimension(Window);
		Win32ResizeDIBSection(&GlobalBackBuffer, Dimension.Width, Dimension.Height);
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
		win32_window_dimension Dimension=Win32GetWindowDimension(Window);
		Win32DisplayBufferInWindow(DeviceContext, Dimension.Width, Dimension.Height, GlobalBackBuffer, X, Y, Width, Height);
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
			int BlueOffset=0;
			int GreenOffset=0;

			Running = true;
			while (Running)
			{
				//
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
				RenderWeirdGradient(GlobalBackBuffer,BlueOffset,GreenOffset);

				HDC DeviceContext = GetDC(Window);
				win32_window_dimension Dimension= Win32GetWindowDimension(Window);
				Win32DisplayBufferInWindow(DeviceContext, Dimension.Width, Dimension.Height, GlobalBackBuffer, 0, 0,  Dimension.Width, Dimension.Height);
				ReleaseDC(Window,DeviceContext);

				++BlueOffset;
				++GreenOffset;
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