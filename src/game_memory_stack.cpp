// NOTE: Create the stack and align to 4 byte boundary
stack CreateStack(void *StartAddress, umm Size)
{
	stack Result = {};

	MemAlign4(&StartAddress, &Size);
	Result.Base = (u8*)StartAddress;
	Result.Offset = Size;
	Result.Size = Size;

	return Result;
}

stack CreateStack(mem_arena *Arena, umm Size)
{
	stack Result = {};
	void *Memory = PushSize(Arena, Size);
	if(Memory) {
		Result = CreateStack(Memory, Size);
	}

	return Result;
}

// NOTE: Push a block of memory onto the stack (actual size is Size + sizeof(stack_block_header))
void *StackPush(stack *Stack, u32 Size)
{
	// TODO: Enforce 4 byte boundary
	void *Result = 0;
	if(Size > 0) {
		u32 BlockSize = Size + sizeof(stack_block_header);
		BlockSize += (u32)((-(s32)BlockSize) & 0x03);
		if(BlockSize <= Stack->Offset) {
			Stack->Offset -= BlockSize;
			stack_block_header *Entry = (stack_block_header*)(Stack->Base + Stack->Offset);
			Entry->Size = BlockSize;
			Result = Entry + 1;
			Assert(((u64)Result & 0x03) == 0x00);
			Assert((Stack->Offset & 0x03) == 0x00);
		}
	}

	return Result;
}

// NOTE: Pop top block of memory from the stack
void StackPop(stack *Stack)
{
	if(Stack->Offset < Stack->Size) {
		stack_block_header *Block = (stack_block_header*)(Stack->Base + Stack->Offset);
		Stack->Offset += Block->Size;
	}
}

// NOTE: General purpose stack
global_persist stack *GlobalStack = 0;

internal void GlobalStackInitialize(void *StackMemory, umm MemorySize)
{
	Assert(StackMemory && MemorySize);
	GlobalStack = (stack*)StackMemory;
	*GlobalStack = CreateStack(GlobalStack + 1, MemorySize - sizeof(stack));
}

internal inline void * PushAlloc(u32 Size)
{
	void *Result = StackPush(GlobalStack, Size);
	return Result;
}

internal inline void PopAlloc()
{
	StackPop(GlobalStack);
}