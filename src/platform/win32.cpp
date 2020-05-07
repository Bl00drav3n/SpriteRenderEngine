#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <MMSystem.h>
#include <dsound.h>

#define DEBUG_TEST_FILL_SOUND_BUFFER 1
#define DEBUG_WIN32_USE_FULLSCREEN 0

#define VK_X 0x58

#pragma warning(disable : 4996)
#include <stdio.h>
#include <stdlib.h>

#include "game_platform.h"
#include "game_math.h"
#include "wgl.cpp"

typedef void __stdcall _glFinish(void);
global_persist _glFinish *glFinish;
global_persist HINSTANCE GlobalInstance;
//global_persist game_memory *GlobalGameMemory;
global_persist IDirectSoundBuffer *GlobalSoundBufferHandle;
global_persist HANDLE GlobalRecordHandle = INVALID_HANDLE_VALUE;

platform_api * Platform = 0;

// TODO: Right now it's not possible to run at smooth 60FPS without getting frame stuttering. VSYNC/Timing issue?
#define TARGET_FRAMES_PER_SECOND 60
#define TARGET_FRAME_DELTA (1.f / (f32)TARGET_FRAMES_PER_SECOND)
#define VSYNC_ENABLED TRUE
#define AUDIO_LATENCY_FRAMES 3

#define VIEWPORT_WIDTH 1600
#define VIEWPORT_HEIGHT 900
#define MULTISAMPLES 16

#define SAVESTATE_VERSION 1
global_persist const char SavestateMagic[4] = { 'D', 'U', 'M', 'P' };

global_persist const char DummyWndClassName[] = "SpriteRenderDummyClass";
global_persist const char DummyWndName[] = "SpriteRenderDummy";
global_persist const char WndClassName[] = "SpriteRenderClass";
global_persist const char WndName[] = "Sprite Renderengine";
global_persist const char RecordFilename[] = "state_record.dat";

enum ERROR_CODES {
	REGISTER_ERROR = 0x1,
	CREATE_WND_ERROR,
	PIXEL_FORMAT_ERROR,
	WGL_CREATE_CONTEXT_ERROR,
	WGL_CREATE_CONTEXT_EX_ERROR,
	CPU_FREQ_ERROR,
};

global_persist debug_state *GlobalDebugState;
global_persist f32 GlobalCyclesPerSecond;
global_persist u32 Win32AllocBlockCount;
global_persist ticket_mutex Win32MemoryMutex = {};

#include "win32_sys.cpp"
#include "win32_callbacks.cpp"

struct game_dll
{
	BOOL Valid;
	char *SourceFilename;
	char *LoadedFilename;
	ULARGE_INTEGER LastWriteTime;

    HMODULE Library;
    game_update_and_render *UpdateAndRender;
};
static void LoadGameDLL(game_dll *Dll, char *SourceFilename, char *LoadedFilename, ULONGLONG LastWriteTime = 0)
{
	if(Dll->Library) {
		FreeLibrary(Dll->Library);
		// NOTE: Sleep a short while for giving the debugger some time to unload the PDB
		Sleep(500);
	}
	
	game_dll Result = {};
	Result.UpdateAndRender = GameUpdateAndRenderStub;
	Result.SourceFilename = SourceFilename;
	Result.LoadedFilename = LoadedFilename;
	Result.LastWriteTime.QuadPart = LastWriteTime;

	if(CopyFileA(SourceFilename, LoadedFilename, FALSE)) {
		HMODULE GameDLL = LoadLibraryA(LoadedFilename);
		if(GameDLL) {
			Result.Library = GameDLL;
			game_update_and_render *GameUpdateAndRender = (game_update_and_render*)GetProcAddress(GameDLL, "GameUpdateAndRender");
			if(GameUpdateAndRender) {
				Result.UpdateAndRender = GameUpdateAndRender;
				Result.Valid = TRUE;
			}
		}
	}

	*Dll = Result;
}

