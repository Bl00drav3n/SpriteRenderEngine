#ifndef __GAME_MEMORY_H__
#define __GAME_MEMORY_H__

// NOTE: Basic memory structures
// mem_arena         - push only
// temporary_memory - snapshot mem_arena state
// stack            - push/pop with memory-block tagging

static inline void MemAlignPow2(void **Ptr, umm *Size, u8 AlignmentMinusOne)
{	
	u64 Offset = (u64)(*Ptr);
	u32 Padding = (u32)((-(s64)Offset) & AlignmentMinusOne);
	Offset = (Offset + AlignmentMinusOne) & ~AlignmentMinusOne;

	*Ptr = (void*)Offset;
	*Size -= Padding;
}

static inline void MemAlign4(void **Ptr, umm *Size)
{
	MemAlignPow2(Ptr, Size, 0x03);
}

static inline void MemAlign8(void **Ptr, umm *Size)
{
	MemAlignPow2(Ptr, Size, 0x07);
}

static inline void MemAlign16(void **Ptr, umm *Size)
{
	MemAlignPow2(Ptr, Size, 0x0F);
}

static inline void MemAlign32(void **Ptr, umm *Size)
{
	MemAlignPow2(Ptr, Size, 0x1F);
}

static inline void MemAlign64(void **Ptr, umm *Size)
{
	MemAlignPow2(Ptr, Size, 0x3F);
}

// NOTE: Convenience functions
#define ZeroArray(Array, Count) ClearMemoryToZero((Array), sizeof(*(Array)) * (Count))
#define ZeroStruct(Struct) ClearMemoryToZero((Struct), sizeof(*(Struct)))
internal inline void ClearMemoryToZero(void *Memory, umm ByteCount)
{
	u8 *Ptr = (u8*)Memory;
	for(umm i = 0; i < ByteCount; i++) {
		Ptr[i] = 0;
	}
}

#undef CopyMemory
internal inline void CopyMemory(void *Dst, void *Src, u32 SrcSize)
{
	u8 *S = (u8*)Src;
	u8 *D = (u8*)Dst;
	for(u32 i = 0; i < SrcSize; i++) {
		*D++ = *S++;
	}
}

// SECTION: mem_arena

// NOTE: uses a simple API where you can only push to the top (dynamically growing)
#define MEM_ARENA_MAGIC 0x12345678
struct mem_arena
{
	u32 Magic;
	umm MinBlockSize;
	platform_memory_block *Block;

	u32 NumBlocks;
	s32 TempCount;
};

#if 0
static inline mem_arena
CreateMemoryArena(void *StartAddress, umm Size, umm MinBlockSize = 0)
{
	mem_arena Result = {};
	Result.Base = (u8*)StartAddress;
    Result.Used = 0;
	Result.Size = Size;
	Result.MinBlockSize = MinBlockSize;

	Result.NumBlocks = 1;
	Result.TempCount = 0;

	return Result;
}
#endif

static inline mem_arena
CreateMemoryArena(umm MinBlockSize = 0)
{
	mem_arena Result = {};
	Result.Magic = MEM_ARENA_MAGIC;
	Result.MinBlockSize = MinBlockSize;
	
	return Result;
}

internal inline void 
FreeLastBlock(mem_arena *Arena)
{
	Assert(Arena->NumBlocks);
	if(Arena->NumBlocks) {
		platform_memory_block *Free = Arena->Block;
		Arena->Block = Free->Prev;
		Arena->NumBlocks--;
		Platform->DeallocateMemory(Free);
	}
}

enum arena_push_flag
{
	ArenaFlag_NoClear = 0x01,
};

struct arena_push_params
{
	u32 Flags;
	umm Alignment;
};

internal inline arena_push_params DefaultArenaParams()
{
	arena_push_params Result;
	Result.Flags = ArenaFlag_NoClear;
	Result.Alignment = 4;
	return Result;
}

internal inline arena_push_params ArenaParams(umm Alignment, arena_push_flag Flags)
{
	arena_push_params Result;
	Result.Flags = Flags;
	Result.Alignment = Alignment;
	return Result;
}

internal inline umm GetAlignmentOffset(platform_memory_block *Block, umm Alignment)
{
	umm Result = 0;
	umm ResultPtr = (umm)Block->Base + Block->Used;
	umm AlignmentMask = Alignment - 1;
	if(ResultPtr & AlignmentMask) {
		Result = Alignment - (ResultPtr & AlignmentMask);
	}
	return Result;
}

internal inline umm GetEffectiveSizeForAlignment(platform_memory_block *Block, umm Size, umm Alignment)
{
	umm Result = Size;
	if(Block && Size) {
		umm Offset = GetAlignmentOffset(Block, Alignment);
		Result += Offset;
	}

	return Result;
}

