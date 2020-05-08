global_persist debug_state * GlobalDebugState;

#ifdef GAME_DEBUG

#define DEBUG_RENDER_GROUP_MEMORY MEGABYTES(32)

internal debug_state * DEBUGInit()
{
	debug_state *DebugState = BootstrapStruct(debug_state, Memory);
	
	for(u32 i = 0; i < ARRAY_COUNT(DebugState->EventBuffers); i++) {
		debug_event_buffer *Buffer = DebugState->EventBuffers + i;
		Buffer->PageSentinel.Next = &Buffer->PageSentinel;
		Buffer->PageSentinel.Prev = &Buffer->PageSentinel;
	}

	return DebugState;
}

internal void DebugTimeGraph(render_group *Group, debug_time_graph *Graph, v2 At, v2 Dim, f32 MinValue, f32 MaxValue)
{
	rect2 GraphRect = MakeRectFromCorners(At, At + Dim);
	PushRectangle(Group, GraphRect, V4(0.2f, 0.2f, 0.2f, 0.5f));
	PushRectangleOutline(Group, GraphRect, 1.f, V4(0.f, 0.f, 0.f, 1.f));

	u32 SampleCount = TIME_GRAPH_SAMPLE_COUNT;
	f32 SpacingX = Dim.x / (f32)(SampleCount - 1);

	// TODO: Line entry for render_group
	v2 *Points = (v2*)PushAlloc(SampleCount * sizeof(v2));
	for(u32 i = 0; i < SampleCount; i++) {
		u32 Index = (Graph->OldestSample + i) % SampleCount;
		f32 Value = (Graph->Samples[Index] - MinValue) / (MaxValue - MinValue);
		Points[i] = At + V2(i * SpacingX, Dim.y * Value);
	}
	PushLines(Group, Points, SampleCount, 1.f, RenderEntryLinesMode_Simple);
	PopAlloc();
}

internal inline debug_stored_event * DebugAllocStoredEvent(debug_state *DebugState)
{
	debug_stored_event *Result = DebugState->FirstFreeEvent;
	if(!Result) {
		Result = push_type(&DebugState->Memory, debug_stored_event);
	}
	else {
		DebugState->FirstFreeEvent = Result->NextFree;
	}

	return Result;
}

internal debug_stored_event * DebugStoreEvent(debug_state *DebugState, debug_frame *Frame, debug_event *Event)
{
	debug_stored_event *Result = DebugAllocStoredEvent(DebugState);
	if(Result) {
		Result->Event = *Event;
		Result->Next = &Frame->EventSentinel;
		Result->Prev = Frame->EventSentinel.Prev;

		Result->Next->Prev = Result;
		Result->Prev->Next = Result;
	}

	return Result;
}

internal void AddSample(debug_time_graph *TimeGraph, f32 Sample)
{
	u32 CurSample = (TimeGraph->OldestSample) % TIME_GRAPH_SAMPLE_COUNT;
	TimeGraph->NewestSample = CurSample;
	TimeGraph->OldestSample = (CurSample + 1) % TIME_GRAPH_SAMPLE_COUNT;
	TimeGraph->Samples[CurSample] = Sample;
}

internal debug_profile_record * DebugAllocRecord(debug_state *DebugState)
{
	debug_profile_record *Result = 0;
	if(DebugState->FirstFreeRecord) {
		Result = DebugState->FirstFreeRecord;
		DebugState->FirstFreeRecord = Result->NextFree;
	}
	else {
		Result = push_type(&DebugState->Memory, debug_profile_record);
	}

	if(Result) {
		ZeroStruct(Result);
	}

	return Result;
}

internal inline void DebugFreeRecords(debug_state *DebugState, debug_profile_record_hash *RecordHash)
{
	debug_profile_record *Record = RecordHash->Record;
	while(Record) {
		debug_profile_record *Next = Record->NextInHash;
		Record->NextFree = DebugState->FirstFreeRecord;
		DebugState->FirstFreeRecord = Record;
		Record = Next;
	}
	RecordHash->Record = 0;
}

internal void DebugFreeFrame(debug_state *DebugState, debug_frame *Frame)
{
	debug_stored_event *FirstEvent = Frame->EventSentinel.Next;
	debug_stored_event *LastEvent = Frame->EventSentinel.Prev;
	if(FirstEvent) {
		Assert(LastEvent);

		// NOTE: Insert whole chain into freelist
		LastEvent->Next = DebugState->FirstFreeEvent;
		DebugState->FirstFreeEvent = FirstEvent;
	}
	Frame->EventSentinel.Next = Frame->EventSentinel.Prev = &Frame->EventSentinel;
}

internal void DebugFreeAllFrames(debug_state *DebugState)
{
	for(u32 i = 0; i < MAX_DEBUG_FRAMES; i++) {
		debug_frame *Frame = DebugState->Frames + i;
		DebugFreeFrame(DebugState, Frame);
	}
}

#define MAX_PROFILE_NODE_DEPTH 256
internal void CollateDebugEvents(debug_state *DebugState, mem_arena *TranArena)
{
	TIMED_FUNCTION();

	if(!DebugState->RecordingPaused) {
		TIMED_BLOCK("StoreDebugEvents");

		u32 RecordingIdx = DebugState->RecordingFrameIdx;
 		RecordingIdx = (RecordingIdx + 1) % MAX_DEBUG_FRAMES;
		DebugState->RecordingFrameIdx = RecordingIdx;
		DebugState->ViewFrameIdx = RecordingIdx;
		debug_frame *Frame = DebugState->Frames + RecordingIdx;
		DebugFreeFrame(DebugState, Frame);

		debug_event_buffer *EventReadBuffer = DebugState->EventBuffers + DebugState->EventReadBuffer;
		Frame->CyclesStart = EventReadBuffer->FrameStartCycles;
		Frame->CycleCount = EventReadBuffer->FrameCycleCount;
		for(debug_event_page *Page = EventReadBuffer->PageSentinel.Next;
			Page != &EventReadBuffer->PageSentinel;
			Page = Page->Next)
		{
			for(u32 i = 0; i < Page->NumEvents; i++) {
				debug_event *Event = Page->Events + i;
				DebugStoreEvent(DebugState, Frame, Event);
			}
		}
	}

	{
		TIMED_BLOCK("BuildDebugNodes");

		debug_frame *Frame = DebugState->Frames + DebugState->ViewFrameIdx;
		debug_profile_node *RootNode = &DebugState->ProfileRootNode;
		ZeroStruct(RootNode);
		RootNode->StartCycles = Frame->CyclesStart;
		RootNode->CycleCount = Frame->CycleCount;
	
		u32 Depth = 1;
		debug_profile_node *NodeStack[MAX_PROFILE_NODE_DEPTH] = {};
		NodeStack[0] = RootNode;
		for(debug_stored_event *StoredEvent = Frame->EventSentinel.Next;
			StoredEvent != &Frame->EventSentinel && StoredEvent;
			StoredEvent = StoredEvent->Next)
		{
			debug_event *Event = &StoredEvent->Event;
			switch(Event->Type) {
				case EVENT_TYPE_OPEN_BLOCK:
				{
					Assert(Depth < ArrayCount(NodeStack));
					debug_profile_node *Node = push_type(TranArena, debug_profile_node);
					if(Node) {
						ZeroStruct(Node);
						Node->Parent = NodeStack[Depth - 1];
						Node->BlockName = Event->BlockName;
						Node->StartCycles = Event->TimeStamp;
						Node->Next = Node->Parent->FirstChild;
						Node->Parent->FirstChild = Node;
					}
					NodeStack[Depth++] = Node;
				} break;
				case EVENT_TYPE_CLOSE_BLOCK:
				{
					Assert(Depth > 1);
					debug_profile_node *Node = NodeStack[--Depth];
					Node->CycleCount = Event->TimeStamp - Node->StartCycles;
				} break;

				InvalidDefaultCase;
			}
		}
	}
}

