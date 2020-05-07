extern GAME_UPDATE_AND_RENDER(GameUpdateAndRender);
internal GAME_UPDATE_AND_RENDER(GameUpdateAndRenderStub)
{
}

internal PLATFORM_SEND_COMMAND(Win32SendCommand)
{
	// TODO: Needs work
	char CmdBuffer[4096];
	if(GlobalPipe) {
		char *Cmd = 0;
		switch(Type) {
			case PlatformCmdType_HelloWorld:
				Cmd = "echo Hello World!\n";
				break;
			case PlatformCmdType_Build:
				Cmd = "..\\vc\\build\n";
				break;
			case PlatformCmdType_List:
				Cmd = "dir\n";
				break;
			case PlatformCmdType_ChangeDir:
				Cmd = CmdBuffer;
				sprintf_s(CmdBuffer, sizeof(CmdBuffer), "cd %s\n", (char*)UserData);
				break;
			case PlatformCmdType_Cat:
				Cmd = CmdBuffer;
				sprintf_s(CmdBuffer, sizeof(CmdBuffer), "type %s\n", (char*)UserData);
				break;
			InvalidDefaultCase;
		}

		SendCmdToPipe(GlobalPipe, Cmd);
	}
}

internal PLATFORM_OPEN_FILE(Win32OpenFile)
{
	file_info Result = {};

	char Buffer[1024];
	char *FilePath = Win32GetFilepath(Name, Type, Buffer, sizeof(Buffer));
	if(FilePath) {
		HANDLE File = CreateFileA(FilePath, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
		if(File != INVALID_HANDLE_VALUE) {
			LARGE_INTEGER FileSize;
			if(GetFileSizeEx(File, &FileSize)) {
				Assert(FileSize.HighPart == 0);
				Result.Valid = true;
				Result.Size = FileSize.LowPart;
				Result.Type = Type;
				Result._Handle = File;
			}
			else {
				OutputDebugStringA("GetFileSizeEx failed\n");
				CloseHandle(File);
			}
		}
		else {
			OutputDebugStringA("CreateFile failed\n");
		}
	}
	else {
		OutputDebugStringA("Could not get filepath\n");
	}

	return Result;
}

internal PLATFORM_CLOSE_FILE(Win32CloseFile)
{
	if(Info->Valid && Info->_Handle != INVALID_HANDLE_VALUE) {
		CloseHandle(Info->_Handle);
		Info->Size = 0;
		Info->Type = FileType_Invalid;
		Info->Valid = false;
		Info->_Handle = INVALID_HANDLE_VALUE;
	}
}

internal PLATFORM_READ_ENTIRE_FILE(Win32ReadEntireFile)
{
	b32 Result = false;
	if(Info->Valid) {
		HANDLE File = (HANDLE)Info->_Handle;
		if(File != INVALID_HANDLE_VALUE) {
			DWORD BytesRead;
			if(ReadFile(File, Buffer, Info->Size, &BytesRead, 0)) {
				if(BytesRead == Info->Size) {
					Result = true;
				}
				else {
					OutputDebugStringA("ReadFile failed: not all bytes were read\n");
				}

				if(SetFilePointer(File, 0, 0, FILE_BEGIN) == INVALID_SET_FILE_POINTER) {
					OutputDebugStringA("SetFilePointer failed\n");
					Win32CloseFile(Info);
				}
			}
			else {
				OutputDebugStringA("ReadFile failed\n");
			}
		}
		else {
			OutputDebugStringA("Win32ReadEntireFile failed: INVALID_HANDLE_VALUE\n");
		}
	}

	return Result;
}

internal PLATFORM_WRITE_FILE(Win32WriteFile)
{
	b32 Result = false;
	char StrBuffer[1024];
	char *FilePath = Win32GetFilepath(Name, Type, StrBuffer, sizeof(StrBuffer));
	if(FilePath) {
		HANDLE File = CreateFileA(FilePath, GENERIC_WRITE, 0, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
		if(File != INVALID_HANDLE_VALUE) {
			DWORD BytesWritten;
			if(WriteFile(File, Buffer, Size, &BytesWritten, 0)) {
				if(BytesWritten == Size) {
					Result = true;
				}
				else {
					OutputDebugStringA("Could not write all bytes to file\n");
				}
			}
			else {
				OutputDebugStringA("WriteFile failed\n");
			}
			CloseHandle(File);
		}
		else {
			OutputDebugStringA("CreateFile failed\n");
		}
	}
	else {
		OutputDebugStringA("Could not get filepath\n");
	}

	return Result;
}

internal PLATFORM_ALLOCATE_MEMORY(Win32AllocateMemory)
{
	platform_memory_block * Result = 0;
	umm TotalSize = Size + sizeof(win32_memory_block);
	win32_memory_block * MemBlock = (win32_memory_block *)VirtualAlloc(0, TotalSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if(MemBlock) {
		Result = (platform_memory_block*)MemBlock;		
		Result->Magic = PLATFORM_MEMORY_BLOCK_MAGIC;
		Result->Base = (u8*)(MemBlock + 1);
		Result->Size = Size;
		Result->Used = 0;
		
		MemBlock->Guardian = WIN32_MEMORY_BLOCK_GUARDIAN;
		Win32AppendMemoryBlock(MemBlock);
		if(GlobalRecordHandle != INVALID_HANDLE_VALUE) {
			MemBlock->Flags |= MemoryBlock_DeleteOnStateLoad;
		}
		
		Win32AllocBlockCount++;
	}

	return Result;
}

internal PLATFORM_DEALLOCATE_MEMORY(Win32DeallocateMemory)
{
	win32_memory_block *MemBlock = (win32_memory_block*)Block;
	Assert(MemBlock->Guardian == WIN32_MEMORY_BLOCK_GUARDIAN);
	Win32RemoveMemoryBlock(MemBlock);
	
	VirtualFree(MemBlock, 0, MEM_RELEASE);
	
	if(Win32AllocBlockCount) {
		Win32AllocBlockCount--;
	}
}