static BOOL AutoReloadDll(game_dll *Dll) 
{
	TIMED_FUNCTION();
	BOOL Result = FALSE;
	BOOL TriggerReload = FALSE;
	HANDLE DllHandle = CreateFileA(Dll->SourceFilename, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	ULARGE_INTEGER LastWrite;
	if(DllHandle != INVALID_HANDLE_VALUE) {
		FILETIME Time;
		if(GetFileTime(DllHandle, 0, 0, &Time)) {
			LastWrite.LowPart  = Time.dwLowDateTime;
			LastWrite.HighPart = Time.dwHighDateTime;
			TriggerReload = Dll->LastWriteTime.QuadPart < LastWrite.QuadPart;
		}
		CloseHandle(DllHandle);
	}
	if(TriggerReload) {
		// NOTE: Dll changed, try reloading
		LoadGameDLL(Dll, Dll->SourceFilename, Dll->LoadedFilename, LastWrite.QuadPart);
		Result = Dll->Valid;
	}

	return Result;
}

internal void CleanupDLL(game_dll *Game)
{
	FreeLibrary(Game->Library);
	DeleteFileA(Game->LoadedFilename);
	Game->Valid = FALSE;
}

void WINAPI GameLoop(void *GameData)
{	
	while(TRUE) {
		process_data *ProcessData = (process_data*)GameData;
		game_dll *Game = ProcessData->Game;
		game_memory *Memory = ProcessData->Memory;
		HWND MainWindow = ProcessData->Window;
		HDC WindowDC = ProcessData->WindowDC;
		cmd_pipe *Pipe = ProcessData->Pipe;
		b32 DllReloaded = AutoReloadDll(Game);

		// NOTE: handle window changes
		RECT WndClientRect = {};
		GetClientRect(MainWindow, &WndClientRect);
//		Assert(WndClientRect.right - WndClientRect.left > 0);
//		Assert(WndClientRect.bottom - WndClientRect.top > 0);

		LONG WindowClientWidth = WndClientRect.right - WndClientRect.left;
		LONG WindowClientHeight = WndClientRect.bottom - WndClientRect.top;

        window_params WindowParams = {};
        WindowParams.Width = (u32)WindowClientWidth;
        WindowParams.Height = (u32)WindowClientHeight;
#ifdef GAME_DEBUG
		DebugInputSwapBuffers();
		GameInputs.DebugInputBuffer = GlobalDebugInput.Buffers[GlobalDebugInput.ReadBuffer];
		GameInputs.DebugInputBufferSize = GlobalDebugInput.Sizes[GlobalDebugInput.ReadBuffer];
#endif
        game_inputs Inputs = GameInputs;
        for(u32 i = 0; i < NUM_BUTTONS; i++) {
			GameInputs.HalfTransitionCount[i] = 0;
        }

        POINT CursorP;
        GetCursorPos(&CursorP);
        ScreenToClient(MainWindow, &CursorP);
        Inputs.MouseX = (f32)CursorP.x / (f32)(WindowParams.Width - 1);
		Inputs.MouseY = (f32)(WindowParams.Height - CursorP.y - 1) / (f32)(WindowParams.Height - 1);
		
		DWORD ByteToLock;
		DWORD BytesToWrite;
		DWORD PlayCursor, WriteCursor;
		BOOL AudioValid = 0;
		if(SUCCEEDED(GlobalAudioOutput.SoundBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor))) {
			TIMED_BLOCK("Win32GetSoundCursorPos");
			ByteToLock = (GlobalAudioOutput.RunningSampleIndex * GlobalAudioOutput.BytesPerSample) % GlobalAudioOutput.BufferSize;
			DWORD TargetCursor = (PlayCursor + GlobalAudioOutput.LatencySampleCount * GlobalAudioOutput.BytesPerSample) % GlobalAudioOutput.BufferSize;
			if(ByteToLock > TargetCursor) {
				BytesToWrite = TargetCursor + (GlobalAudioOutput.BufferSize - ByteToLock);
			}
			else {
				BytesToWrite = TargetCursor - ByteToLock;
			}

			AudioValid = 1;
		}

		Memory->AudioBuffer->SampleCount = BytesToWrite / GlobalAudioOutput.BytesPerSample;
		
		BOOL Signaled = FALSE;
		if(WaitForSingleObject(Pipe->SignalReady, 0) == WAIT_OBJECT_0) {
			Memory->ConsoleBuffer = (u8*)Pipe->ReadBuffer;
			Memory->ConsoleBufferSize = Pipe->Size;
			Signaled = TRUE;
		}
		else {
			Memory->ConsoleBuffer = 0;
			Memory->ConsoleBufferSize = 0;
		}

        Game->UpdateAndRender(Memory, &WindowParams, &Inputs, DllReloaded);
		
		if(Memory->DebugState) {
			GlobalDebugState = Memory->DebugState;
			GlobalDebugState->CyclesPerSecond = GlobalCyclesPerSecond;
		}

		if(Signaled) {
			ResetEvent(Pipe->SignalReady);
		}

#if DEBUG_TEST_FILL_SOUND_BUFFER
		if(AudioValid) {
			TIMED_BLOCK("Win32FillSoundbuffer");
			Win32FillSoundBuffer(Memory->AudioBuffer, ByteToLock, BytesToWrite);
		}
#endif
		SwitchToFiber(ProcessData->EventFiber);
	}

	SwitchToFiber(((process_data*)GameData)->EventFiber);
}