internal inline u8 DebugConsoleTextLineAddChar(debug_console_text_line *Line, u8 Char)
{
	u8 Result = 0;
	if(Line->Size < MAX_DEBUG_CONSOLE_LINE_CHAR_COUNT) {
		Line->Buffer[Line->Size++] = Char;
		Result = Char;
	}

	return Result;
}

internal inline void DebugConsoleTextLineDelChar(debug_console_text_line *Line)
{
	if(Line->Size > 0) {
		Line->Size--;
	}
}

internal inline void CmdLineAddChar(command_line *CmdLine, u8 Char)
{
	CmdLine->AutoComplete = false;
	DebugConsoleTextLineAddChar(&CmdLine->Line, Char);
}

internal inline void CmdLineDelChar(command_line *CmdLine)
{
	CmdLine->AutoComplete = false;
	DebugConsoleTextLineDelChar(&CmdLine->Line);
}

internal inline debug_console_text_line * DebugConsolePushLine(debug_console *Console)
{
	u32 NewLineIdx = (Console->MostRecentLineIdx + 1) % MAX_DEBUG_CONSOLE_LINES;
	Console->PushedLineCount++;
	Console->MostRecentLineIdx = NewLineIdx;

	debug_console_text_line *Target = Console->Lines + NewLineIdx;
	Target->Size = 0;

	return Target;
}

internal void DebugConsolePushString(const char *Str, debug_msg_type Type)
{
	if(GlobalDebugState) {
		debug_console *Console = &GlobalDebugState->Console;
		debug_console_text_line *Line = DebugConsolePushLine(Console);
		
		if(Type == MSG_ERROR) {
			DebugConsoleTextLineAddChar(Line, '!');
			DebugConsoleTextLineAddChar(Line, ' ');
			Console->Active = true;
		}
		else if(Type == MSG_WARNING) {
			DebugConsoleTextLineAddChar(Line, 'W');
			DebugConsoleTextLineAddChar(Line, ' ');
		}

		while(*Str) {
			char Token = *Str++;
			if(Token == '\n') {
				Line = DebugConsolePushLine(Console);
			}
			else {
				// TODO: Automatic line breaking based on actual console screen size?
				if(!DebugConsoleTextLineAddChar(Line, Token)) {
					Line = DebugConsolePushLine(Console);
					Str--;
				}
			}
		}
	}
}

enum debug_cmd_type
{
	CMD_TYPE_TOGGLE_DEBUG_OVERLAY,
	CMD_TYPE_TOGGLE_PROFILER,
	CMD_TYPE_TOGGLE_SOUND_DISPLAY,
	CMD_TYPE_TOGGLE_ISO_GRID,
	CMD_TYPE_TOGGLE_MEMORY,
    CMD_TYPE_TOGGLE_SPRITEMAPS,
	CMD_TYPE_LIST_COMMANDS,
	CMD_TYPE_CLEAR,
	CMD_TYPE_CLOSE,
	CMD_TYPE_TEST,
	CMD_TYPE_SYSTEM,
	CMD_TYPE_RELOAD_LEVEL,

	CMD_TYPE_COUNT
};

struct debug_command
{
	debug_cmd_type Type;
	char *Str;
};

global_persist debug_command DebugCommands[] = {
	{ CMD_TYPE_TOGGLE_DEBUG_OVERLAY, "overlay" },
	{ CMD_TYPE_TOGGLE_PROFILER, "profiler" },
	{ CMD_TYPE_TOGGLE_SOUND_DISPLAY, "sound" },
	{ CMD_TYPE_TOGGLE_ISO_GRID, "isogrid" },
	{ CMD_TYPE_TOGGLE_MEMORY, "memory" },
    { CMD_TYPE_TOGGLE_SPRITEMAPS, "sprites" },
	{ CMD_TYPE_CLEAR, "clear" },
	{ CMD_TYPE_LIST_COMMANDS, "list" },
	{ CMD_TYPE_LIST_COMMANDS, "help" },
	{ CMD_TYPE_CLOSE, "close" },
	{ CMD_TYPE_SYSTEM, "sys" },
	{ CMD_TYPE_RELOAD_LEVEL, "reload" },
};

