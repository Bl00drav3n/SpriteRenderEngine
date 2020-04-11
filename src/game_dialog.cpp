internal void loadDialog(game_state *State)
{
	file_info Info = Platform->OpenFile("dialogtext", FileType_Dialog);
	if(Info.Valid) {
		mem_arena *Arena = &State->TransientArena;
		temporary_memory TempMem = BeginTempMemory(Arena);
		u8 *Data = (u8*)PushSize(Arena, Info.Size);
		if(Platform->ReadEntireFile(&Info, Data)) {
			// Parse stuff for drawing here...
			int a = 0;
		}
		Platform->CloseFile(&Info);
		EndTempMemory(&TempMem);
	}
}