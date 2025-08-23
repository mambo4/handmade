//
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "xaudio2.lib")

// these #defines re use 'static' with more clarfied intent
#define internal static
#define local_persist static
#define global_variable static

#include <Windows.h>
#include <stdint.h>		// for access to unit8_t type
#include <xinput.h>		// for xbox controller
#include <xaudio2.h>	// for audio
#include <combaseapi.h> // to intilize COM for Xaudio2




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

typedef int32 bool32;
#include "handmade.cpp"

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

#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState) // macro creates fns 'name' with the signature args
typedef X_INPUT_GET_STATE(x_input_get_state);											   // create a 'type' x_input_get_state(DWORD dwUserIndex, XINPUT_STATE *pState)
X_INPUT_GET_STATE(XInputGetStateStub)													   // create XInputGetStateStub(DWORD dwUserIndex, XINPUT_STATE *pState){ return(0); }
{
	return (ERROR_DEVICE_NOT_CONNECTED);
}
global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub; // define a pointer to XInputGetStateStub
#define XInputGetState XInputGetState_									 // call the pointer 'XInputGetState'

#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);

X_INPUT_SET_STATE(XInputSetStateStub)
{
	return (ERROR_DEVICE_NOT_CONNECTED);
}
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

// globals
global_variable bool GlobalRunning;
global_variable win32_offscreen_buffer GlobalBackBuffer;
global_variable IXAudio2 *pXAudio2 = nullptr;
global_variable IXAudio2MasteringVoice *pMasterVoice = nullptr;
global_variable HRESULT hr;
global_variable bool32 soundIsPlaying = false;
//assets
global_variable TCHAR *boopFile = TEXT("W:/handmade/assets/audio/bip.wav");
global_variable TCHAR *bipFile = TEXT("W:/handmade/assets/audio/boop.wav");

// consts
const float PI = 3.14159265359;
const uint32 STARTCOLOR = 0x00000000;
// https://pages.mtu.edu/~suits/notefreqs.html