int CALLBACK
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
LPSTR lpCmdLine, int nCmdShow)
{
	cmd_pipe Pipe = {};
	if(!CreateConsolePipe(&Pipe)) {
		return -1;
	}

	GlobalInstance = hInstance;

	// NOTE: Create an dummy window for OpenGL context
	WNDCLASSEXA DummyWndClass = {};
	DummyWndClass.cbSize = sizeof(WNDCLASSEX);
	DummyWndClass.style = CS_OWNDC;
	DummyWndClass.lpfnWndProc = DefWindowProcA;
	DummyWndClass.hInstance = hInstance;
	DummyWndClass.lpszClassName = DummyWndClassName;
	RegisterClassExA(&DummyWndClass);

	HWND DummyWindow = CreateWindowA(DummyWndClassName, 
		DummyWndName,
		WS_DISABLED | WS_MINIMIZE,
		CW_USEDEFAULT, CW_USEDEFAULT, 
		CW_USEDEFAULT, CW_USEDEFAULT, 
		nullptr, 
		nullptr, 
		hInstance, 
		nullptr);

	// NOTE: Set up OGL dummy context
	PIXELFORMATDESCRIPTOR pfd = {
		sizeof(PIXELFORMATDESCRIPTOR),
		1,
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
		PFD_TYPE_RGBA,
		32,
		0, 0, 0, 0, 0, 0,
		0,
		0,
		0,
		0, 0, 0, 0,
		24,
		8,
		0,
		PFD_MAIN_PLANE,
		0,
		0, 0, 0
	};

	HDC DummyWindowDC = GetDC(DummyWindow);
	int PixelFormat = ChoosePixelFormat(DummyWindowDC, &pfd);
	SetPixelFormat(DummyWindowDC, PixelFormat, &pfd);

	HGLRC GLDummyContext = wglCreateContext(DummyWindowDC);
	wglMakeCurrent(DummyWindowDC, GLDummyContext);

	// NOTE: Create main window and setup real GL context

	WNDCLASSEXA WndClass = {};
	WndClass.cbSize = sizeof(WNDCLASSEX);
	WndClass.style = CS_OWNDC;
	WndClass.lpfnWndProc = WindowProc;
	WndClass.hInstance = hInstance;
	WndClass.lpszClassName = WndClassName;
    WndClass.hCursor = LoadCursor(0, IDC_ARROW);
	RegisterClassExA(&WndClass);
	
	// TODO: For debug and/or convenience reasons, provide a way to
	// quickly switch between fullscreen and windowed. (need to fix stutter in window mode)

	RECT ClientRect = { 0, 0, VIEWPORT_WIDTH, VIEWPORT_HEIGHT };
	DWORD WindowStyle = WS_OVERLAPPEDWINDOW;
	AdjustWindowRect(&ClientRect, WindowStyle, FALSE);
	HWND MainWindow = CreateWindowA(WndClassName,
		WndName, 
		WindowStyle,
		CW_USEDEFAULT, CW_USEDEFAULT, 
		ClientRect.right - ClientRect.left, ClientRect.bottom - ClientRect.top, 
		nullptr, 
		nullptr, 
		hInstance, 
		nullptr);

	HDC WindowDC = GetDC(MainWindow);

	wglGetExtensionsStringARB = (_wglGetExtensionsStringARB*)wglGetProcAddress("wglGetExtensionsStringARB");
	if(wglGetExtensionsStringARB) {
		char * Extensions = (char*)wglGetExtensionsStringARB(WindowDC);
		if(Extensions) {
			WGL_Extensions.ExtensionsString = Extensions;
			WGL_Extensions.Valid = TRUE;
			LoadExtensionsWGL();
		}

		if(ExtensionSupported("WGL_ARB_pixel_format")) {
			UINT NumFormats;
			int *Attributes;
			int AttributesWithMultisamples[] = {
					WGL_SAMPLE_BUFFERS_ARB, 1,
					WGL_SAMPLES_ARB, MULTISAMPLES,

					WGL_DRAW_TO_WINDOW_ARB, 1,
					WGL_SUPPORT_OPENGL_ARB, 1,
					WGL_DOUBLE_BUFFER_ARB, 1,
					WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
					WGL_COLOR_BITS_ARB, 32,
					0
				};

#if 0
			if(ExtensionSupported("WGL_ARB_multisample")) {
				Attributes = AttributesWithMultisamples;
			}
			else {
				Attributes = AttributesWithMultisamples + 4;
			}
#endif
		    Attributes = AttributesWithMultisamples + 4;
		
			wglChoosePixelFormatARB = (_wglChoosePixelFormatARB*)wglGetProcAddress("wglChoosePixelFormatARB");
			wglChoosePixelFormatARB(WindowDC, Attributes, 0, 1, &PixelFormat, &NumFormats);
			DescribePixelFormat(WindowDC, PixelFormat, sizeof(pfd), &pfd);
			SetPixelFormat(WindowDC, PixelFormat, &pfd);
			GlobalContextData.PixelFormat = PixelFormat;
		}

		if(ExtensionSupported("WGL_ARB_create_context")) {
			wglCreateContextAttribsARB = (_wglCreateContextAttribsARB*)wglGetProcAddress("wglCreateContextAttribsARB");
		}
	}
	
	wglDeleteContext(GLDummyContext);
	ReleaseDC(DummyWindow, DummyWindowDC);
	DestroyWindow(DummyWindow);

	// NOTE: Create the real GL context
	HGLRC GLContext = 0;
	if(wglCreateContextAttribsARB) {
		// TODO: Remove all non-core functionality!
		int MajorVersion = 3;
		int MinorVersion = 3;
#if 0
		int ProfileMask = WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB;
#else
        int ProfileMask = WGL_CONTEXT_CORE_PROFILE_BIT_ARB;
#endif
        int ContextFlags = 0;
#ifdef GAME_DEBUG
        ContextFlags |= WGL_CONTEXT_DEBUG_BIT_ARB;
#endif

		int ContextAttribs[] = {
			WGL_CONTEXT_MAJOR_VERSION_ARB, MajorVersion,
			WGL_CONTEXT_MINOR_VERSION_ARB, MinorVersion,
			WGL_CONTEXT_PROFILE_MASK_ARB, ProfileMask,
            WGL_CONTEXT_FLAGS_ARB, ContextFlags,
			0
		};
		GLContext = wglCreateContextAttribsARB(WindowDC, 0, ContextAttribs);
	}

	// NOTE: Fallback default GL context
	if(!GLContext) {
		GLContext = wglCreateContext(WindowDC);
	}

	wglMakeCurrent(WindowDC, GLContext);
	wglSwapInterval = (_wglSwapInterval*)wglGetProcAddress("wglSwapIntervalEXT");
	if(wglSwapInterval) {
#if VSYNC_ENABLED
		wglSwapInterval(1);
#else
		wglSwapInterval(0);
#endif
	}

	wglSwapBuffers = (_wglSwapBuffers*)Win32GetProcAddressGL("wglSwapBuffers");
	glFinish = (_glFinish*)Win32GetProcAddressGL("glFinish");

	LARGE_INTEGER LastCounter;
	LARGE_INTEGER CPUFreq;
	QueryPerformanceFrequency(&CPUFreq);

	ShowWindow(MainWindow, nCmdShow);
	UpdateWindow(MainWindow);

	// NOTE: Initialize audio
	win32_init_audio_desc AudioDesc = {};
	AudioDesc.BytesPerSample = sizeof(s16);
	AudioDesc.Channels = 2;
	AudioDesc.SamplesPerSecond = 44100;
	AudioDesc.BufferSize = AudioDesc.Channels * AudioDesc.SamplesPerSecond * AudioDesc.BytesPerSample;
	Win32InitAudio(MainWindow, &AudioDesc);

	// NOTE: Setup memory
	game_memory Memory = {};		
	u32 StackSize = MEGABYTES(16);
	Memory.StackMemory = VirtualAlloc(0, StackSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	Memory.StackMemorySize = StackSize;
	
#ifdef GAME_DEBUG
	{ // NOTE: Quick measurement of rdtsc cycle frequency
		LARGE_INTEGER CurCounter;
		QueryPerformanceCounter(&LastCounter);
		u64 BeginCycles = GetCycleCount();
		do {
			QueryPerformanceCounter(&CurCounter);
			YieldProcessor();
		} while((f32)(CurCounter.QuadPart - LastCounter.QuadPart) / (f32)CPUFreq.QuadPart < 0.5f);
		u64 EndCycles = GetCycleCount();
		f32 MeasuredTime = (f32)(CurCounter.QuadPart - LastCounter.QuadPart) / (f32)CPUFreq.QuadPart;
		GlobalCyclesPerSecond = (f32)(EndCycles - BeginCycles) / MeasuredTime;
	}
#else
	Memory.DebugMemory = 0;
	Memory.DebugMemorySize = 0;
#endif

	game_audio_buffer AudioBuffer = {};
	AudioBuffer.SampleCount = 0;
	AudioBuffer.Samples = (s16*)VirtualAlloc(0, AudioDesc.BufferSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	AudioBuffer.SamplesPerSecond = AudioDesc.SamplesPerSecond;
	Memory.AudioBuffer = &AudioBuffer;
	
	DWORD LastError = GetLastError();
	// Map music file to process memory space
	debug_game_audio DebugAudio = {};
	HANDLE DebugAudioFile = CreateFileA("data/audio/music.dat", GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if(DebugAudioFile != INVALID_HANDLE_VALUE) {
		HANDLE FileMapping = CreateFileMappingA(DebugAudioFile, 0, PAGE_READONLY, 0, 0, 0);
		if(FileMapping) {
			void *Data = MapViewOfFile(FileMapping, FILE_MAP_READ, 0, 0, 0);
			if(Data) {
				DebugAudio.Header = (audio_data_header*)Data;
				DebugAudio.Valid = true;
			}
			else {
				OutputDebugStringA("Could not map view of audio file\n");
			}
		}
		else {
			OutputDebugStringA("Could not create file mapping for audio file\n");
		}
	}
	else {
		OutputDebugStringA("Could not open audio file\n");
	}
	LastError = GetLastError();

	Memory.DebugGameAudio = &DebugAudio;
	
	Platform = &Memory.PlatformAPI;
	Platform->LoadProcAddressGL = Win32GetProcAddressGL;
	Platform->SendCommand = Win32SendCommand;
	Platform->OpenFile = Win32OpenFile;
	Platform->ReadEntireFile = Win32ReadEntireFile;
	Platform->CloseFile = Win32CloseFile;
	Platform->WriteFile = Win32WriteFile;
	Platform->AllocateMemory = Win32AllocateMemory;
	Platform->DeallocateMemory = Win32DeallocateMemory;

	FILE *DebugFontFile = fopen("data/fonts/vera.fnt", "rb");
	if(DebugFontFile) {
		size_t bytes_read = fread(&Memory.DebugFont, 1, sizeof(ascii_font), DebugFontFile);
		Assert(bytes_read == sizeof(ascii_font));
		fclose(DebugFontFile);
	}

	char *SourceNameDLL = "game.dll";
	char *LoadedNameDLL = "game_loaded.dll";
	game_dll Game = {};
	LoadGameDLL(&Game, SourceNameDLL, LoadedNameDLL, 0);

	GameInputs.TimeStep = TARGET_FRAME_DELTA;
	GameInputs.Seed = GetTickCount();
#ifdef GAME_DEBUG
	DebugInputInit();
#endif
	
	process_data ProcessData = {};
	ProcessData.EventFiber = ConvertThreadToFiber((void*)&ProcessData);
	ProcessData.GameFiber = CreateFiber(0, GameLoop, (void*)&ProcessData);
	ProcessData.CPUFreq = CPUFreq;
	ProcessData.Game = &Game;
	ProcessData.Memory = &Memory;
	ProcessData.LastFrameTime = 0.0;
	ProcessData.Window = MainWindow;
	ProcessData.WindowDC = WindowDC;
	ProcessData.GLContext = GLContext;
	ProcessData.Pipe = &Pipe;
	GlobalPipe = &Pipe;
	GlobalGameFiber = ProcessData.GameFiber;
	GlobalProcData = &ProcessData;

	HANDLE ReadPipeThread = CreateThread(0, 0, ReadPipeThreadProc, &Pipe, 0, 0);
	
	Win32PlayAudio(TRUE);
	while(TRUE) {
		LARGE_INTEGER StartCounter;
		QueryPerformanceCounter(&StartCounter);
		DebugBeginFrame();
		MSG Message;
		{
			TIMED_BLOCK("Win32MessageLoop");
			while (PeekMessage(&Message, NULL, 0, 0, PM_REMOVE)) {
				TranslateMessage(&Message);
				DispatchMessageA(&Message);
			
				if (Message.message == WM_QUIT) {
					DestroyWindow(ProcessData.Window);
					CleanupDLL(&Game);
					Win32PlayAudio(FALSE);
					return 0;
				}
			}
		}

		RECT Rect = {};
		GetClientRect(ProcessData.Window, &Rect);
		SwitchToFiber(ProcessData.GameFiber);
		{
			TIMED_BLOCK("Win32SwapBuffers");
			SwapBuffers(ProcessData.WindowDC);
			//wglSwapBuffers(ProcessData.WindowDC);
			//glFinish();
		}

#if 1
		{
			TIMED_BLOCK("Win32BusyLoop");
			// NOTE: Busy wait for next frame
			// TODO: Find something better than busy wait?
			LARGE_INTEGER CurCounter;
			LONGLONG TargetCounter = StartCounter.QuadPart + ProcessData.CPUFreq.QuadPart / TARGET_FRAMES_PER_SECOND;
			do {
				QueryPerformanceCounter(&CurCounter);
				YieldProcessor();
			} while (CurCounter.QuadPart < TargetCounter);
			
			double TimeDelta = (double)(CurCounter.QuadPart - StartCounter.QuadPart) / (double)ProcessData.CPUFreq.QuadPart;
			ProcessData.LastFrameTime = TimeDelta;
		}
#endif

		DebugEndFrame();
	}

	return 0;
}
