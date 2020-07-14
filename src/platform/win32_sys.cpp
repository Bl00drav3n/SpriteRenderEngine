#define WIN32_MEMORY_BLOCK_GUARDIAN 0xAABBCCDDFFEEDDCC

struct gl_context_data
{
	int PixelFormat;
};
global_persist gl_context_data GlobalContextData;

enum win32_memory_block_flags
{
	MemoryBlock_DeleteOnStateLoad = 1,
};

struct win32_memory_block
{
	platform_memory_block Block;
	u64 Guardian;

	win32_memory_block * Prev;
	win32_memory_block * Next;
	u32 Flags;
};

global_persist win32_memory_block Win32MemorySentinel = {
	{},
	WIN32_MEMORY_BLOCK_GUARDIAN,
	&Win32MemorySentinel,
	&Win32MemorySentinel
};

void Win32AppendMemoryBlock(win32_memory_block *MemBlock)
{
	BeginTicketMutex(&Win32MemoryMutex);
	MemBlock->Next = &Win32MemorySentinel;
	MemBlock->Prev = Win32MemorySentinel.Prev;
	MemBlock->Next->Prev = MemBlock;
	MemBlock->Prev->Next = MemBlock;
	EndTicketMutex(&Win32MemoryMutex);
}

void Win32RemoveMemoryBlock(win32_memory_block *MemBlock)
{
	BeginTicketMutex(&Win32MemoryMutex);
	MemBlock->Next->Prev = MemBlock->Prev;
	MemBlock->Prev->Next = MemBlock->Next;
	EndTicketMutex(&Win32MemoryMutex);
}

struct cmd_pipe;
struct game_dll;
struct process_data
{
	LPVOID EventFiber;
	LPVOID GameFiber;
	
	HWND Window;
	HDC WindowDC;
	HGLRC GLContext;
	LARGE_INTEGER CPUFreq;
	//LARGE_INTEGER LastCounter;
	f64 LastFrameTime;
	
	cmd_pipe *Pipe;
	game_dll *Game;
	game_memory *Memory;
};

global_persist process_data *GlobalProcData;

#ifdef GAME_DEBUG

global_persist struct {
	u32 ReadBuffer;
	u32 WriteBuffer;
	u32 Sizes[2];
	u8 Buffers[2][DEBUG_INPUT_BUFFER_MAX_SIZE];
} GlobalDebugInput;

internal void DebugInputInit()
{
	GlobalDebugInput.ReadBuffer = 0;
	GlobalDebugInput.WriteBuffer = 1;
}

internal void DebugInputPutChar(u8 Char)
{
	u32 WriteBuffer = GlobalDebugInput.WriteBuffer;
	u32 Size = GlobalDebugInput.Sizes[WriteBuffer];
	u8 *Buffer = GlobalDebugInput.Buffers[WriteBuffer];
	if(Size < DEBUG_INPUT_BUFFER_MAX_SIZE) {
		Buffer[Size++] = Char;
		GlobalDebugInput.Sizes[WriteBuffer] = Size;
	}
}

internal void DebugInputSwapBuffers()
{
	u32 WriteBuffer = GlobalDebugInput.ReadBuffer;
	GlobalDebugInput.Sizes[WriteBuffer] = 0;
	GlobalDebugInput.WriteBuffer = WriteBuffer;
	GlobalDebugInput.ReadBuffer = 1 - WriteBuffer;
}

internal void DebugInputHandleCharacterMessage(WPARAM wParam, LPARAM lParam)
{
	u16 UTF16Char = (u16)wParam;
	u8 Char = (u8)UTF16Char;
	DebugInputPutChar(Char);
}

#endif

#define READ_BUFFER_SIZE KILOBYTES(64)
struct cmd_pipe
{	
	HANDLE PipeParentRead;
	HANDLE PipeParentWrite;
	HANDLE PipeChildRead;
	HANDLE PipeChildWrite;
	HANDLE SignalReady;

	u32 Size;
	char ReadBuffer[READ_BUFFER_SIZE + 1];
};

global_persist cmd_pipe *GlobalPipe;