internal void Win32LoadXInput(void)
{
	HMODULE XInputLibrary = LoadLibraryA("XInput1_4.dll");
	if (XInputLibrary)
	{
		XInputGetState = (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
		XInputSetState = (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
	}
	else
	{
		// todo: diagnostic
	}
}

win32_window_dimension Win32GetWindowDimension(HWND Window)
{

	RECT ClientRect;
	win32_window_dimension Result;
	GetClientRect(Window, &ClientRect);
	Result.Width = ClientRect.right - ClientRect.left;
	Result.Height = ClientRect.bottom - ClientRect.top;
	return (Result);
}

internal void RenderGrid(win32_offscreen_buffer *Buffer, int XOffset, int YOffset, int RedOffset)
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

			const int X_COORD = X - X_ORIGIN + XOffset;
			const int Y_COORD = Y - Y_ORIGIN + YOffset;

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

				if (Y + YOffset == Y_ORIGIN || X + XOffset == X_ORIGIN)
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
	Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

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

internal void Win32RumbleController(uint16 speed)
{
	// speed is normally 1000
	XINPUT_VIBRATION Vibration;
	Vibration.wLeftMotorSpeed = speed;
	Vibration.wRightMotorSpeed = speed;
	XInputSetState(0, &Vibration);
}

/***************************************************
XAudio2 stuff
***************************************************/

internal void Win32InitXaudio2()
{
	HRESULT hResult;
	hResult = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	if (SUCCEEDED(hResult))
	{
		OutputDebugStringA("CoInitializeEx() SUCCEEDED.");

		if (SUCCEEDED(hResult = XAudio2Create(&pXAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR)))
		{
			OutputDebugStringA("XAudio2Create() SUCCEEDED.");
			if (SUCCEEDED(hResult = pXAudio2->CreateMasteringVoice(&pMasterVoice)))
			{
				OutputDebugStringA("CreateMasteringVoice() SUCCEEDED.");
			}
			else
			{
				OutputDebugStringA("CreateMasteringVoice() FAILED");
			}
		}
		else
		{
			OutputDebugStringA("XAudio2Create() FAILED.");
		}
	}
	else
	{
		OutputDebugStringA("CoInitializeEx() FAILED");
	}
}

#ifdef _XBOX // Big-Endian
#define fourccRIFF 'RIFF'
#define fourccDATA 'data'
#define fourccFMT 'fmt '
#define fourccWAVE 'WAVE'
#define fourccXWMA 'XWMA'
#define fourccDPDS 'dpds'
#endif

#ifndef _XBOX // Little-Endian
#define fourccRIFF 'FFIR'
#define fourccDATA 'atad'
#define fourccFMT ' tmf'
#define fourccWAVE 'EVAW'
#define fourccXWMA 'AMWX'
#define fourccDPDS 'sdpd'
#endif

HRESULT FindChunk(HANDLE hFile, DWORD fourcc, DWORD &dwChunkSize, DWORD &dwChunkDataPosition)
{
    HRESULT hResult = S_OK;
    if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, 0, NULL, FILE_BEGIN)) {
        return HRESULT_FROM_WIN32(GetLastError());
	}
    DWORD dwChunkType;
    DWORD dwChunkDataSize;
    DWORD dwRIFFDataSize = 0;
    DWORD dwFileType;
    DWORD bytesRead = 0;
    DWORD dwOffset = 0;

    while (hResult == S_OK)
    {
        DWORD dwRead;
        if (0 == ReadFile(hFile, &dwChunkType, sizeof(DWORD), &dwRead, NULL)){
            hResult = HRESULT_FROM_WIN32(GetLastError());
		}

        if (0 == ReadFile(hFile, &dwChunkDataSize, sizeof(DWORD), &dwRead, NULL)){
            hResult = HRESULT_FROM_WIN32(GetLastError());
		}

		switch (dwChunkType)
        {
        case fourccRIFF:
            dwRIFFDataSize = dwChunkDataSize;
            dwChunkDataSize = 4;
            if (0 == ReadFile(hFile, &dwFileType, sizeof(DWORD), &dwRead, NULL))
                hResult = HRESULT_FROM_WIN32(GetLastError());
            break;
        default:
            if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, dwChunkDataSize, NULL, FILE_CURRENT))
                return HRESULT_FROM_WIN32(GetLastError());
        }
        dwOffset += sizeof(DWORD) * 2;
        if (dwChunkType == fourcc)
        {
            dwChunkSize = dwChunkDataSize;
            dwChunkDataPosition = dwOffset;
            return S_OK;
        }
        dwOffset += dwChunkDataSize;
        if (bytesRead >= dwRIFFDataSize)
            return S_FALSE;
    }
    return S_OK;
}

HRESULT ReadChunkData(HANDLE hFile, void *buffer, DWORD buffersize, DWORD bufferoffset)
{
    HRESULT hResult = S_OK;
    if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, bufferoffset, NULL, FILE_BEGIN))
    {
		OutputDebugStringA("ReadChunkData() INVALID_SET_FILE_POINTER\n");
		return HRESULT_FROM_WIN32(GetLastError());
    }
    DWORD dwRead;
    if (0 == ReadFile(hFile, buffer, buffersize, &dwRead, NULL))
    {
        OutputDebugStringA("ReadFile");
        hResult = HRESULT_FROM_WIN32(GetLastError());
    }
    return hResult;
}

class VoiceCallback : public IXAudio2VoiceCallback
{
public:

	void STDMETHODCALLTYPE OnBufferStart(void *) override
	{
		soundIsPlaying = false;
	}
	void STDMETHODCALLTYPE OnBufferEnd(void *pBufferContext) override
	{
		// Audio buffer finished playing
		soundIsPlaying = false; // set global soundIsPlaying to false
	}
	// Implement other methods as empty
	void STDMETHODCALLTYPE OnVoiceProcessingPassStart(UINT32) override {}
	void STDMETHODCALLTYPE OnVoiceProcessingPassEnd() override {}
	void STDMETHODCALLTYPE OnStreamEnd() override {}

