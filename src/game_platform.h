#ifndef __GAME_PLATFORM_H__
#define __GAME_PLATFORM_H__

#include "game_config.h"
#include "game_types.h"

#ifdef _DEBUG
#define Assert(expr) if(!(expr)) __debugbreak()
#define InvalidCodepath if(1) __debugbreak()
#define InvalidDefaultCase default: if(1) __debugbreak()
#define NotImplemented if(1) __debugbreak()
#else
#define Assert(expr) (void)(0)
#define InvalidCodepath (void)(0)
#define InvalidDefaultCase (void)(0)
#define NotImplemented (void)(0)
#endif

#define KILOBYTES(x) (1024L * x)
#define MEGABYTES(x) (1024L * KILOBYTES(x))
#define GIGABYTES(x) (1024L * MEGABYTES(x))
#define ArrayCount(x) (sizeof(x) / sizeof(x[0]))

#define global_persist static
#define local_persist static
#define internal static

#define OffsetOf(Struct, Member) (umm)(&((Struct *)0)->Member)

#include "string_util.h"

enum file_type
{
	FileType_Invalid,

	FileType_Shader,
	FileType_Level,
	FileType_Image,
	FileType_Dialog,
};

struct file_info
{
	b32 Valid;
	file_type Type;
	u32 Size;

	void * _Handle;
};

enum platform_cmd_type
{
	PlatformCmdType_HelloWorld,
	PlatformCmdType_Build,
	PlatformCmdType_ChangeDir,
	PlatformCmdType_List,
	PlatformCmdType_Cat,
};

struct platform_memory_block
{
	u8 * Base;
	umm Size;
	umm Used;
	
	platform_memory_block *Prev;
};

#define PLATFORM_LOAD_OPENGL_FUNCTION(name) void * name(char *Func)
typedef PLATFORM_LOAD_OPENGL_FUNCTION(platform_load_opengl_function);

#define PLATFORM_SEND_COMMAND(name) void name(platform_cmd_type Type, void *UserData)
typedef PLATFORM_SEND_COMMAND(platform_send_command);

#define PLATFORM_OPEN_FILE(name) file_info name(char *Name, file_type Type)
typedef PLATFORM_OPEN_FILE(platform_open_file);

#define PLATFORM_READ_ENTIRE_FILE(name) b32 name(file_info *Info, void *Buffer)
typedef PLATFORM_READ_ENTIRE_FILE(platform_read_entire_file);

#define PLATFORM_CLOSE_FILE(name) void name(file_info *Info)
typedef PLATFORM_CLOSE_FILE(platform_close_file);

#define PLATFORM_WRITE_FILE(name) b32 name(char *Name, file_type Type, void *Buffer, u32 Size)
typedef PLATFORM_WRITE_FILE(platform_write_file);

#define PLATFORM_ALLOCATE_MEMORY(name) platform_memory_block * name(umm Size)
typedef PLATFORM_ALLOCATE_MEMORY(platform_allocate_memory);

#define PLATFORM_DEALLOCATE_MEMORY(name) void name(platform_memory_block *Block)
typedef PLATFORM_DEALLOCATE_MEMORY(platform_deallocate_memory);

struct platform_api
{
	platform_open_file            * OpenFile;
	platform_read_entire_file     * ReadEntireFile;
	platform_close_file           * CloseFile;
	platform_write_file           * WriteFile;
	platform_load_opengl_function * LoadProcAddressGL;
	platform_send_command         * SendCommand;
	platform_allocate_memory      * AllocateMemory;
	platform_deallocate_memory    * DeallocateMemory;
};
extern platform_api * Platform;

#include "game_intrin.h"
#include "game_math.h"
#include "game_memory.h"
#include "game_debug.h"
#include "file_format.h"
#include "font.h"

enum GAME_INPUT_BUTTON
{
    BUTTON_MOUSE_LEFT,
    BUTTON_MOUSE_RIGHT,

    BUTTON_LEFT,
    BUTTON_RIGHT,
    BUTTON_UP,
    BUTTON_DOWN,
    BUTTON_ACTION,
	BUTTON_START,
	BUTTON_SPECIAL,

	BUTTON_DEBUG_OVERLAY,
    BUTTON_DEBUG_TOGGLE_SPRITES,
	BUTTON_DEBUG_CONSOLE,
    BUTTON_DEBUG_TOGGLE_SOUND,

    NUM_BUTTONS,
    UNKNOWN_BUTTON
};

#define DEBUG_INPUT_BUFFER_MAX_SIZE 1024
struct game_inputs
{
    u8 HalfTransitionCount[NUM_BUTTONS];
    u8 State[NUM_BUTTONS];

	f32 MouseX;
	f32 MouseY;

	f32 TimeStep;
	u32 Seed;

#ifdef GAME_DEBUG
	u32 DebugInputBufferSize;
	u8  *DebugInputBuffer;
#endif
};

struct game_audio_buffer
{
	u32 SamplesPerSecond;
	u32 SampleCount;
	s16 *Samples;
};

struct debug_game_audio
{
	b32 Valid;
	audio_data_header *Header;
};

struct game_memory
{
	struct game_state *GameState;
	struct debug_state *DebugState;

	void *StackMemory;
	umm StackMemorySize;

	game_audio_buffer *AudioBuffer;
	debug_game_audio *DebugGameAudio;
	ascii_font DebugFont;

	umm ConsoleBufferSize;
	u8 *ConsoleBuffer;

	platform_api PlatformAPI;
};

struct window_params
{
    u32 Width;
    u32 Height;
};

#define GAME_UPDATE_AND_RENDER(name) void name(game_memory *GameMemoryIn, window_params *WindowParamsIn, game_inputs *InputsIn, b32 DllReloaded)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);

#endif