BOOL CreateConsolePipe(cmd_pipe *Result)
{
	HANDLE PipeParentRead = INVALID_HANDLE_VALUE;
	HANDLE PipeParentWrite = INVALID_HANDLE_VALUE;
	HANDLE PipeChildRead = INVALID_HANDLE_VALUE;
	HANDLE PipeChildWrite = INVALID_HANDLE_VALUE;
	HANDLE SignalReady = 0;
	SECURITY_ATTRIBUTES SecurityAttr = { sizeof(SECURITY_ATTRIBUTES) };
	SecurityAttr.bInheritHandle = TRUE;

	CreatePipe(&PipeParentRead, &PipeChildWrite, &SecurityAttr, READ_BUFFER_SIZE);
	SetHandleInformation(PipeParentRead, HANDLE_FLAG_INHERIT, 0);
	CreatePipe(&PipeChildRead, &PipeParentWrite, &SecurityAttr, READ_BUFFER_SIZE);
	SetHandleInformation(PipeParentWrite, HANDLE_FLAG_INHERIT, 0);

	SignalReady = CreateEventA(0, TRUE, FALSE, 0);
	if(!SignalReady) {
		OutputDebugStringA("Could not create SignalReady event\n");
		return FALSE;
	}

	char CmdLine[] = "/a /k z:\\sys\\shell.bat";
	PROCESS_INFORMATION ProcInfo = {};
	STARTUPINFO StartupInfo = { sizeof(STARTUPINFO) };
	StartupInfo.hStdError = PipeChildWrite;
	StartupInfo.hStdOutput = PipeChildWrite;
	StartupInfo.hStdInput = PipeChildRead;
	StartupInfo.dwFlags = STARTF_USESTDHANDLES;

	if(!CreateProcessA("C:\\Windows\\system32\\cmd.exe", CmdLine, 0, 0, TRUE, 0, 0, 0, &StartupInfo, &ProcInfo)) {
		OutputDebugStringA("Could not create cmd.exe process\n");
		return FALSE;
	}

	CloseHandle(ProcInfo.hProcess);
	CloseHandle(ProcInfo.hThread);

	Result->PipeParentRead = PipeParentRead;
	Result->PipeParentWrite = PipeParentWrite;
	Result->PipeChildRead = PipeChildRead;
	Result->PipeChildWrite = PipeChildWrite;
	Result->SignalReady = SignalReady;

	return TRUE;
}

DWORD WINAPI ReadPipeThreadProc(LPVOID lParameter)
{
	cmd_pipe *Pipe = (cmd_pipe*)lParameter;
	for(;;) {
		if(WaitForSingleObject(Pipe->SignalReady, 0) == WAIT_TIMEOUT) {
			DWORD BytesRead;
			BOOL Ret = ReadFile(Pipe->PipeParentRead, Pipe->ReadBuffer, READ_BUFFER_SIZE, &BytesRead, 0);
			if(Ret && BytesRead > 0) {
				Pipe->Size = BytesRead;
				Pipe->ReadBuffer[BytesRead] = 0;
				SetEvent(Pipe->SignalReady);
			}
			else {
				OutputDebugStringA("Could not read data from pipe\n");
				break;
			}
		}
	}

	return 0;
}

internal void SendCmdToPipe(cmd_pipe *Pipe, char *Cmd)
{
	char *Ptr = Cmd;
	u32 BytesToWrite = SafeTruncateU64ToU32(strlen(Cmd));
	for(;;) {
		DWORD BytesWritten;
		BOOL Ret = WriteFile(Pipe->PipeParentWrite, Ptr, BytesToWrite, &BytesWritten, 0);
		if(!Ret || !BytesWritten)
			break;
		Ptr += BytesWritten;
		BytesToWrite -= BytesWritten;
	}
}

internal void Win32PlayAudio(BOOL Enable)
{
	if(GlobalSoundBufferHandle) {
		if(Enable) {
			if(FAILED(GlobalSoundBufferHandle->Play(0, 0, DSBPLAY_LOOPING))) {		
				OutputDebugStringA("IDirectSoundBuffer::Play failed\n");
			}
		}
		else {
			if(FAILED(GlobalSoundBufferHandle->Stop())) {
				OutputDebugStringA("IDirectSoundBuffer::Stop failed\n");
			}
		}
	}
}

struct win32_stored_memory_block_header
{
	u64 MemAddr;
	u32 BlockSize;
};