#define DEFAULT_MINIMUM_ARENA_BLOCK_SIZE MEGABYTES(1)
// NOTE: Push a block of memory
internal void *
PushSize(mem_arena *Arena, umm SizeInit, arena_push_params Params = DefaultArenaParams())
{
	void * Result = 0;
	platform_memory_block *Block = Arena->Block;
	umm Size = GetEffectiveSizeForAlignment(Block, SizeInit, Params.Alignment);
	if(!Block || Block->Used + Size > Block->Size) {
		if(!Arena->MinBlockSize) {
			Arena->MinBlockSize = DEFAULT_MINIMUM_ARENA_BLOCK_SIZE;
		}

		umm BlockSize = MAXIMUM(Arena->MinBlockSize, Size);
		platform_memory_block *NewBlock = Platform->AllocateMemory(BlockSize);
		if(NewBlock) {
			NewBlock->Prev = Block;
			Arena->Block = NewBlock;
			Arena->NumBlocks++;
		}
		Block = NewBlock;
	}
	
	if(Block) {
		umm Offset = GetAlignmentOffset(Block, Params.Alignment);
		Result = Block->Base + Block->Used + Offset;
		Block->Used += Size;

		Assert(((umm)Result & (Params.Alignment - 1)) == 0);

		if(!(Params.Flags & ArenaFlag_NoClear)) {
			ClearMemoryToZero(Result, SizeInit);
		}
	}

	return Result;
}

// NOTE: Release all allocated memory blocks
static inline void
ResetArena(mem_arena *Arena)
{
	while(Arena->NumBlocks > 0) {
		FreeLastBlock(Arena);
	}
}

#define push_type(Arena, type, ...) (type*)PushSize(Arena, sizeof(type), __VA_ARGS__)
#define push_array(Arena, type, Count, ...) (type*)PushSize(Arena, Count * sizeof(type), __VA_ARGS__)

#define BootstrapStruct(type, Member, ...) (type*)BootstrapPushSize(sizeof(type), OffsetOf(type, Member), ## __VA_ARGS__)
internal void * BootstrapPushSize(umm Size, umm ArenaOffset, umm MinBlockSize = 0)
{
	mem_arena Bootstrap = CreateMemoryArena(MinBlockSize);
	void *Struct = PushSize(&Bootstrap, Size);
	*(mem_arena*)((u8*)Struct + ArenaOffset) = Bootstrap;

	return Struct;
}

// SECTION: temporary_memory

// NOTE: Use this to snapshot the state of a given mem_arena so you can
// reset it back to this state when you are done. 
// Think of BeginTempMemory/EndTempMemory as a push/pop pair.

struct temporary_memory
{
	b32 Valid;
	mem_arena *Arena;
	platform_memory_block *Block;
	umm Used;
};

// NOTE: Take snapshot of current Arena state, increase refcount
internal inline temporary_memory 
BeginTempMemory(mem_arena *Arena)
{
	temporary_memory Result = {};
	Result.Arena = Arena;
	if(Arena->Block) {
		Result.Block = Arena->Block;
		Result.Used = Arena->Block->Used;
		Arena->TempCount++;
	}
	Result.Valid = true;
	
	return Result;
}

// NOTE: Reset Arena state to saved state, decrease refcount, remove all newly allocated blocks
internal inline void 
EndTempMemory(temporary_memory *Temp)
{
	if(Temp->Valid) {
		mem_arena *Arena = Temp->Arena;
		while(Arena->Block != Temp->Block) {
			FreeLastBlock(Arena);
		}

		if(Arena->Block) {
			Arena->Block->Used = Temp->Used;
		}
		Arena->TempCount--;
		Temp->Valid = false;
	}
}

// SECTION: stack

// NOTE: Simple stack implementation
// A stack memory entry is always preceded
// by a stack_block_header that provides information
// about the memory block that was allocated. In the future
// this header might grow to up to 16bytes and might contain
// additional information like a magic value and a block
// identifier. In the future CreateStack will enforce 4 or 16
// byte alignment of the base pointer.
// The stack grows top-down (starts at higher memory address), so if
// you have the pointer to a block of memory, subtract the header size from
// it to get the header location.

// High memory
// |  _________  --- Base + Size (invalid)
// | |xxxxxxxxx| -
// | |xxxxxxxxx| --- User memory
// | |xxxxxxxxx| -
// | |_________| --- (Header+1)    --> top
// | |_________| --- Base + Offset --> begin of top memory block header
// | |         | -
// | |         | --- Free memory
// | |         | -
// | |_________| --- Base
// V 
// Low memory

struct stack
{
	u8 *Base;
	umm Size;
	umm Offset;
};

// NOTE: should be multiple of 4 bytes
struct stack_block_header
{
	u32 Size;
};

#define ARRAY_COUNT(x) (sizeof(x) / sizeof(x[0]))

#endif
