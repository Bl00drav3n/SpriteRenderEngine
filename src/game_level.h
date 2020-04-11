#define MAX_ENTITIES_PER_BLOCK 256
struct level_block
{
	u32           Index;
	u32           EntityCount;
	stored_entity Entities[MAX_ENTITIES_PER_BLOCK];

	level_block *Prev;
	level_block *Next;
	level_block *NextFree;
};

struct level
{
	b32			  Loaded;
	camera        Camera;
	v2            BlockDim;

	u32           AllocatedBlockCount;
	level_block   LevelBlocksSentry;
	level_block * FirstFreeBlock;

	u32           RunningEntityIndex; // TODO: Protect from overflow?
};

struct level_file_header
{
	u32			   MagicNo;    
	world_position PlayerSpawn;
	u32            EntityCount;
	u32            BlockCount; 
	u64            EntityTableOffset;
};

struct level_file_entity_entry
{
	u32	Type;
	u64	Offset;
};

internal level_block * GetLevelBlockFromIndex(level *Level, u32 Index)
{
	TIMED_FUNCTION();

	level_block *Result = 0;
	for(level_block *Block = Level->LevelBlocksSentry.Next;
		Block != &Level->LevelBlocksSentry;
		Block = Block->Next)
	{
		if(Block->Index == Index) {
			Result = Block;
		}
	}

	return Result;
}

// NOTE: Map a world position into the correct block
// IMPORTANT: Can only calculate adjacent blocks!
internal void RecanonicalizeWorldPosition(level *Level, world_position *P)
{
	TIMED_FUNCTION();

	v2 RegionHalfDim = 0.5f * Level->BlockDim;

	if(P->Offset.x >= RegionHalfDim.x) {
		if(P->BlockIndex + 1 < Level->AllocatedBlockCount) {
			P->Offset = P->Offset - V2(Level->BlockDim.x, 0.f);
			P->BlockIndex++;
		}
		else {
			P->Offset.x = RegionHalfDim.x;
		}
	}
	else if(P->Offset.x < -RegionHalfDim.x) {
		if(P->BlockIndex > 0) {
			P->Offset = P->Offset + V2(Level->BlockDim.x, 0.f);
			P->BlockIndex--;
		}
		else {
			P->Offset.x = -RegionHalfDim.x;
		}
	}

	if(P->Offset.y > RegionHalfDim.y) {
		P->Offset.y = RegionHalfDim.y;
	}
	else if(P->Offset.y < -RegionHalfDim.y) {
		P->Offset.y = -RegionHalfDim.y;
	}

	Assert(P->BlockIndex < Level->AllocatedBlockCount);
	Assert(P->Offset.x >= -Level->BlockDim.x && P->Offset.x < Level->BlockDim.x);
	Assert(P->Offset.y >= -Level->BlockDim.y && P->Offset.y < Level->BlockDim.y);
}

// TODO: This only works for blocks adjacent to the active block at the moment!
internal world_position GetCanonicalWorldPosition(level *Level, v2 P)
{
	TIMED_FUNCTION();

	world_position Result;
	v2 BlockHalfDim = 0.5f * Level->BlockDim;
	u32 CameraBlock = Level->Camera.P.BlockIndex;
	v2 CameraOffset = Level->Camera.P.Offset;
	v2 CameraBlockCenter = -CameraOffset;
	v2 CameraBlockP = P - CameraBlockCenter;
	
	Result.BlockIndex = CameraBlock;
	Result.Offset = CameraBlockP;
	if(Result.Offset.x < -BlockHalfDim.x) {
		while(Result.Offset.x < -BlockHalfDim.x) {
			if(Result.BlockIndex == 0) {
				Result.Offset.x = -BlockHalfDim.x;
				break;
			}
			else {
				Result.BlockIndex--;
				Result.Offset.x += Level->BlockDim.x;
			}
		}
	}
	else if(Result.Offset.x > BlockHalfDim.x) {
		while(Result.Offset.x > BlockHalfDim.x) {
			if(Result.BlockIndex == Level->AllocatedBlockCount - 1) {
				Result.Offset.x = BlockHalfDim.x;
				break;
			}
			else {
				Result.BlockIndex++;
				Result.Offset.x -= Level->BlockDim.x;
			}
		}
	}

	Result.Offset.y = Clamp(Result.Offset.y, -BlockHalfDim.y, BlockHalfDim.y);

	// TODO: Should P->WorldP mapping be bijective?
	Assert(Result.BlockIndex < Level->AllocatedBlockCount);
	Assert(Result.Offset.x >= -BlockHalfDim.x && Result.Offset.x <= BlockHalfDim.x);
	Assert(Result.Offset.y >= -BlockHalfDim.y && Result.Offset.y <= BlockHalfDim.y);

	return Result;
}

internal inline world_position MakeWorldPosition(v2 P, u32 Idx)
{
	world_position Result;
	Result.BlockIndex = Idx;
	Result.Offset = P;

	return Result;
}

internal v2 MapIntoCameraSpace(level *Level, world_position P)
{
	TIMED_FUNCTION();

	v2 Result;
	camera *Camera = &Level->Camera;
	
	f32 Delta = Level->BlockDim.x;
	Result = P.Offset - Camera->P.Offset;
	if(Camera->P.BlockIndex > P.BlockIndex) {
		Result.x -= Delta * (Camera->P.BlockIndex - P.BlockIndex);
	}
	else if(Camera->P.BlockIndex < P.BlockIndex) {
		Result.x += Delta * (P.BlockIndex - Camera->P.BlockIndex);
	}

	return Result;
}