internal inline platform_memory_block * GetPlatformMemoryBlock(win32_stored_memory_block_header Header)
{
	platform_memory_block *Result = (platform_memory_block*)Header.MemAddr;
	Assert(Result != 0);
	return Result;
}

internal inline BOOL Win32WriteToFile(HANDLE File, void *Buffer, u32 BufferSize)
{
	DWORD BytesWritten;
	BOOL Result = FALSE;
	if(WriteFile(File, Buffer, BufferSize, &BytesWritten, 0)) {
		if(BytesWritten == BufferSize) {
			Result = TRUE;
		}
		else {
			OutputDebugStringA("Error writing to file: not all bytes written\n");
		}
	}
	else {
		OutputDebugStringA("Error writing to file: write failed\n");
	}

	return Result;
}

internal inline BOOL Win32DebugWriteMemoryBlock(win32_stored_memory_block_header Header, HANDLE File)
{
	BOOL Result = FALSE;
	if(Win32WriteToFile(File, &Header, sizeof(Header))) {
		platform_memory_block *Block = GetPlatformMemoryBlock(Header);
		if(Win32WriteToFile(File, Block->Base, Header.BlockSize)) {
			Result = TRUE;
		}
	}

	return Result;
}

internal inline BOOL Win32DebugReadMemoryBlock(HANDLE File)
{
	BOOL Result = FALSE;
	win32_stored_memory_block_header Header = {};
	DWORD BytesRead;
	if(ReadFile(File, &Header, sizeof(Header), &BytesRead, 0)) {
		if(BytesRead == sizeof(Header)) {
			platform_memory_block *Block = GetPlatformMemoryBlock(Header);
			if(ReadFile(File, Block->Base, Header.BlockSize, &BytesRead, 0)) {
				if(BytesRead == Header.BlockSize) {
					Block->Used = Header.BlockSize;
					Result = TRUE;
				}
				else {
					OutputDebugStringA("Could not read memory block\n");
				}
			}
			else {
				OutputDebugStringA("Could not read from file\n");
			}
		}
		else {
			OutputDebugStringA("Could not read memory block header\n");
		}
	}
	else {
		OutputDebugStringA("Could not read from file\n");
	}

	return Result;
}

struct win32_savestate_header
{
	u8 Magic[4];
	u32 Version;
	u32 MemoryBlocksOffset;
	u32 MemoryBlockCount;
};

internal BOOL Win32DebugValidateSavestate(win32_savestate_header *Savestate)
{
	// TODO: Implement!
	BOOL Result = TRUE;
	return Result;
}

internal BOOL Win32DebugLoadSavestate(win32_savestate_header *Savestate, HANDLE File)
{
	BOOL Result = FALSE;
	if(SetFilePointer(File, Savestate->MemoryBlocksOffset, 0, FILE_BEGIN) != INVALID_SET_FILE_POINTER) {
		for(u32 i = 0; i < Savestate->MemoryBlockCount; i++) {
			if(Win32DebugReadMemoryBlock(File)) {
				Result = TRUE;
			}
			else {
				Result = FALSE;
				break;
			}
		}
	}

	if(Result) {
		// NOTE: Clean up unused memory blocks
		for(win32_memory_block *Block = Win32MemorySentinel.Next;
			Block != &Win32MemorySentinel;)
		{
			win32_memory_block *Next = Block->Next;
			if(Block->Flags & MemoryBlock_DeleteOnStateLoad) {
				Win32RemoveMemoryBlock(Block);
			}
			Block = Next;
		}
	}

	return Result;
}

