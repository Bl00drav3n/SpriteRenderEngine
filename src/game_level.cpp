b32 ValidateLevelFile(level_file_header *Header)
{
	b32 Result = Header->MagicNo == LEVEL_FILE_MAGIC;
	return Result;
}

internal level_block * AllocateLevelBlock(level *Level, mem_arena *Arena)
{
	level_block *Result = 0;
	if(Level->FirstFreeBlock) {
		Result = Level->FirstFreeBlock;
		Level->FirstFreeBlock = Result->NextFree;
	}
	else {
		Result = push_type(Arena, level_block);
	}

	if(Result) {
		// TODO: Set only EntityCount to 0?
		ZeroStruct(Result);
		
		Result->Prev = Level->LevelBlocksSentry.Prev;
		Result->Next = &Level->LevelBlocksSentry;
		Result->Next->Prev = Result;
		Result->Prev->Next = Result;
		Result->Index = Level->AllocatedBlockCount++;
	}
	
	return Result;
}

internal void FreeLevelBlock(level *Level, level_block *Block)
{
	Assert(Level->AllocatedBlockCount > 0);
	Block->Next->Prev = Block->Prev;
	Block->Prev->Next = Block->Next;
	Block->NextFree = Level->FirstFreeBlock;
	Level->FirstFreeBlock = Block;
	Level->AllocatedBlockCount--;
}

internal void FreeLevel(level *Level)
{
	for(level_block *Block = Level->LevelBlocksSentry.Next;
		Block != &Level->LevelBlocksSentry;
		Block = Block->Next)
	{
		FreeLevelBlock(Level, Block);
		Assert(Block->Next);
	}
	Level->RunningEntityIndex = 1;
	Level->Loaded = false;
}

internal void InitLevel(level *Level, v2 CameraBounds)
{
	v2 HalfDim = V2(150.f, 150.f);
	Level->BlockDim = 2 * HalfDim;
	camera *Camera = &Level->Camera;
	Camera->P.Offset = V2();
	Camera->P.BlockIndex = 0;
	Camera->Bounds = CameraBounds;
	Camera->Speed = 50.f;
	Camera->Velocity = V2(Camera->Speed, 0.f);
}

internal void SaveLevel(game_state *State)
{
	level *Level = &State->Level;
	if(Level->Loaded) {
		u32 EntityCount = 0;
		for(level_block *Block = Level->LevelBlocksSentry.Next;
			Block != &Level->LevelBlocksSentry;
			Block = Block->Next)
		{
			EntityCount += Block->EntityCount;
		}

		temporary_memory TempMemory = BeginTempMemory(&State->TransientArena);
		u32 EntityTableSize = sizeof(level_file_entity_entry) * EntityCount;
		level_file_entity_entry* EntityTable = (level_file_entity_entry*)PushSize(&State->TransientArena, EntityTableSize);

		level_file_header Header = {};
		Header.MagicNo = LEVEL_FILE_MAGIC;
		Header.EntityCount = EntityCount;
		Header.BlockCount = Level->AllocatedBlockCount;
		Header.EntityTableOffset = sizeof(Header);
		u64 DataOffset = Header.EntityTableOffset + EntityTableSize;
		//ToDo: PlayerPos

		u64 Offset = 0;
		if(EntityTable){
			level_file_entity_entry *Entry;
			Entry = EntityTable;
			for(level_block *Block = Level->LevelBlocksSentry.Next;
				Block != &Level->LevelBlocksSentry;
				Block = Block->Next)
			{
				for(u32 j = 0; j < Block->EntityCount; j++, Entry++) {
					stored_entity *Entity = &Block->Entities[j];
					Entry->Type = Entity->Type;
					Entry->Offset = DataOffset + Offset;
					Offset += GetEntityDataSize(Entity);
				}
			}
			Assert(Entry == EntityTable + EntityCount);

			//Offset represents also the data volume
			u32 DataSize = SafeTruncateU64ToU32(Offset);
			u32 TotalSize = sizeof(Header) + EntityTableSize + DataSize;
			u8* FileBuffer = push_array(&State->TransientArena, u8, TotalSize);
			CopyMemory(FileBuffer, &Header, sizeof(Header));
			CopyMemory(FileBuffer + Header.EntityTableOffset, EntityTable, EntityTableSize);
			
			Entry = EntityTable;
			for(level_block *Block = Level->LevelBlocksSentry.Next;
				Block != &Level->LevelBlocksSentry;
				Block = Block->Next)
			{
				for(u32 j = 0; j < Block->EntityCount; j++, Entry++) {
					stored_entity *Entity = &Block->Entities[j];
					//ToDo: Write Data to Buffer properly
					entity_data_header *DataHeader = (entity_data_header*)(FileBuffer + Entry->Offset);
					DataHeader->Pos = Entity->Position;
					DataHeader->Faction = Entity->Faction;
				}
			}
			Assert(Entry == EntityTable + EntityCount);

			Platform->WriteFile("Level", FileType_Level, FileBuffer, TotalSize);
		}

		EndTempMemory(&TempMemory);
	}
}

internal void LoadLevel(game_state *State)
{
	// TODO: This does not seem to work right now!
	// TODO: Check nested temporary memories work correctly
	// TODO: Streamline entity save/load (SpawnPlayer creates a SimEntity and needs a region, Floater needs a SimEntity reference, etc.)
	level *Level = &State->Level;
	FreeLevel(Level);

	file_info Info = Platform->OpenFile("Level", FileType_Level);
	if(Info.Valid) {
		mem_arena *Arena = &State->TransientArena;
		temporary_memory TempMem = BeginTempMemory(Arena);
		u8 *Data = (u8*)PushSize(Arena, Info.Size);
		if(Platform->ReadEntireFile(&Info, Data)) {
			level_file_header *Header = (level_file_header*)Data;
			if(ValidateLevelFile(Header)) {
				for(u32 i = 0; i < Header->BlockCount; i++) {
					AllocateLevelBlock(Level, &State->MemoryArena);
				}
				level_file_entity_entry *EntityTable = (level_file_entity_entry*)(Data + Header->EntityTableOffset);
				for(u32 EntityIdx = 0;
					EntityIdx < Header->EntityCount;
					EntityIdx++)
				{
					level_file_entity_entry *EntityEntry = EntityTable + EntityIdx;
					entity_data_header *DataHeader = (entity_data_header*)(Data + EntityEntry->Offset);
					switch(EntityEntry->Type) {
					case EntityType_Enemy:
						SpawnEnemy(State, DataHeader->Pos);
						break;
					//InvalidDefaultCase;
					}
				}
				
				// TODO: Load from file?
				InitLevel(Level, State->CameraBounds);
				Level->Loaded = true;
			}
		}
		EndTempMemory(&TempMem);
	}
}