internal void CmdLineExecute(debug_console *Console)
{
	debug_state *State = GlobalDebugState;
	command_line *CmdLine = &Console->CmdLine;
	u32 CmdLineStart = CmdLine->BeginOffset;
	CmdLine->AutoComplete = false;

	debug_command *Cmd = 0;
	split_str_result SplitResult = SplitOnFirstToken((char*)CmdLine->Line.Buffer + CmdLineStart, CmdLine->Line.Size - CmdLineStart, ' ');
	char *Arg = SplitResult.Rhs;
	u32 ArgLen = SplitResult.RhsLen;
	for(u32 i = 0; i < ARRAY_COUNT(DebugCommands); i++) {
		debug_command *TestCmd = DebugCommands + i;
		if(StringsAreEqual(SplitResult.Lhs, SplitResult.LhsLen, TestCmd->Str)) {
			Cmd = TestCmd;
			break;
		}
	}

	Assert(ArgLen < CmdLine->Line.Size);

	debug_console_text_line * Target = DebugConsolePushLine(Console);
	CopyMemory(Target->Buffer, CmdLine->Line.Buffer, CmdLine->Line.Size);
	Target->Size = CmdLine->Line.Size;
	
	CmdLine->LastCmdSize = CmdLine->Line.Size;
	CmdLine->Line.Size = 0;

#define TOGGLE(x) DebugConsolePushString((x = !x) ? "enabled" : "disabled")
	if(Cmd) {
		switch(Cmd->Type) {
			case CMD_TYPE_TOGGLE_DEBUG_OVERLAY: 
				TOGGLE(State->DrawOverlay);
				break;
			case CMD_TYPE_TOGGLE_PROFILER: 
				TOGGLE(State->DrawProfiler); break;
			case CMD_TYPE_TOGGLE_SOUND_DISPLAY: 
				TOGGLE(State->DrawSoundDisplay); break;
			case CMD_TYPE_TOGGLE_ISO_GRID:
				TOGGLE(State->DrawIsoGrid); break;
			case CMD_TYPE_TOGGLE_MEMORY:
				TOGGLE(State->ShowMemory); break;
            case CMD_TYPE_TOGGLE_SPRITEMAPS:
                TOGGLE(State->DrawSpritemaps); break;
			case CMD_TYPE_CLEAR:
				for(u32 i = 0; i < Console->NumberOfDisplayedLines; i++) {
					DebugConsolePushString("");
				}
				break;
			case CMD_TYPE_CLOSE:
				Console->Active = false;
				break;
			case CMD_TYPE_SYSTEM:
			{
				// TODO: Needs a lot of work obviously
				split_str_result SysResult = SplitOnFirstToken(Arg, ArgLen, ' ');
				if(StringsAreEqual(SysResult.Lhs, SysResult.LhsLen, "build")) {
					Platform->SendCommand(PlatformCmdType_Build, 0);
				}
				else if(StringsAreEqual(SysResult.Lhs, SysResult.LhsLen, "hello")) {
					Platform->SendCommand(PlatformCmdType_HelloWorld, 0);
				}
				else if(StringsAreEqual(SysResult.Lhs, SysResult.LhsLen, "ls")) {
					Platform->SendCommand(PlatformCmdType_List, 0);
				}
				else if(StringsAreEqual(SysResult.Lhs, SysResult.LhsLen, "cd")) {
					if(SysResult.Rhs && SysResult.RhsLen) {
						if(SysResult.Rhs + SysResult.RhsLen < (char*)CmdLine->Line.Buffer + MAX_DEBUG_CONSOLE_LINE_CHAR_COUNT) {
							SysResult.Rhs[SysResult.RhsLen] = 0;
							Platform->SendCommand(PlatformCmdType_ChangeDir, SysResult.Rhs);
						}
						else {
							DebugConsolePushString("Max arg length exceeded!", MSG_ERROR);
						}
					}
					else {
						DebugConsolePushString("Invalid argument!", MSG_ERROR);
					}
				}
				else if(StringsAreEqual(SysResult.Lhs, SysResult.LhsLen, "cat")) {
					if(SysResult.Rhs && SysResult.RhsLen) {
						if(SysResult.Rhs + SysResult.RhsLen < (char*)CmdLine->Line.Buffer + MAX_DEBUG_CONSOLE_LINE_CHAR_COUNT) {
							SysResult.Rhs[SysResult.RhsLen] = 0;
							Platform->SendCommand(PlatformCmdType_Cat, SysResult.Rhs);
						}
						else {
							DebugConsolePushString("Max arg length exceeded!", MSG_ERROR);
						}
					}
					else {
						DebugConsolePushString("Invalid argument!", MSG_ERROR);
					}
				}
				else
					DebugConsolePushString("Invalid command!", MSG_ERROR);
			} break;
			case CMD_TYPE_LIST_COMMANDS:
			{
				debug_console_text_line *Line = DebugConsolePushLine(&GlobalDebugState->Console);
				for(u32 i = 0; i < ARRAY_COUNT(DebugCommands); i++) {
					u8 *Str = (u8*)DebugCommands[i].Str;
					while(*Str) {
						if(!DebugConsoleTextLineAddChar(Line, *Str)) {
							Line = DebugConsolePushLine(&GlobalDebugState->Console);
							DebugConsoleTextLineAddChar(Line, *Str);
						}
						Str++;
					}
					if(i < ARRAY_COUNT(DebugCommands)) {
						DebugConsoleTextLineAddChar(Line, ' ');
					}
				}
			} break;
			case CMD_TYPE_RELOAD_LEVEL:
			{
				State->ReloadLevel = true;
				break;
			}
			default: 
				DebugConsolePushString("Command not implemented");
		}
	}
	else {
		DebugConsolePushString("Unrecognized command");
	}
#undef TOGGLE_VALUE
}

internal void CmdLineAutoComplete(command_line *CmdLine)
{
	debug_console_text_line *Line = &CmdLine->Line;
	if(!CmdLine->AutoComplete) {
		char *Pattern = 0;
		u32 PatternSize = 0;
		u8 *Begin = Line->Buffer + CmdLine->BeginOffset;
		u8 *End = Line->Buffer + Line->Size;
		if(Begin == End) {
			// NOTE: Empty command line, ignore
		}
		else {
			u8 *At = End;
			// NOTE: Find last word
			do {
				if(*(--At) == ' ')
					break;
			} while(At >= Begin);
			Pattern = (char*)At + 1;
			PatternSize = (u32)((char*)End - Pattern);
		}

		CmdLine->Pattern = Pattern;
		CmdLine->PatternSize = PatternSize;

		if(Pattern) {
			CmdLine->AutoComplete = true;
		}
	}

	if(CmdLine->Pattern) {
		// NOTE: Find a match
		u8 *Found = 0;
		u32 Index = CmdLine->PatternMatch;
		for(u32 i = 0; i < ARRAY_COUNT(DebugCommands); i++) {
			Index = (Index + 1) % ARRAY_COUNT(DebugCommands);
			debug_command *Cmd = DebugCommands + Index;
			if(StringStartsWith(CmdLine->Pattern, CmdLine->PatternSize, Cmd->Str)) {
				CmdLine->PatternMatch = Index;
				Found = (u8*)Cmd->Str;
				break;
			}
		}

		// NOTE: Copy command into command buffer
		if(Found) {
			char *Dst = CmdLine->Pattern;
			u32 LineSize = (u32)(Dst - (char*)Line->Buffer);
			while(*Found) {
				if(LineSize < MAX_DEBUG_CONSOLE_LINE_CHAR_COUNT) {
					*Dst++ = *Found++;
					LineSize++;
				}
				else {
					break;
				}
			}
			Line->Size = LineSize;
		}
	}
}

