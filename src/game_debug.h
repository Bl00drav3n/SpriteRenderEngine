#ifndef __GAME_DEBUG_H__
#define __GAME_DEBUG_H__

struct debug_state;
extern debug_state * GlobalDebugState;

enum debug_event_type
{
	EVENT_TYPE_OPEN_BLOCK,
	EVENT_TYPE_CLOSE_BLOCK,
};

struct debug_event
{
	// TODO: Can reduce size
	u32 Type;
	u32 ThreadId;
	u64 TimeStamp;
	char *BlockName;
};

struct debug_stored_event
{
	debug_event Event;

	union {
		struct {
			debug_stored_event *Prev;
			debug_stored_event *Next;
		};
		struct {
			umm Pad;
			debug_stored_event *NextFree;
		};
	};
};

struct debug_profile_record
{
	char *BlockName;
	u64 CycleCount;
	u32 HitCount;

	union {
		debug_profile_record *NextInHash;
		debug_profile_record *NextFree;
	};
};

struct debug_profile_record_hash
{
	debug_profile_record *Record;
};

struct debug_profile_node
{
	char *BlockName;
	u64 StartCycles;
	u64 CycleCount;

	debug_profile_node *Parent;
	debug_profile_node *FirstChild;

	debug_profile_node *Next;
};

#define DEBUG_PROFILE_RECORD_HASHTABLE_SIZE 64
struct debug_frame
{
	u64 CyclesStart;
	u64 CycleCount;

	debug_stored_event EventSentinel;
	debug_profile_record_hash Records[DEBUG_PROFILE_RECORD_HASHTABLE_SIZE];
};

#define TIME_GRAPH_SAMPLE_COUNT 1470
struct debug_time_graph
{
	u32 NewestSample;
	u32 OldestSample;
	f32 Samples[TIME_GRAPH_SAMPLE_COUNT];
};

#define MAX_DEBUG_EVENTS_PER_PAGE 1024
struct debug_event_page
{
	u32 NumEvents;
	debug_event Events[MAX_DEBUG_EVENTS_PER_PAGE];

	debug_event_page *Prev;
	debug_event_page *Next;

	debug_event_page *NextFree;
};

struct debug_event_buffer
{
	u64 FrameStartCycles;
	u64 FrameCycleCount;
	u32 LostEvents;

	debug_event_page PageSentinel;
};

#define MAX_DEBUG_CONSOLE_LINE_CHAR_COUNT 64
struct debug_console_text_line
{
	// TODO: use string_buffer?
	u8 Buffer[MAX_DEBUG_CONSOLE_LINE_CHAR_COUNT];
	u32 Size;
};

#define DEBUG_CONSOLE_CMDLINE_START_SYMBOL '$'
#define MAX_DEBUG_CONSOLE_LINES 256

struct command_line
{
	b32 AutoComplete;
	u32 PatternMatch;
	u32 PatternSize;
	char *Pattern;

	u32 BeginOffset;
	u32 LastCmdSize;
	debug_console_text_line Line;
};
struct debug_console
{
	b32 Active;
	u32 PushedLineCount;
	u32 MostRecentLineIdx;
	u32 NumberOfDisplayedLines;
	command_line CmdLine;
	debug_console_text_line Lines[MAX_DEBUG_CONSOLE_LINES];
};

#define MAX_DEBUG_FRAMES 256
#define MAX_NODE_PATH 1024
struct debug_state
{
	b32 RecordingPaused;
	f32 CyclesPerSecond;
	
	u32 EventReadBuffer;
	u32 EventWriteBuffer;
	debug_event_buffer EventBuffers[2];
	debug_event_page *NextFreePage;

	debug_stored_event *FirstFreeEvent;
	debug_profile_record *FirstFreeRecord;
	debug_profile_node ProfileRootNode;
	char SelectedNodePath[MAX_NODE_PATH];
	
	u32 ViewFrameIdx;
	u32 RecordingFrameIdx;
	debug_frame Frames[MAX_DEBUG_FRAMES];

	mem_arena Memory;

	debug_time_graph FrameGraph;
	debug_time_graph AudioGraph[2];

	debug_console Console;
	
	b32 ShowMemory;
	b32 DrawOverlay;
	b32 DrawProfiler;
	b32 DrawSoundDisplay;
	b32 DrawIsoGrid;
    b32 DrawSpritemaps;
	b32 ReloadLevel;
    b32 MuteAudio;
};

internal inline u64 GetCycleCount()
{
	return __rdtsc();
}

internal inline void SwapEventBuffers(debug_state *State)
{
	u32 ReadBuffer = State->EventWriteBuffer;
	u32 WriteBuffer = 1 - ReadBuffer;
	State->EventWriteBuffer = WriteBuffer;
	State->EventReadBuffer = ReadBuffer;

	Assert(ReadBuffer != WriteBuffer);
	Assert(ReadBuffer == 0 || ReadBuffer == 1);
	Assert(WriteBuffer == 0 || WriteBuffer == 1);
}