	void STDMETHODCALLTYPE OnLoopEnd(void *) override {}
	void STDMETHODCALLTYPE OnVoiceError(void *, HRESULT) override {}
};

HRESULT playAudio(
    TCHAR *strFileName,
    IXAudio2 *pXAudio2, 
    IXAudio2MasteringVoice *pMasterVoice,
    WAVEFORMATEXTENSIBLE wfx,
    XAUDIO2_BUFFER buffer,
    HRESULT hResult
    )
	/*
	Back to episode 008 to try the bespoke square wave to buffer.
	*/
{
	hResult=S_OK;
    HANDLE hFile = CreateFile(
        strFileName,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        0,
        NULL);

    if (INVALID_HANDLE_VALUE == hFile)
    {
		char errorMsg[256];
		wsprintfA(errorMsg, "playAudio(): INVALID_HANDLE_VALUE %s\n", strFileName);
		OutputDebugStringA(errorMsg);
		return S_FALSE;
	}

    if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, 0, NULL, FILE_BEGIN))
    {
        OutputDebugStringA("playAudio(): INVALID_SET_FILE_POINTER\n");
		return S_FALSE;
	}

    DWORD dwDataSize;
    BYTE *pDataBuffer = new BYTE[dwDataSize];

	buffer.AudioBytes = dwDataSize;      // size of the audio buffer in bytes
	buffer.pAudioData = pDataBuffer;      // buffer containing audio data
    buffer.Flags = XAUDIO2_END_OF_STREAM; // tell the source voice not to expect any data after this buffer

    IXAudio2SourceVoice *pSourceVoice;
    if (FAILED(hResult = pXAudio2->CreateSourceVoice(&pSourceVoice, (WAVEFORMATEX *)&wfx)))
    {
		OutputDebugStringA("playAudio() : FAILED pXAudio2->CreateSourceVoice(&pSourceVoice, (WAVEFORMATEX*)&wfx)");
		return hResult;
	}

    if (FAILED(hResult = pSourceVoice->SubmitSourceBuffer(&buffer)))
    {
		OutputDebugStringA("playAudio() : FAILED pSourceVoice->SubmitSourceBuffer(&buffer)");
		return hResult;
	}

    if (FAILED(hResult = pSourceVoice->Start(0)))
    {
		OutputDebugStringA("playAudio() : FAILED pSourceVoice->Start(0)");
		return hResult;
	}else{
		OutputDebugStringA("playAudio() : ");
		OutputDebugStringA((LPCSTR)(strFileName));
		OutputDebugStringA("\n");
	}

	return hResult;
}