struct debug_memory_arena_info
{
	u32 BlockCount;
	u64 MinBlockSize;
	u64 LargestBlockSize;
	u64 TotalUsed;
	u64 TotalAvaiable;
	u64 TotalWasted;
};

internal debug_memory_arena_info
GetMemoryArenaInfo(mem_arena *Arena)
{
	u64 LargestBlockSize = 0;
	u64 TotalUsed = 0;
	u64 TotalAvailable = 0;
	u64 TotalWasted = 0;

	if(Arena->Block) {
		u64 CurBlockFreeSpace = Arena->Block->Size - Arena->Block->Used;
		platform_memory_block *Block = Arena->Block;
		while(Block) {
			TotalUsed += Block->Used;
			TotalAvailable += Block->Size;
			LargestBlockSize = MAXIMUM(LargestBlockSize, Block->Size);
			Block = Block->Prev;
		}
		TotalWasted = TotalAvailable - TotalUsed - CurBlockFreeSpace;
	}

	debug_memory_arena_info Info = {};
	Info.BlockCount = Arena->NumBlocks;
	Info.MinBlockSize = Arena->MinBlockSize;
	Info.LargestBlockSize = LargestBlockSize;
	Info.TotalAvaiable = TotalAvailable;
	Info.TotalUsed = TotalUsed;
	Info.TotalWasted = TotalWasted;

	return Info;
}

internal void ProcessDebugInput(debug_state *DebugState, game_inputs *Inputs, u8 *ConsoleInputBuffer, umm ConsoleInputBufferSize)
{
	TIMED_FUNCTION();

	// TODO: Only process inputs if Console. is actually active
	debug_console *Console = &DebugState->Console;
	
	if(Inputs->State[BUTTON_DEBUG_CONSOLE] & Inputs->HalfTransitionCount[BUTTON_DEBUG_CONSOLE]) {
		Console->Active = !Console->Active;
	}

	if(Inputs->State[BUTTON_DEBUG_OVERLAY] & Inputs->HalfTransitionCount[BUTTON_DEBUG_OVERLAY]) {
		DebugState->DrawOverlay = !DebugState->DrawOverlay;
	}

    if (Inputs->State[BUTTON_DEBUG_TOGGLE_SOUND] & Inputs->HalfTransitionCount[BUTTON_DEBUG_TOGGLE_SOUND]) {
        DebugState->MuteAudio = !DebugState->MuteAudio;
    }

    if (Inputs->State[BUTTON_DEBUG_TOGGLE_SPRITES] & Inputs->HalfTransitionCount[BUTTON_DEBUG_TOGGLE_SPRITES]) {
        DebugState->DrawSpritemaps = !DebugState->DrawSpritemaps;
    }

	if(Console->Active) {
		command_line *CmdLine = &Console->CmdLine;
		if(Inputs->State[BUTTON_UP] & Inputs->HalfTransitionCount[BUTTON_UP]) {
			CmdLine->Line.Size = CmdLine->LastCmdSize;
		}
		else if(Inputs->State[BUTTON_DOWN] & Inputs->HalfTransitionCount[BUTTON_DOWN]) {
			CmdLine->Line.Size = 0;
		}
		if(ConsoleInputBuffer && ConsoleInputBufferSize) {
			DebugConsolePushString((char*)ConsoleInputBuffer);
		}
		CmdLine->BeginOffset = 2;
		CmdLine->Line.Buffer[0] = DEBUG_CONSOLE_CMDLINE_START_SYMBOL;
		CmdLine->Line.Buffer[1] = ' ';
		CmdLine->Line.Size = MAXIMUM(CmdLine->BeginOffset, CmdLine->Line.Size);
		for(u32 i = 0; i < Inputs->DebugInputBufferSize; i++) {
			u8 Char = Inputs->DebugInputBuffer[i];
			switch(Char) 
			{
				case '\b':
					CmdLineDelChar(CmdLine);
					break;
				case '\t':
					CmdLineAutoComplete(CmdLine);
					break;
				case '\r':
				case '\n':
					CmdLineExecute(Console);
					break;
				default:
					CmdLineAddChar(CmdLine, Char);
			}
		}
	}
}

/*

struct debug_draw_glyph_entry {
	f32 OffsetX;
	u8 Codepoint;
};

struct debug_draw_line_spec
{
	font *Font;
	f32 Width;
	u32 EntryCount;
	debug_draw_glyph_entry Entries[MAX_DEBUG_CONSOLE_LINE_CHAR_COUNT];
};

internal void DebugPushGlyphEntry(debug_draw_line_spec *Spec, f32 OffsetX, u8 Codepoint)
{
	Assert(Spec->EntryCount < MAX_DEBUG_CONSOLE_LINE_CHAR_COUNT);
	debug_draw_glyph_entry *Entry = Spec->Entries + Spec->EntryCount++;
	Entry->OffsetX = OffsetX;
	Entry->Codepoint = Codepoint;
}

internal void ProcessTextLine(debug_console_text_line *Line, font *Font, debug_draw_line_spec *Spec)
{
	u8 PrevCodepoint = 0;
	f32 Width = 0.f;
	f32 OffsetX = 0.f;
	u32 EntryCount = 0;
	glyph *Glyphs = Font->Data->Glyphs;

	Spec->EntryCount = 0;
	for(u32 i = 0; i < Line->Size; i++) {
		u8 Codepoint = Line->Buffer[i];
		if(Codepoint >= ASCII_GLYPH_COUNT) {
			Codepoint = '_';
		}

		OffsetX += FontGetKerning(Font->Data, Codepoint, PrevCodepoint);
		DebugPushGlyphEntry(Spec, OffsetX, Codepoint);
		OffsetX += Glyphs[Codepoint].AdvanceWidth;
		PrevCodepoint = Codepoint;
		Width += OffsetX;
	}
	
	Spec->Font = Font;
	Spec->Width = Width;
}

internal void ConsoleTextLine(render_group *Group, font *Font, debug_console_text_line *Line, v2 Origin)
{
	debug_draw_line_spec TextSpec;
	ProcessTextLine(Line, Font, &TextSpec);

	glyph *Glyphs = Font->Data->Glyphs;
	for(u32 i = 0; i < TextSpec.EntryCount; i++) {
		draw_glyph_entry *Entry = TextSpec.Entries + i;
		v2 At = V2(Origin.x + Entry->OffsetX, Origin.y);
		PushGlyph(Group, Font, At, Entry->Codepoint);
	}
}
*/

internal void ConsoleTextLine(render_group *Group, font *Font, debug_console_text_line *Line, v2 Origin)
{
	PushText(Group, Font, (char*)Line->Buffer, Line->Size, Origin, V4(1.f, 1.f, 1.f, 1.f));
}