internal debug_event_page * AllocDebugEventPage(debug_state *State)
{
	debug_event_page *Result = 0;
	if(State->NextFreePage) {
		Result = State->NextFreePage;
		State->NextFreePage = Result->NextFree;
	}
	else {
		Result = push_type(&State->Memory, debug_event_page);
	}

	Assert(Result != 0);
	return Result;
}

internal inline debug_event_page * AddDebugEventPage(debug_state *State, debug_event_buffer *Buffer)
{
	debug_event_page *Page = AllocDebugEventPage(State);
	Page->Next = &Buffer->PageSentinel;
	Page->Prev = Buffer->PageSentinel.Prev;
	Page->Prev->Next = Page;
	Page->Next->Prev = Page;

	return Page;
}

internal void FreeDebugEventPages(debug_state *State, debug_event_buffer *Buffer)
{
	debug_event_page *Page = Buffer->PageSentinel.Next;
	while(Page != &Buffer->PageSentinel) {
		debug_event_page *Next = Page->Next;

		Page->NumEvents = 0;
		Page->Next = Page->Prev = 0;

		Page->NextFree = State->NextFreePage;
		State->NextFreePage = Page;

		Page = Next;
	}

	Buffer->PageSentinel.Next = Buffer->PageSentinel.Prev = &Buffer->PageSentinel;
}

internal debug_event * AllocDebugEvent(debug_state *State, debug_event_buffer *Buffer)
{
	debug_event_page *Page = Buffer->PageSentinel.Prev;
	if(Page == &Buffer->PageSentinel || (Page != &Buffer->PageSentinel && Page->NumEvents == MAX_DEBUG_EVENTS_PER_PAGE)) {
		Page = AddDebugEventPage(State, Buffer);
	}

	Assert(Page != 0);
	
	debug_event *Result = 0;
	if(Page) {
		Result = Page->Events + Page->NumEvents++;
	}
	return Result;
}

internal inline void PushDebugEvent(debug_state *State, char *BlockName, debug_event_type Type)
{
	debug_event_buffer *WriteBuffer = State->EventBuffers + State->EventWriteBuffer;
	debug_event *Result = AllocDebugEvent(State, WriteBuffer);
	Result->Type = Type;
	Result->ThreadId = GetThreadID();
	Result->TimeStamp = GetCycleCount();
	Result->BlockName = BlockName;
}

struct debug_timed_block
{
	char *Name;
	debug_timed_block(const char *BlockName, const char *File, u32 Line, const char *Function) {
		Name = (char*)BlockName;
		if(GlobalDebugState) {
			PushDebugEvent(GlobalDebugState, Name, EVENT_TYPE_OPEN_BLOCK);
		}
	}

	~debug_timed_block() {
		if(GlobalDebugState) {
			PushDebugEvent(GlobalDebugState, Name, EVENT_TYPE_CLOSE_BLOCK);
		}
	}
};

internal inline void DebugBeginFrame()
{
	if(GlobalDebugState) {
		debug_state *State = GlobalDebugState;
		debug_event_buffer *WriteBuffer = State->EventBuffers + State->EventWriteBuffer;
		WriteBuffer->LostEvents = 0;
		WriteBuffer->FrameStartCycles = GetCycleCount();
		FreeDebugEventPages(State, WriteBuffer);
	}
}

internal inline void DebugEndFrame()
{
	if(GlobalDebugState) {
		debug_state *State = GlobalDebugState;
		debug_event_buffer *WriteBuffer = State->EventBuffers + State->EventWriteBuffer;
		WriteBuffer->FrameCycleCount = GetCycleCount() - WriteBuffer->FrameStartCycles;
		SwapEventBuffers(State);
	}
}

#ifdef GAME_DEBUG
/*
#define TIMED_BLOCK__(BlockName, Line) debug_timed_block TimedBlock_##Line(BlockName, __FILE__, __LINE__, __FUNCTION__)
#define TIMED_BLOCK_(File, BlockName, Line) TIMED_BLOCK__(BlockName, Line)
#define TIMED_BLOCK() TIMED_BLOCK_(__FILE__, __FUNCTION__, __LINE__)
*/

#define TIMED_BLOCK(BlockName) debug_timed_block TimedBlock(BlockName, __FILE__, __LINE__, __FUNCTION__);
#define TIMED_FUNCTION() TIMED_BLOCK(__FUNCTION__)

#else
#define TIMED_BLOCK(...)
#define TIMED_FUNCTION(...)
#endif // NOTE: GAME_DEBUG

#endif

enum debug_msg_type
{
	MSG_INFO,
	MSG_WARNING,
	MSG_ERROR,
};

internal void DebugConsolePushString(const char *Str, debug_msg_type Type = MSG_INFO);