internal void Win32DebugStoreGameState()
{
	if(GlobalRecordHandle != INVALID_HANDLE_VALUE) {
		CloseHandle(GlobalRecordHandle);
		GlobalRecordHandle = INVALID_HANDLE_VALUE;
	}
	
	if(GlobalRecordHandle == INVALID_HANDLE_VALUE) {
		GlobalRecordHandle = CreateFileA(RecordFilename,
			GENERIC_READ | GENERIC_WRITE, 
			0, 0, CREATE_ALWAYS, 
			FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_WRITE_THROUGH | FILE_FLAG_SEQUENTIAL_SCAN | FILE_FLAG_DELETE_ON_CLOSE, 0);
	}
	
	if(GlobalRecordHandle != INVALID_HANDLE_VALUE) {
		Win32PlayAudio(FALSE);
		win32_savestate_header Savestate = {};
		// TODO: Error handling
		if(Win32WriteToFile(GlobalRecordHandle, &Savestate, sizeof(Savestate))) {
			u32 BlockCount = 0;
			for(win32_memory_block *Block = Win32MemorySentinel.Next;
				Block != &Win32MemorySentinel;
				Block = Block->Next)
			{
				platform_memory_block *PlatBlock = (platform_memory_block*)Block;
				win32_stored_memory_block_header Header = {};
				Header.MemAddr = (u64)PlatBlock;
				Header.BlockSize = SafeTruncateU64ToU32(PlatBlock->Used);
				// TODO: Error handling!
				if(Win32DebugWriteMemoryBlock(Header, GlobalRecordHandle)) {
					BlockCount++;
				}
				else {
					BlockCount = 0;
					break;
				}
			}
			if(SetFilePointer(GlobalRecordHandle, 0, 0, FILE_BEGIN) != INVALID_SET_FILE_POINTER) {
				Savestate.Magic[0] = SavestateMagic[0];
				Savestate.Magic[1] = SavestateMagic[1];
				Savestate.Magic[2] = SavestateMagic[2];
				Savestate.Magic[3] = SavestateMagic[3];
				Savestate.Version = SAVESTATE_VERSION;
				Savestate.MemoryBlocksOffset = sizeof(Savestate);
				Savestate.MemoryBlockCount = BlockCount;
				if(!Win32WriteToFile(GlobalRecordHandle, &Savestate, sizeof(Savestate))) {
					OutputDebugStringA("Closing savestate handle\n");
					CloseHandle(GlobalRecordHandle);
					GlobalRecordHandle = INVALID_HANDLE_VALUE;
				}
			}
			else {
				OutputDebugStringA("Could not set file pointer to beginning of record\n");
			}
		}
		Win32PlayAudio(TRUE);
	}
}

internal void Win32DebugLoadGameState()
{	
	if(GlobalRecordHandle != INVALID_HANDLE_VALUE) {
		Win32PlayAudio(FALSE);
		if(SetFilePointer(GlobalRecordHandle, 0, 0, FILE_BEGIN) != INVALID_SET_FILE_POINTER) {
			DWORD BytesRead;
			win32_savestate_header Savestate = {};
			if(ReadFile(GlobalRecordHandle, &Savestate, sizeof(Savestate), &BytesRead, 0)) {
				if(BytesRead == sizeof(Savestate)) {
					if(Win32DebugValidateSavestate(&Savestate)) {
						Win32DebugLoadSavestate(&Savestate, GlobalRecordHandle);
					}
				}
				else {
					OutputDebugStringA("Could not read savestate header\n");
				}
			}
			else {
				OutputDebugStringA("Reading from savestate failed\n");
			}
		}
		else {
			OutputDebugStringA("Could not set file pointer to beginning of record\n");
		}
		Win32PlayAudio(TRUE);
	}
}

internal char * Win32GetFilepath(char *Name, file_type Type, void *Buffer, u32 BufferSize)
{
	string_buffer StrBuf = CreateStringBuffer(Buffer, BufferSize);
	char *SubPath = 0;
	char *FileExt = 0;
	switch(Type) {
		case FileType_Shader:
			SubPath = "data\\shaders\\";
			FileExt = ".glsl";
			break;
		case FileType_Level:
			SubPath = "data\\levels\\";
			FileExt = ".lvl";
			break;
		case FileType_Image:
			SubPath = "data\\images\\";
			FileExt = ".img";
			break;
		case FileType_Dialog:
			SubPath = "data\\dialog\\";
			FileExt = ".di";
			break;
		case FileType_Invalid:
		InvalidDefaultCase;
	}

	char *FilePath = 0;
	if(SubPath && FileExt) {
		CatString(&StrBuf, SubPath);
		CatString(&StrBuf, Name);
		CatString(&StrBuf, FileExt);
		FilePath = NullTerminateString(&StrBuf);
	}

	return FilePath;
}