internal void DebugConsole(render_group *Group, debug_console *Console, font *Font, window_params *WindowParams, u32 LineCount = 16)
{
	TIMED_FUNCTION();

	PushBlend(Group, true);
	// TODO: Find way to activate/deactivate console (F2)
	f32 Ascent = Font->Data->Ascent;
	f32 Descent = Font->Data->Descent;
	f32 LineAdvance = FontGetLineAdvance(Font->Data);

	v2 Margin = V2(5.f, 5.f);
	v2 ConsoleDim = V2(WindowParams->Width - 2.f * Margin.x, (1 + LineCount) * LineAdvance + 2 * Margin.y);
	v2 Origin = V2(Margin.x, (f32)WindowParams->Height - (ConsoleDim.y + Margin.y));

	Console->NumberOfDisplayedLines = LineCount;
	v2 At = V2(Origin.x, Origin.y - Descent);

	rect2 Backdrop = MakeRectFromCorners(Origin - Margin, Origin + ConsoleDim + Margin);
	PushRectangle(Group, Backdrop, V4(0.f, 0.f, 0.2f, 0.8f));
	PushRectangleOutline(Group, Backdrop, 2.f, V4(1.f, 1.f, 0.f, 1.f));
	PushRectangleOutline(Group, MakeRectFromCorners(Origin, Origin + V2(ConsoleDim.x, LineAdvance)), 1.f, V4(1.f, 0.f, 0.f, 1.f));
	
	debug_console_text_line *CmdLine = &Console->CmdLine.Line;
	ConsoleTextLine(Group, Font, CmdLine, At);
	At.y += LineAdvance;

	for(u32 i = 0; i < MINIMUM(Console->PushedLineCount, Console->NumberOfDisplayedLines); i++) {
		debug_console_text_line *Line = Console->Lines + (MAX_DEBUG_CONSOLE_LINES + Console->MostRecentLineIdx - i) % MAX_DEBUG_CONSOLE_LINES;
		ConsoleTextLine(Group, Font, Line, At);
		At.y += LineAdvance;
	}
}

internal void PrintMemoryArenaInfo(char *Name, char *Buffer, u32 BufferSize, mem_arena *Arena)
{
	debug_memory_arena_info MemInfo = GetMemoryArenaInfo(Arena);
	text_fmt_sprintf(Buffer, BufferSize,
		"%s:\n"
		"  %lldkb used, %lldkb total, %lldkb wasted (%.2f%%)\n"
		"  %d block(s), %lldkb min block size, %lldkb largest block\n",
		Name,
		MemInfo.TotalUsed >> 10,
		MemInfo.TotalAvaiable >> 10,
		MemInfo.TotalWasted >> 10,
		100.f * ((f32)MemInfo.TotalWasted / (f32)MemInfo.TotalAvaiable),
		MemInfo.BlockCount,
		MemInfo.MinBlockSize >> 10,
		MemInfo.LargestBlockSize >> 10);
}

internal v2 DebugTimeline(debug_state *DebugState, v2 At, render_group *Group, v2 CursorP, game_inputs *Inputs)
{	
	v2 FrameDim = V2(6.f, 32.f);
	v2 TimelineDim = V2(FrameDim.x * MAX_DEBUG_FRAMES, FrameDim.y);
	At.y -= TimelineDim.y;
	v2 TimelineOrigin = At;
		
	rect2 LatestFrameRect;
	v4 LatestFrameColor = V4(1.f, 0.f, 1.f, 1.f);
	v4 LatestFrameBorderColor = V4(1.f, 1.f, 1.f, 1.f);
	//glColor4f(0.f, 0.f, 0.f, 0.5f);
	//DrawRectangle(MakeRectFromCorners(TimelineOrigin, TimelineOrigin + TimelineDim));
	for(u32 i = 0; i < MAX_DEBUG_FRAMES; i++) {
		// TODO: Color code bad frames?
		v2 FrameOrigin = TimelineOrigin + V2(FrameDim.x * i, 0.f);
		rect2 R = MakeRectFromCorners(FrameOrigin, FrameOrigin + FrameDim);
		if(i == DebugState->RecordingFrameIdx) {
			LatestFrameRect = R;
			continue;
		}

		f32 t = (f32)(((i + MAX_DEBUG_FRAMES)- DebugState->RecordingFrameIdx) % MAX_DEBUG_FRAMES) / (f32)MAX_DEBUG_FRAMES;
		f32 Green = Lerp(t, 0.f, 1.f);
		Green *= Green;
		Green *= Green;
		f32 Red = 1.f - Green;

		PushRectangle(Group, R, V4(Red, Green, 0.f, 1.f));
		PushRectangleOutline(Group, R, 1.5f, V4(0.f, 0.f, 0.f, 1.f));

		if(IsInside(R, CursorP)) {
			PushRectangle(Group, R, V4(0.f, 0.f, 1.f, 1.f));
			PushRectangleOutline(Group, R, 2.f, V4(1.f, 1.f, 1.f, 1.f));
			if(Inputs->State[BUTTON_MOUSE_LEFT]) {
				DebugState->ViewFrameIdx = i;
				DebugState->RecordingPaused = true;
			}
			else if(Inputs->State[BUTTON_MOUSE_RIGHT] && Inputs->HalfTransitionCount[BUTTON_MOUSE_RIGHT]) {
				DebugState->RecordingPaused = false;
				DebugState->ViewFrameIdx = DebugState->RecordingFrameIdx;
			}
		}
	}
	PushRectangle(Group, LatestFrameRect, LatestFrameColor);
	PushRectangleOutline(Group, LatestFrameRect, 1.5f, LatestFrameBorderColor);

	if(DebugState->RecordingPaused) {
		v2 FrameOrigin = TimelineOrigin + V2(FrameDim.x * DebugState->ViewFrameIdx, 0.f);
		rect2 R = MakeRectFromCorners(FrameOrigin, FrameOrigin + FrameDim);
		PushRectangle(Group, R, V4(0.f, 0.f, 1.f, 1.f));
		PushRectangleOutline(Group, R, 1.5f, V4(1.f, 1.f, 0.f, 1.f));
	}

	return At;
}

internal inline debug_profile_node *DebugGetFirstChildNodeByName(debug_profile_node *Node, char *Name, u32 NameLen)
{
	debug_profile_node *Result = 0;
	for(debug_profile_node *Child = Node->FirstChild; Child != 0; Child = Child->Next) {
		if(StringsAreEqual(Name, NameLen, Child->BlockName)) {
			Result = Child;
			break;
		}
	}

	return Result;
}