HRESULT playAudioFile(
	TCHAR *strFileName,
	IXAudio2 *pXAudio2,
	IXAudio2MasteringVoice *pMasterVoice,
	WAVEFORMATEXTENSIBLE wfx,
	XAUDIO2_BUFFER buffer,
	HRESULT hResult)
{
	hResult = S_OK;
	HANDLE hFile = CreateFile(
		strFileName,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		0,
		NULL);

	if (INVALID_HANDLE_VALUE == hFile)
	{
		char errorMsg[256];
		wsprintfA(errorMsg, "playAudio(): INVALID_HANDLE_VALUE %s\n", strFileName);
		OutputDebugStringA(errorMsg);
		return S_FALSE;
	}

	if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, 0, NULL, FILE_BEGIN))
	{
		OutputDebugStringA("playAudio(): INVALID_SET_FILE_POINTER\n");
		return S_FALSE;
	}

	DWORD dwChunkSize;
	DWORD dwChunkPosition;

	// check the file type, should be fourccWAVE or 'XWMA'
	FindChunk(hFile, fourccRIFF, dwChunkSize, dwChunkPosition);
	DWORD filetype;
	ReadChunkData(hFile, &filetype, sizeof(DWORD), dwChunkPosition);
	if (filetype != fourccWAVE)
	{
		OutputDebugStringA("playAudio(): filetype != fourccWAVE\n");
		return S_FALSE;
	}

	FindChunk(hFile, fourccFMT, dwChunkSize, dwChunkPosition);
	ReadChunkData(hFile, &wfx, dwChunkSize, dwChunkPosition);

	FindChunk(hFile, fourccDATA, dwChunkSize, dwChunkPosition);
	BYTE *pDataBuffer = new BYTE[dwChunkSize];
	ReadChunkData(hFile, pDataBuffer, dwChunkSize, dwChunkPosition);

	buffer.AudioBytes = dwChunkSize;	  // size of the audio buffer in bytes
	buffer.pAudioData = pDataBuffer;	  // buffer containing audio data
	buffer.Flags = XAUDIO2_END_OF_STREAM; // tell the source voice not to expect any data after this buffer
	buffer.LoopCount =1;

	IXAudio2SourceVoice *pSourceVoice;
	if (FAILED(hResult = pXAudio2->CreateSourceVoice(&pSourceVoice, (WAVEFORMATEX *)&wfx)))
	{
		OutputDebugStringA("playAudio() : FAILED pXAudio2->CreateSourceVoice(&pSourceVoice, (WAVEFORMATEX*)&wfx)");
		return hResult;
	}

	if (FAILED(hResult = pSourceVoice->SubmitSourceBuffer(&buffer)))
	{
		OutputDebugStringA("playAudio() : FAILED pSourceVoice->SubmitSourceBuffer(&buffer)");
		return hResult;
	}

	if (FAILED(hResult = pSourceVoice->Start(0)))
	{
		OutputDebugStringA("playAudio() : FAILED pSourceVoice->Start(0)");
		return hResult;
	}
	else
	{
		OutputDebugStringA("playAudio() : ");
		OutputDebugStringA((LPCSTR)(strFileName));
		OutputDebugStringA("\n");
	}

	return hResult;
}


/***************************************************
Main stuff
***************************************************/