global_persist game_inputs GameInputs;
internal GAME_INPUT_BUTTON GetButton(WPARAM wParam)
{
    u8 VirtualKey = (u8)(wParam & 0xff);
    GAME_INPUT_BUTTON Button = UNKNOWN_BUTTON;
    switch(VirtualKey) {
        case VK_LBUTTON:
            Button = BUTTON_MOUSE_LEFT;
            break;
        case VK_RBUTTON:
            Button = BUTTON_MOUSE_RIGHT;
            break;
        case VK_LEFT:
            Button = BUTTON_LEFT;
            break;
		case VK_RIGHT:
            Button = BUTTON_RIGHT;
            break;
		case VK_UP:
            Button = BUTTON_UP;
            break;
		case VK_DOWN:
            Button = BUTTON_DOWN;
            break;
        case VK_X:
            Button = BUTTON_ACTION;
            break;
		case VK_LSHIFT:
			Button = BUTTON_SPECIAL;
			break;
		case VK_RETURN:
			Button = BUTTON_START;
			break;
        case VK_F2:
            Button = BUTTON_DEBUG_CONSOLE;
            break;
        case VK_F3:
            Button = BUTTON_DEBUG_TOGGLE_SOUND;
            break;
		case VK_F4:
			Button = BUTTON_DEBUG_OVERLAY;
			break;
        case VK_F5:
            Button = BUTTON_DEBUG_TOGGLE_SPRITES;
            break;
		case 'N':
			Button = BUTTON_DEBUG_SINGLE_STEP;
			break;
    }

    return Button;
}

internal inline void ButtonPressed(GAME_INPUT_BUTTON Button)
{
	GameInputs.HalfTransitionCount[Button]++;
	GameInputs.State[Button] = 1;
}

internal inline void ButtonUnpressed(GAME_INPUT_BUTTON Button)
{
	GameInputs.HalfTransitionCount[Button]++;
	GameInputs.State[Button] = 0;
}

static struct win32_audio_output
{
	IDirectSoundBuffer *SoundBuffer;
	DWORD BufferSize;
	DWORD RunningSampleIndex;
	DWORD LatencySampleCount;
	DWORD BytesPerSample;
	DWORD SamplesPerSecond;
	BOOL Valid;
} GlobalAudioOutput;

struct win32_init_audio_desc
{
	DWORD SamplesPerSecond;
	WORD BytesPerSample;
	WORD Channels;
	DWORD BufferSize;
};

internal void Win32ClearSoundBuffer(IDirectSoundBuffer *SoundBuffer)
{
	void *AudioPtr1, *AudioPtr2;
	DWORD AudioBytes1, AudioBytes2;
	if(SUCCEEDED(SoundBuffer->Lock(0, 0, &AudioPtr1, &AudioBytes1, &AudioPtr2, &AudioBytes2, DSBLOCK_ENTIREBUFFER))) {
		if(AudioPtr1) {
			memset(AudioPtr1, 0, AudioBytes1);
		}
		if(AudioPtr2) {
			memset(AudioPtr2, 0, AudioBytes2);
		}

		SoundBuffer->Unlock(AudioPtr1, AudioBytes1, AudioPtr2, AudioBytes2);
	}
}