internal debug_profile_node * DebugGetProfileNodeFromPath(debug_profile_node *RootNode, char *Path)
{
	debug_profile_node *Result = 0;
	debug_profile_node *Node = RootNode;
	if(*Path) {
		u32 PathLen = GetStringLength(Path);
		split_str_result SplitStr = SplitOnFirstToken(Path, PathLen, '/');
		Path = SplitStr.Rhs;
	}
	while(Path && *Path) {
		u32 PathLen = GetStringLength(Path);
		split_str_result SplitStr = SplitOnFirstToken(Path, PathLen, '/');
		Path = SplitStr.Rhs;
		if(SplitStr.Lhs) {
			Node = DebugGetFirstChildNodeByName(Node, SplitStr.Lhs, SplitStr.LhsLen);
			if(Node) {
				if(!SplitStr.Rhs) {
					Result = Node;
				}
			}
			else {
				break;
			}
		}
	}

	if(!Result) {
		Result = RootNode;
	}

	return Result;
}

internal void DebugSetProfileNodePath(debug_state *State, debug_profile_node *Node)
{
	if(Node) {
		u32 Depth = 0;
		debug_profile_node *Nodes[MAX_PROFILE_NODE_DEPTH];
		string_buffer Buffer = CreateStringBuffer(State->SelectedNodePath, ArrayCount(State->SelectedNodePath));
		while(Node->Parent) {
			Assert(Depth < MAX_PROFILE_NODE_DEPTH);
			Nodes[Depth++] = Node;
			Node = Node->Parent;
		}
		while(Depth) {
			CatString(&Buffer, "/");
			CatString(&Buffer, Nodes[--Depth]->BlockName);
		}
		NullTerminateString(&Buffer);
	}
}

internal void DebugBuildProfileRecords(debug_state *DebugState, debug_frame *Frame, debug_profile_node *RootNode)
{	
	TIMED_FUNCTION();

	for(u32 i = 0; i < DEBUG_PROFILE_RECORD_HASHTABLE_SIZE; i++) {
		debug_profile_record_hash *Entry = Frame->Records + i;
		DebugFreeRecords(DebugState, Entry);
	}

	for(debug_profile_node *Child = RootNode->FirstChild; Child != 0; Child = Child->Next)
	{
		u32 HashIdx = 0;
		for(char *At = Child->BlockName; *At; At++) {
			HashIdx += *At;
		}
		HashIdx = HashIdx % ArrayCount(Frame->Records);
		debug_profile_record_hash *Entry = Frame->Records + HashIdx;
		debug_profile_record *CurRecord = Entry->Record;
		debug_profile_record *Record = 0;
		for(debug_profile_record *RecordWalker = Entry->Record;
			RecordWalker != 0;
			RecordWalker = RecordWalker->NextInHash)
		{
			if(RecordWalker->BlockName == Child->BlockName) {
				Record = RecordWalker;
				break;
			}
		}

		if(!Record) {
			Record = DebugAllocRecord(DebugState);
			if(Record) {
				Record->BlockName = Child->BlockName;
				Record->NextInHash = Entry->Record;
				Entry->Record = Record;
			}
		}

		if(Record) {
			Record->CycleCount += Child->CycleCount;
			Record->HitCount++;
		}
	}
}

internal debug_profile_node * DebugProfileBars(render_group *Group, debug_profile_node *RootNode, f32 BorderThickness, v2 CursorP, v2 ProfileOrigin, f32 ProfileWidth, f32 ProfileHeight, u32 Depth)
{
	debug_profile_node * Result = 0;
	
	local_persist v4 BlockColors[] = {
		{ 0.0f, 0.0f, 1.0f, 1.0f },
		{ 0.0f, 1.0f, 0.0f, 1.0f },
		{ 0.0f, 1.0f, 1.0f, 1.0f },
		{ 1.0f, 0.0f, 0.0f, 1.0f },
		{ 1.0f, 0.0f, 1.0f, 1.0f },
		{ 1.0f, 1.0f, 0.0f, 1.0f },
		{ 0.0f, 0.5f, 1.0f, 1.0f },
		{ 0.5f, 0.0f, 1.0f, 1.0f },
		{ 0.5f, 0.5f, 1.0f, 1.0f },
		{ 0.0f, 1.0f, 0.5f, 1.0f },
		{ 0.5f, 1.0f, 0.0f, 1.0f },
		{ 0.5f, 1.0f, 0.5f, 1.0f },
		{ 1.0f, 0.0f, 0.5f, 1.0f },
		{ 1.0f, 0.5f, 0.0f, 1.0f },
		{ 1.0f, 0.5f, 0.5f, 1.0f },
		{ 1.0f, 0.5f, 0.5f, 1.0f },
	};
	
	if(Depth) {
		Depth--;
		for(debug_profile_node *Node = RootNode->FirstChild;
			Node;
			Node = Node->Next)
		{
			u32 BlockColorHash = (u32)((u64)Node->BlockName >> 3);
			for(char *CharAt = Node->BlockName;
				*CharAt;
				CharAt++)
			{
				BlockColorHash += *CharAt;
			}
			BlockColorHash = BlockColorHash % ArrayCount(BlockColors);

			f32 BlockWidth = ProfileWidth * ((f32)(Node->CycleCount) / (f32)(RootNode->CycleCount));
			v2 At = ProfileOrigin + V2(ProfileWidth * ((f32)(Node->StartCycles - RootNode->StartCycles) / (f32)RootNode->CycleCount), 0.f);
			v2 Dim = V2(BlockWidth, ProfileHeight);
			rect2 R = MakeRectFromCorners(At, At + Dim);
			PushRectangle(Group, R, BlockColors[BlockColorHash]);
			PushRectangleOutline(Group, R, BorderThickness, V4(0.f, 0.f, 0.f, 1.f));

			if(IsInside(R, CursorP)) {
				Result = Node;
			}

			// NOTE: Lewd
			debug_profile_node *HotChildNode = DebugProfileBars(Group, Node, BorderThickness, CursorP, At, BlockWidth, 0.5f * ProfileHeight, Depth);
			if(HotChildNode) {
				Result = HotChildNode;
			}
		}
		PushRectangleOutline(Group, MakeRectFromCorners(ProfileOrigin, ProfileOrigin + V2(ProfileWidth, ProfileHeight)), 1.f, V4(0.f, 0.f, 0.f, 1.0f));
	}

	return Result;
}