LRESULT CALLBACK Win32MainWindowCallback(
	HWND Window,   // handle to a window
	UINT Message,  // window message we want to handle
	WPARAM WParam, // wide pointer (unsigned int)
	LPARAM LParam) // long pointer (long(32 bitr))
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

		typedef int32 bool32;
		bool32 AltKeyWasDown = ((LParam & (1 << 29)) != 0);
		if ((VKCode == VK_F4) && AltKeyWasDown)
		{
			GlobalRunning = false;
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
		Result = DefWindowProcA(Window, Message, WParam, LParam);
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
	WindowClass.lpszClassName = " ";

	//perf
	LARGE_INTEGER PerformanceFrequencyResult;
	QueryPerformanceFrequency(&PerformanceFrequencyResult);
	int64 PerCountFrequency =PerformanceFrequencyResult.QuadPart;
	
//window loop
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
			// since we specified CS_OWNDC, we can just get one device context and use it forever because we are not sharing it.
			HDC DeviceContext = GetDC(Window);

			// colors
			int XOffset = 0;
			int YOffset = 0;
			int RedOffset = 0;
			int BlueOffset = 0;

			//audio

			int SamplesPerSecond=41000;
			int SquareWaveCounter=0;
			int Hz=256;
			int SquareWavePeriod = SamplesPerSecond/Hz;

			// haptics
			uint16 VibrationSpeed = 0;

			// sound
			bool32 SoundOn = false;
			Win32InitXaudio2();
			WAVEFORMATEXTENSIBLE wfx = {0};
			XAUDIO2_BUFFER xaudioBuffer = {0};
			
			//perf
			LARGE_INTEGER LastCounter;
			QueryPerformanceCounter(&LastCounter);
			int64 LastCycleCount = __rdtsc();  

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
					XINPUT_STATE ControllerState;
					ZeroMemory(&ControllerState, sizeof(XINPUT_STATE));

					// Simply get the state of the controller from XInput.
					dwResult = XInputGetState(i, &ControllerState);
					if (dwResult == ERROR_SUCCESS)
					{
						// Controller is connected
						XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;

						bool32 Up = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
						bool32 Down = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
						bool32 Left = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
						bool32 Right = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
						bool32 Start = (Pad->wButtons & XINPUT_GAMEPAD_START);
						bool32 Back = (Pad->wButtons & XINPUT_GAMEPAD_BACK);
						bool32 AButton = (Pad->wButtons & XINPUT_GAMEPAD_A);
						bool32 BButton = (Pad->wButtons & XINPUT_GAMEPAD_B);
						bool32 XButton = (Pad->wButtons & XINPUT_GAMEPAD_X);
						bool32 YButton = (Pad->wButtons & XINPUT_GAMEPAD_Y);

						int16 LStickX = Pad->sThumbLX;
						int16 LStickY = Pad->sThumbLY;
						int16 RStickX = Pad->sThumbRX;
						int16 RStickY = Pad->sThumbRY;

						/*******************************************************
						 *  GAME LOOP :simulate/update
						 ********************************************************/
						XOffset -= LStickX /8192;
						YOffset += LStickY /8192;
						RedOffset = 0;

						if (AButton)
						{
							RedOffset = 255;
							VibrationSpeed = 0;
						}

						if (BButton)
						{
							RedOffset = 128;
							VibrationSpeed = 1000;
						}

						if (XButton || YButton)
						{
							SoundOn = true;
						}
						else
						{
							SoundOn = false;
						}

						if (XButton)
						{
							if (! soundIsPlaying)
							{
								playAudioFile(boopFile,pXAudio2,pMasterVoice,wfx,xaudioBuffer,hr);
							}
						}

						if (YButton)
						{
							if (!soundIsPlaying)
							{
								playAudioFile(bipFile, pXAudio2, pMasterVoice, wfx, xaudioBuffer, hr);
							}
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
				HDC DeviceContext = GetDC(Window);
				Win32RumbleController(VibrationSpeed);

				game_offscreen_buffer Buffer = {};
				Buffer.Memory = GlobalBackBuffer.Memory;
				Buffer.Width = GlobalBackBuffer.Width;	
				Buffer.Height = GlobalBackBuffer.Height;
				Buffer.Pitch = GlobalBackBuffer.Pitch;

				GameUpdateAndRender(&Buffer, XOffset, YOffset, RedOffset);

				// RenderWeirdGradient(&GlobalBackBuffer, XOffset, YOffset, RedOffset);
				
				win32_window_dimension Dimension = Win32GetWindowDimension(Window);
				Win32DisplayBufferInWindow(&GlobalBackBuffer, DeviceContext, Dimension.Width, Dimension.Height);
				
				//perf - before or after releaseDC?
				int64 EndCycleCount=__rdtsc();
				LARGE_INTEGER EndCounter;
				QueryPerformanceCounter(&EndCounter);
				int64 CyclesElapsed=EndCycleCount-LastCycleCount;
				int64 CounterElapsed=EndCounter.QuadPart-LastCounter.QuadPart;
				int32 MSPerFrame=(int32)((1000*CounterElapsed)/PerCountFrequency);
				int32 FPS=(PerCountFrequency/CounterElapsed);
				int32 MCPF= (int32)(CyclesElapsed/ (1000*1000));

#if 0				
				char Buffer[200];
				wsprintfA(Buffer, "mspf: %d\t fps: %d;\t mcpf: %d\n",MSPerFrame,FPS, MCPF); 
				OutputDebugStringA(Buffer);
#endif
				LastCounter=EndCounter;
				LastCycleCount=EndCycleCount;

				ReleaseDC(Window, DeviceContext);


			}
		}
		else
		{
			// todo: logging CreateWindowExA()
		}
	}
	else
	{
		// todo: logging
	}

	return (0);
}