internal void Win32InitAudio(HWND Window, win32_init_audio_desc *Desc)
{
	IDirectSound8 *DSound;
	if(SUCCEEDED(DirectSoundCreate8(0, &DSound, 0))) {
		if(SUCCEEDED(DSound->SetCooperativeLevel(Window, DSSCL_NORMAL))) {
			WAVEFORMATEX AudioFormat = {};
			AudioFormat.wFormatTag = WAVE_FORMAT_PCM;
			AudioFormat.nChannels = Desc->Channels;
			AudioFormat.nSamplesPerSec = Desc->SamplesPerSecond;
			AudioFormat.wBitsPerSample = Desc->BytesPerSample * 8;
			AudioFormat.nBlockAlign = (WORD)(AudioFormat.nChannels * AudioFormat.wBitsPerSample / 8);
			AudioFormat.nAvgBytesPerSec = AudioFormat.nSamplesPerSec * AudioFormat.nBlockAlign;
	
			IDirectSoundBuffer *SoundBuffer;
			DSBUFFERDESC BufferDesc = {};
			BufferDesc.dwSize = sizeof(BufferDesc);
			BufferDesc.dwFlags = DSBCAPS_CTRLPAN | DSBCAPS_CTRLVOLUME | DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_STICKYFOCUS;
			BufferDesc.dwBufferBytes = Desc->BufferSize;
			BufferDesc.lpwfxFormat = &AudioFormat;
			BufferDesc.guid3DAlgorithm = DS3DALG_DEFAULT;
			if(SUCCEEDED(DSound->CreateSoundBuffer(&BufferDesc, &SoundBuffer, 0))) {
				Win32ClearSoundBuffer(SoundBuffer);

				f32 Volume = 0.8f;
				f32 Pan = 0.f;
				SoundBuffer->SetVolume((LONG)(DSBVOLUME_MIN + Volume * (DSBVOLUME_MAX - DSBVOLUME_MIN)));
				GlobalAudioOutput.SoundBuffer = SoundBuffer;
				GlobalAudioOutput.BufferSize = Desc->BufferSize;
				GlobalAudioOutput.RunningSampleIndex = 0;
				GlobalAudioOutput.LatencySampleCount = (DWORD)((AUDIO_LATENCY_FRAMES * Desc->SamplesPerSecond * Desc->BytesPerSample * Desc->Channels) * TARGET_FRAME_DELTA);
				GlobalAudioOutput.BytesPerSample = Desc->BytesPerSample;
				GlobalAudioOutput.SamplesPerSecond = Desc->SamplesPerSecond;
				GlobalAudioOutput.Valid = 1;
				GlobalSoundBufferHandle = SoundBuffer;
			}
			else {
				OutputDebugStringA("IDirectSound8::CreateSoundBuffer failed\n");
			}
		}
		else {
			OutputDebugStringA("IDirectSound8::SetCooperativeLevel failed\n");
		}
	}
	else {
		OutputDebugStringA("DirectSoundCreate8 failed\n");
	}
}

internal void Win32FillSoundBuffer(game_audio_buffer *Input, DWORD ByteToLock, DWORD BytesToWrite)
{
	if(!GlobalAudioOutput.Valid)
		return;

	IDirectSoundBuffer *SoundBuffer = GlobalAudioOutput.SoundBuffer;
	void *AudioPtr[2];
	DWORD AudioBytes[2];
	HRESULT LockResult = SoundBuffer->Lock(ByteToLock, BytesToWrite, &AudioPtr[0], &AudioBytes[0], &AudioPtr[1], &AudioBytes[1], 0);
	if(LockResult == DSERR_BUFFERLOST) {
		if(SUCCEEDED(SoundBuffer->Restore())) {
			LockResult = SoundBuffer->Lock(ByteToLock, BytesToWrite, &AudioPtr[0], &AudioBytes[0], &AudioPtr[1], &AudioBytes[1], 0);
		}
	}

	switch(LockResult) {
		case DSERR_BUFFERLOST:
			OutputDebugStringA("DSERR: buffer lost\n");
			break;
		case DSERR_INVALIDCALL:
			OutputDebugStringA("DSERR: invalid call\n");
			break;
		case DSERR_INVALIDPARAM:
			OutputDebugStringA("DSERR: invalid param\n");
			break;
		case DSERR_PRIOLEVELNEEDED:
			OutputDebugStringA("DSERR: priority level needed\n");
		break;
	}

	if(SUCCEEDED(LockResult)) {
		s16 *SampleIn = Input->Samples;
		for(u32 RegionIndex = 0; RegionIndex < 2; RegionIndex++) {
			s16 *SampleOut = (s16*)AudioPtr[RegionIndex];
			for(u32 i = 0; i < AudioBytes[RegionIndex] / 4; i++) {
				*SampleOut++ = *SampleIn++;
				*SampleOut++ = *SampleIn++;
				GlobalAudioOutput.RunningSampleIndex += 2;
			}
		}

		if(FAILED(SoundBuffer->Unlock(AudioPtr[0], AudioBytes[0], AudioPtr[1], AudioBytes[1]))) {
			OutputDebugStringA("IDirectSoundBuffer::Unlock failed\n");
		}
	}
	else {
		OutputDebugStringA("IDirectSoundBuffer::Lock failed\n");
	}
}