internal void DebugOverlay(render_group *Group, game_state *State, debug_state *DebugState, game_inputs *Inputs, game_audio_buffer *AudioBuffer, window_params *Window, mem_arena *TempMemory)
{
	TIMED_FUNCTION();

	f32 LineAdvance = FontGetLineAdvance(State->Renderer.DebugFont.Data);
	v2 CursorP = V2(Inputs->MouseX * Window->Width, Inputs->MouseY * Window->Height);

	u32 TextCmdBufferSize = KILOBYTES(4);
	void *TextCmdBuffer = PushSize(TempMemory, TextCmdBufferSize);

	formatted_text DebugText;
	BeginFormattedText(&DebugText, TextCmdBuffer, TextCmdBufferSize);

	text_fmt_push_color(V4(1.f, 1.f, 1.f, 1.f));
	text_fmt_push_string("Active block: ");
	text_fmt_push_u32(State->Level.Camera.P.BlockIndex);
	text_fmt_push_newline();
	
	text_fmt_push_string("Player block: ");
	text_fmt_push_u32(State->DebugPlayerBlock);
	text_fmt_push_newline();

	text_fmt_push_string("Loaded entities: ");
	text_fmt_push_u32(State->SimRegion.DebugLoadedEntityCount);
	text_fmt_push_newline();

	text_fmt_push_string("Player Health: ");
	text_fmt_push_u32(State->PlayerHealth);
	text_fmt_push_newline();
	
	text_fmt_push_string("Hit Count: ");
	text_fmt_push_u32(State->DebugHitCount);
	text_fmt_push_newline();
	
	char MemReportBuffers[4][256];
	if(DebugState->ShowMemory) {
		// TODO: Put name into arena?
		mem_arena *MemArenas[] = {
			&State->MemoryArena,
			&State->TransientArena,
			&DebugState->Memory,
		};
		char *MemName[] = { 
			"Persistent", 
			"Transient", 
			"Debug",
		};

		for(u32 i = 0; i < ArrayCount(MemName); i++) {
			PrintMemoryArenaInfo(MemName[i], MemReportBuffers[i], sizeof(MemReportBuffers[i]), MemArenas[i]);
			text_fmt_push_string(MemReportBuffers[i]);
		}

		text_fmt_sprintf(MemReportBuffers[3], sizeof(MemReportBuffers[3]), "VRAM Texture Memory: %d kb\n", State->Renderer.TextureMemoryUsed >> 10);
		text_fmt_push_string(MemReportBuffers[3]);
	}

	text_fmt_push_string("MousePosX: ");
	text_fmt_push_u32((u32)CursorP.x);
	text_fmt_push_newline();
	text_fmt_push_string("MousePosY: ");
	text_fmt_push_u32((u32)CursorP.y);
	text_fmt_push_newline();

	debug_frame *Frame = DebugState->Frames + DebugState->ViewFrameIdx;
	u64 FrameCyclesStart = Frame->CyclesStart;
	u64 FrameCycleCount = Frame->CycleCount;

	f32 FrameTime = (f32)FrameCycleCount / DebugState->CyclesPerSecond;
	text_fmt_push_string("Last frame time: ");
	text_fmt_push_u64_milli((u64)(1000000.f * FrameTime));
	text_fmt_push_newline();
	
	v2 TextCursorP = V2(8.f, Window->Height - State->Renderer.DebugFont.Data->Ascent);
	EndFormattedText();
	v2 LayoutAt = PushFormattedText(Group, &State->Renderer.DebugFont, &DebugText, TextCursorP);
	
	v2 GraphDim = V2(300.f, 100.f);
	v2 GraphAt = V2((f32)Window->Width - 300.f, (f32)Window->Height - 100.f);
	AddSample(&DebugState->FrameGraph, FrameTime);
	DebugTimeGraph(Group, &DebugState->FrameGraph, GraphAt, GraphDim, 0.f, 0.05f);
	GraphAt.y -= GraphDim.y;

	if(DebugState->DrawSoundDisplay) {
		TIMED_BLOCK("DrawSoundDisplay");

		Assert((AudioBuffer->SampleCount & 1) == 0);
		for(u32 i = 0; i < AudioBuffer->SampleCount; i += 2) {
			AddSample(&DebugState->AudioGraph[0], AudioBuffer->Samples[i]);
			AddSample(&DebugState->AudioGraph[1], AudioBuffer->Samples[i + 1]);
		}
		DebugTimeGraph(Group, &DebugState->AudioGraph[0], GraphAt, GraphDim, -32768.f, 32767.f);
		GraphAt.y -= GraphDim.y;
	
		DebugTimeGraph(Group, &DebugState->AudioGraph[1], GraphAt, GraphDim, -32768.f, 32767.f);
		GraphAt.y -= GraphDim.y;
	}

	if(DebugState->DrawProfiler) {
		TIMED_BLOCK("DrawProfiler");

		LayoutAt = DebugTimeline(DebugState, LayoutAt, Group, CursorP, Inputs);
		f32 ProfileWidth = 1024.f;
		f32 ProfileHeight = 128.f;
		LayoutAt.y -= ProfileHeight + 8.f;
		v2 ProfileOrigin = LayoutAt;
		f32 BorderThickness = 1.f;
		rect2 ProfileRect = MakeRectFromCorners(ProfileOrigin, ProfileOrigin + V2(ProfileWidth, ProfileHeight));
		PushRectangle(Group, ProfileRect, V4(0.f, 0.f, 0.f, 0.5f));
		PushRectangleOutline(Group, ProfileRect, BorderThickness, V4(0.f, 0.f, 0.f, 1.f));
		
		debug_profile_node *RootNode = DebugGetProfileNodeFromPath(&DebugState->ProfileRootNode, DebugState->SelectedNodePath);
		u32 MaxProfileBarDepth = 2;
		debug_profile_node *HotNode = DebugProfileBars(Group, RootNode, BorderThickness, CursorP, ProfileOrigin, ProfileWidth, ProfileHeight, MaxProfileBarDepth);

		if(HotNode) {
			if(Inputs->State[BUTTON_MOUSE_LEFT] && Inputs->HalfTransitionCount[BUTTON_MOUSE_LEFT]) {
				if(HotNode->FirstChild) {
					DebugSetProfileNodePath(DebugState, HotNode);
				}
			}
			else {
				char HotNodeText[256];
				f32 TimeSpent = (f32)HotNode->CycleCount / DebugState->CyclesPerSecond;
				text_fmt_sprintf(HotNodeText, ArrayCount(HotNodeText), "%s: %.2f ms, %lld cyc, %.2f%%", 
					HotNode->BlockName, 
					1000.f * ((f32)HotNode->CycleCount / DebugState->CyclesPerSecond),
					HotNode->CycleCount, 
					100.f * (f32)HotNode->CycleCount / (f32)RootNode->CycleCount);
				text_extent TextExtent = GetTextExtent(&State->Renderer.DebugFont, HotNodeText);

				v2 TextP = CursorP + V2(0.f, -TextExtent.LowerRight.y);
				v2 HalfDim = 0.5f * (TextExtent.LowerRight - TextExtent.UpperLeft);
				v2 Center = TextP + TextExtent.UpperLeft + HalfDim;
				rect2 R = MakeRectHalfDim(Center, V2(HalfDim.x, -HalfDim.y));
				PushRectangle(Group, R, V4(0.f, 0.f, 0.f, 0.5f));
		
				PushText(Group, &State->Renderer.DebugFont, HotNodeText, StrLen(HotNodeText), TextP, V4(1.f, 1.f, 1.f, 1.f));
			}
		}
		if(Inputs->State[BUTTON_MOUSE_RIGHT] && Inputs->HalfTransitionCount[BUTTON_MOUSE_RIGHT]) {
			if(IsInside(ProfileRect, CursorP)) {
				DebugSetProfileNodePath(DebugState, RootNode->Parent);
			}
		}

		DebugBuildProfileRecords(DebugState, Frame, RootNode);

		for(u32 i = 0; i < DEBUG_PROFILE_RECORD_HASHTABLE_SIZE; i++) {
			debug_profile_record * Record = Frame->Records[i].Record;
			while(Record) {				
				char RecordText[256];
				f32 AvgCycleCount = (f32)Record->CycleCount / (f32)Record->HitCount;
				text_fmt_sprintf(RecordText, 
					ArrayCount(RecordText), 
					"%s: %.2f ms, %lld cyc, %d hits, %.1f cyc/hits, %.2f%% (%.2f%% per hit)", 
					Record->BlockName, 
					1000.f * ((f32)Record->CycleCount / DebugState->CyclesPerSecond),
					Record->CycleCount, 
					Record->HitCount, 
					AvgCycleCount, 100.f * (f32)Record->CycleCount / (f32)RootNode->CycleCount, 
					100.f * (f32)AvgCycleCount / (f32)RootNode->CycleCount);
		
				LayoutAt.y -= LineAdvance;
				PushText(Group, &State->Renderer.DebugFont, RecordText, StrLen(RecordText), LayoutAt, V4(1.f, 1.f, 1.f, 1.f));
				
				Record = Record->NextInHash;
			}
		}
	}
}

internal void DebugSpline(render_group *Group, parametric_spline *S, f32 Width, u32 SamplePointsPerSegment)
{
	if(S->Count > 1 && SamplePointsPerSegment > 1) {
		v4 Color = V4(0.f, 1.f, 1.f, 1.f);
		u32 NumPoints = SamplePointsPerSegment * (S->Count - 1);
		v2 *Points = (v2*)PushAlloc(NumPoints * sizeof(v2));
		for(u32 i = 0; i < NumPoints; i++) {
			f32 t = (f32)i / (f32)(NumPoints - 1) * (f32)(S->Count - 1);
			Points[i] = SplineAtTime(S, t);
		}
		PushLines(Group, Points, NumPoints, Width, RenderEntryLinesMode_Smooth, Color);
		PopAlloc();
	}

	/*
	glPointSize(4.f);
	glBegin(GL_POINTS);
	glColor4f(1.f, 0.f, 0.f, 1.f);
	for(u32 i = 0; i < S->Count; i++) {
		glVertex2f(S->x[i], S->y[i]);
	}
	glEnd();
	*/
}

internal void DebugSystem(debug_state *DebugState, game_state *State, window_params *Window, game_inputs *Inputs, game_audio_buffer *AudioBuffer, mem_arena *TempMemory, u8 *ConsoleBuffer, umm ConsoleBufferSize)
{	
	TIMED_FUNCTION();

	ProcessDebugInput(DebugState, Inputs, ConsoleBuffer, ConsoleBufferSize);
	
	CollateDebugEvents(DebugState, &State->TransientArena);
	
	render_group Group = AllocateRenderGroup(TempMemory, DEBUG_RENDER_GROUP_MEMORY);
	viewport Viewport = { 0, 0, Window->Width, Window->Height };
	mat4 WindowMatrix = CreateViewProjection(V2(Window->Width / 2.f, Window->Height / 2.f), V2((f32)Window->Width, (f32)Window->Height));
	PushTransformation(&Group, Viewport, WindowMatrix);
	PushBlend(&Group, true);

	if(DebugState->DrawOverlay) {
		DebugOverlay(&Group, State, DebugState, Inputs, AudioBuffer, Window, TempMemory);
	}
	
	if(DebugState->Console.Active) {
		DebugConsole(&Group, &DebugState->Console, &State->Renderer.DebugFont, Window);
	}

	GfxDrawRenderGroup(&State->Renderer, &Group);

    if (DebugState->DrawSpritemaps) {
		// TODO: Have a way to make this platform independent (include in backend implementation)
        // TODO: Have a system to draw to framebuffers and stuff
        // TODO: Highlight topmost sprite the mouse is hovering over
        GLuint Framebuffer;
        glGenFramebuffers(1, &Framebuffer);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, Framebuffer);

        u32 MarginX = 8;
		u32 MarginY = 8;
        u32 OffsetX = MarginX;
        u32 OffsetY = MarginY;
        u32 TargetWidth = SPRITEMAP_LAYER_WIDTH / 2;
        u32 TargetHeight = SPRITEMAP_LAYER_HEIGHT / 2;
        for (u32 i = 0; i < SPRITEMAP_COUNT; i++) {
            glFramebufferTextureLayer(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, State->Renderer.Spritemaps->TexId, 0, i);
            glBlitFramebuffer(0, 0, SPRITEMAP_LAYER_WIDTH, SPRITEMAP_LAYER_HEIGHT, OffsetX, OffsetY, OffsetX + TargetWidth, OffsetY + TargetHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST);
            OffsetX += TargetWidth + MarginX;
        }

		OffsetX = MarginX;
		OffsetY = TargetHeight + 2 * MarginY;
		glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, State->Renderer.GlyphAtlas->Handle, 0);
		glBlitFramebuffer(0, 0, MAX_GLYPH_PIXELS_X * GLYPH_ATLAS_COUNT_X, MAX_GLYPH_PIXELS_Y * GLYPH_ATLAS_COUNT_Y, OffsetX, OffsetY, OffsetX + TargetWidth, OffsetY + TargetHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST);

        glDeleteFramebuffers(1, &Framebuffer);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    }

	if(DebugState->ReloadLevel) {
		LoadLevel(State);
		DebugState->ReloadLevel = false;
	}
}
#else

internal void DebugConsolePushString(char *Str, debug_msg_type Type)
{
}

#endif