HWND CreateFullscreenWindow(HWND hWnd)
{
	HMONITOR Monitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
	MONITORINFO MonitorInfo = { sizeof(MonitorInfo) };
	if(!GetMonitorInfo(Monitor, &MonitorInfo)) 
		return 0;

	return CreateWindowA(WndClassName, 
		WndName,
		WS_POPUP | WS_VISIBLE,
		MonitorInfo.rcMonitor.left + 1920,
		MonitorInfo.rcMonitor.top,
		MonitorInfo.rcMonitor.right - MonitorInfo.rcMonitor.left,
		MonitorInfo.rcMonitor.bottom - MonitorInfo.rcMonitor.top,
		hWnd, 0, GlobalInstance, 0);
}

internal void SetMainWindow(HWND Window)
{
	process_data *Proc = GlobalProcData;
	wglMakeCurrent(0, 0);
	ReleaseDC(Proc->Window, Proc->WindowDC);
	DestroyWindow(Proc->Window);

	Proc->Window = Window;
	Proc->WindowDC = GetDC(Window);

	PIXELFORMATDESCRIPTOR pfd = {};
	DescribePixelFormat(Proc->WindowDC, GlobalContextData.PixelFormat, sizeof(pfd), &pfd);
	SetPixelFormat(Proc->WindowDC, GlobalContextData.PixelFormat, &pfd);
	wglMakeCurrent(Proc->WindowDC, Proc->GLContext);
}

global_persist LPVOID GlobalGameFiber;

LRESULT CALLBACK
WindowProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    GAME_INPUT_BUTTON Button;
	switch (Msg) {
		case WM_CREATE:
			break;
		case WM_LBUTTONDOWN:
			ButtonPressed(BUTTON_MOUSE_LEFT);
			break;
		case WM_RBUTTONDOWN:
			ButtonPressed(BUTTON_MOUSE_RIGHT);
			break;
		case WM_KEYDOWN:
			// NOTE: Ignores repeat-messages
			if((lParam & (1 << 30)) == 0) {
				if(GetKeyState(VK_CONTROL) & 0x8000) {
					switch(wParam) {
						case 'S':
						{
							Win32DebugStoreGameState();
						} break;
						case 'L':
						{
							Win32DebugLoadGameState();
						} break;
					}
				}
				else if(GetKeyState(VK_F12) & 0x8000) {
					HWND NewWindow = CreateFullscreenWindow(0);
					SetMainWindow(NewWindow);
				}
				else {
					Button = GetButton(wParam);
					if(Button != UNKNOWN_BUTTON) {
						ButtonPressed(Button);
					}
				}
			}
			break;
		case WM_LBUTTONUP:
			ButtonUnpressed(BUTTON_MOUSE_LEFT);
			break;
		case WM_RBUTTONUP:
			ButtonUnpressed(BUTTON_MOUSE_RIGHT);
			break;
		case WM_KEYUP:
			Button = GetButton(wParam);
			if(Button != UNKNOWN_BUTTON) {
				ButtonUnpressed(Button);
			}
			break;
		case WM_CHAR:
#ifdef GAME_DEBUG
			DebugInputHandleCharacterMessage(wParam, lParam);
#endif
			break;
		case WM_MOVING:
		case WM_PAINT:
		case WM_SIZE:
			if(GlobalGameFiber) {
				SwitchToFiber(GlobalGameFiber);
			}
			return DefWindowProcA(hWnd, Msg, wParam, lParam);
			break;
		case WM_SYSCOMMAND:
			switch(wParam & 0xFFF0) {
				case SC_MOVE:
				{
					// TODO: This shit means some faggot is holding the left mouse
					// button down on the menu bar. Not handling this event
					// will fuck everything up as you can't move the window around any more,
					// but switching to the game fiber and calling DefWindowProc after will just
					// hang the fucking application. Fuck!
#if 0
					if(GlobalGameFiber) {
						SwitchToFiber(GlobalGameFiber);
					}
#endif
					return DefWindowProcA(hWnd, Msg, wParam, lParam);
				} break;
				default:
					return DefWindowProcA(hWnd, Msg, wParam, lParam);
			}
			break;
		case WM_CLOSE:
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProcA(hWnd, Msg, wParam, lParam);
	}

	return 0;
}

static inline BOOL FileExists(char *Filename)
{
	return GetFileAttributesA(Filename) != INVALID_FILE_ATTRIBUTES;
}
