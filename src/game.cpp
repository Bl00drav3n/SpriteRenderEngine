#include "game_platform.h"
#include "game_rng.h"
#include "game_renderer.h"
#include "game_render_group.h"
#include "game_particles.h"
#include "game_isosurface.h"
#include "game_text.h"
#include "game_dialog.h"
#include "game.h"

global_persist mem_arena STBI_GlobalMemory;
#define STBI_MALLOC(Size)           PushSize(&STBI_GlobalMemory, Size)
#define STBI_REALLOC(Ptr, NewSize)  STBI_MALLOC(NewSize)
#define STBI_FREE(Ptr)              0
#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#include "stb_image.h"

global_persist platform_api * Platform = 0;

#include "game_memory_stack.cpp"
#include "spline.cpp"
#include "game_render_group.cpp"
#include "game_renderer.cpp"
#include "game_renderer_opengl.cpp"
#include "game_particles.cpp"
#include "game_entity.cpp"
#include "game_level.cpp"
#include "game_text.cpp"
#include "game_isosurface.cpp"
#include "game_debug.cpp"
#include "game_dialog.cpp"

internal void BeginSimRegion(game_state *State)
{
	TIMED_FUNCTION();

	sim_region *Region = &State->SimRegion;
	level *Level = &State->Level;
	Region->TransMemory = &State->TransientArena;

	// NOTE: Clear sim region
	Region->DebugLoadedEntityCount = 0;
	Region->EntityListSentry.Next = 0;
	entity_id Id = InvalidEntityId();
	for(u32 i = 0; i < MAX_SIM_ENTITY_COUNT; i++) {
		Region->EntityTable[i].Id = Id;
	}

	u32 ActiveBlock = Level->Camera.P.BlockIndex;
	
	State->LoadedLevelBlocksCount = 0;
	u32 BlockIdsToLoad[3] = { (u32)-1, (u32)-1, (u32)-1 };
	for(u32 i = 0; i < 3; i++) {
		State->LoadedLevelBlocks[i] = (u32)-1;
	}
	u32 NumBlocksToLoad = 0;
	s32 Low = (ActiveBlock > 0) ? -1 : 0;
	s32 High = (ActiveBlock < Level->AllocatedBlockCount - 1) ? 1 : 0 ;
	for(s32 Index = Low; Index <= High; Index++) {
		BlockIdsToLoad[NumBlocksToLoad++] = ActiveBlock + Index;
	}

	for(u32 BlockIdIndex = 0; BlockIdIndex < NumBlocksToLoad; BlockIdIndex++) {
		// TODO: Use list for adjacency
		level_block *LevelBlock = GetLevelBlockFromIndex(Level, BlockIdsToLoad[BlockIdIndex]);
		if(LevelBlock) {
			for(u32 EntityIndex = 0; EntityIndex < LevelBlock->EntityCount; EntityIndex++) {
				stored_entity *StoredEntity = LevelBlock->Entities + EntityIndex;
				LoadEntity(State, StoredEntity);
			}

			LevelBlock->EntityCount = 0;
		}
		else {
			InvalidCodepath;
		}
	}

	for(u32 i = 0; i < 3; i++) {
		State->LoadedLevelBlocks[i] = BlockIdsToLoad[i];
	}
	State->LoadedLevelBlocksCount = NumBlocksToLoad;
}

internal void EndSimRegion(game_state *State)
{
	TIMED_FUNCTION();

	sim_region *Region = &State->SimRegion;

	// TODO: This assumes we don't touch any non-loaded blocks!
	for(sim_entity_item *Item = Region->EntityListSentry.Next;
		Item;
		Item = Item->Next)
	{
		sim_entity *SimEntity = Item->Entity;
		StoreEntity(State, SimEntity);
	}
}

//NOTE: Is Pos.xy the center or top-left/bottom-right corner?
internal b32 Collided(collision_volume A, collision_volume B, v2 PosA, v2 PosB)
{
	TIMED_FUNCTION();

	b32 Result = false;

	if(A.Type == B.Type) {
		// NOTE: A and B have same collision type
		collision_type Type = A.Type;
		switch(Type) {
			case CollisionType_Sphere:
			{
				// NOTE: Sphere-Sphere collision
				v2 Diff = PosA - PosB;
				f32 DistanceRadial = A.Radius + B.Radius;
				f32 DistanceRadial2 = DistanceRadial * DistanceRadial;
				f32 DistanceCenter2 = Dot(Diff, Diff);

				Result = (DistanceRadial2 >= DistanceCenter2);
			} break;
			case CollisionType_Box:
			{
				// NOTE: Box-Box collision
				rect2 TestVolume = MinkowskiSumRect2(A.XYExtendFromOrigin, B.XYExtendFromOrigin, PosA);
				v2 Min = TestVolume.Center - TestVolume.HalfDim;
				v2 Max = TestVolume.Center + TestVolume.HalfDim;
				Result = ((PosB.x >= Min.x) && 
						  (PosB.x <= Max.x) && 
						  (PosB.y >= Min.y) && 
						  (PosB.y <= Max.y));
			} break;
			
			InvalidDefaultCase;
		}
	}
	else {
		//TODO: Handle Box<->Sphere Collision properly. For now it's only approximated
		//NOTE: This calls Collided recursively!
		switch(A.Type) {
			case CollisionType_Box:
			{
				collision_volume NewA;
				NewA.Type = CollisionType_Sphere;
				NewA.Radius = Length(A.XYExtendFromOrigin);
				Result = Collided(NewA, B, PosA, PosB);
			} break;
			case CollisionType_Sphere:
			{
				Result = Collided(B, A, PosB, PosA);
			} break;
			
			InvalidDefaultCase;
		}
	}

	return Result;
}

internal sound * CreateSound(game_state *State, audio_file *File, u32 SoundIndex, f32 Volume = 1.f, f32 Delay = 0.f, s32 Loop = 0)
{
	sound *Sound = 0;
	if(State->FirstFreeSound) {
		Sound = State->FirstFreeSound;
		State->FirstFreeSound = Sound->NextFree;
	}
	else if(State->SoundCount < MAX_SOUND_COUNT) {
		Sound = push_type(&State->MemoryArena, sound);
		State->SoundCount++;
	}

	if(Sound) {
		audio_data_entry *Entry = File->AudioTable + SoundIndex;
		Sound->SampleCount = Entry->Size / sizeof(s16);
		Sound->Samples = (s16*)(File->Base + Entry->Offset);
		Sound->PlayCursor = 0;
		Sound->Loop = Loop;
		Sound->StartDelay = Delay;
		Sound->Volume = Volume;

		sound *Prev = State->SoundListSentry.Prev;
		Prev->Next = Sound;
		Sound->Prev = Prev;
		Sound->Next = &State->SoundListSentry;
		State->SoundListSentry.Prev = Sound;

		Sound->Valid = 1;
	}

	return Sound;
}

internal void FreeSound(game_state *State, sound *Sound)
{
	if(Sound) {
		sound *Next = State->FirstFreeSound;
		Sound->NextFree = Next;
		State->FirstFreeSound = Sound;

		Sound->Prev->Next = Sound->Next;
		Sound->Next->Prev = Sound->Prev;

		Sound->Valid = 0;
	}
}

internal inline u32 MixSound(sound *Sound, f32 *SampleOut, u32 RequestedSampleCount)
{
	TIMED_FUNCTION();

	u32 SamplesToMix = MINIMUM(Sound->SampleCount - Sound->PlayCursor, RequestedSampleCount);
	s16 *SampleIn = Sound->Samples + Sound->PlayCursor;
	for(u32 i = 0; i < SamplesToMix; i++) {
		s32 Sample = *SampleIn++;
		Sample += 32768;
		*SampleOut++ += Clamp(Sound->Volume * (-1.f + 2.f * ((f32)Sample / 65535.f)), -1.f, 1.f);
	}
	Sound->PlayCursor = (u32)(SampleIn - Sound->Samples);

	return SamplesToMix;
}

static void ProcessSound(game_state *State, game_audio_buffer *Output, mem_arena *Memory, f32 TimeStep)
{
	TIMED_FUNCTION();

	temporary_memory TempMemory = BeginTempMemory(Memory);
	f32 *SampleBuffer = push_array(Memory, f32, Output->SampleCount, ArenaParams(16, ArenaFlag_NoClear));
	ZeroArray(SampleBuffer, Output->SampleCount);

	// NOTE: Mix samples
	for(sound *Sound = State->SoundListSentry.Next; Sound != &State->SoundListSentry; Sound = Sound->Next) {
		Sound->StartDelay = ClampAboveZero(Sound->StartDelay - TimeStep);
		if(Sound->StartDelay == 0.f) {
			if(Sound->Samples) {
				u32 SamplesMixed = MixSound(Sound, SampleBuffer, Output->SampleCount);
				if(Sound->PlayCursor == Sound->SampleCount) {
					if(Sound->Loop) {
						if(Sound->Loop > 0) {
							Sound->Loop--;
						}
						Sound->PlayCursor = 0;
						MixSound(Sound, SampleBuffer + SamplesMixed, MINIMUM(Sound->SampleCount, Output->SampleCount - SamplesMixed));
					}
					else {
						FreeSound(State, Sound);
					}
				}
			}
		}
	}

	s16 *SampleOut = Output->Samples;
	f32 *SampleIn = SampleBuffer;
	for(u32 i = 0; i < Output->SampleCount; i++) {
		f32 Sample = *SampleIn++;
		Sample = (Sample + 1.f) / 2.f;
		*SampleOut++ = (s16)Clamp(-32768.f + 65535.f * Sample, -32768.f, 32767.f);
	}

	EndTempMemory(&TempMemory);
}

internal audio_file CreateAudioFile(debug_game_audio *Audio)
{
	audio_file Result = {};
	if(Audio->Valid) {
		Result.Valid = true;
		Result.Base = (u8*)Audio->Header;
		Result.AudioEntryCount = Audio->Header->EntryCount;
		Result.AudioTable = (audio_data_entry*)(Audio->Header + 1);
	}

	return Result;
}

internal void LoadDebugFont(game_state *State, ascii_font *Font)
{
	render_state *Renderer =  &State->Renderer;
	font *DebugFont = &State->DebugFont;
	DebugFont->Data = Font;
	for(u32 GlyphIndex = 0; 
		GlyphIndex < ArrayCount(DebugFont->GlyphTextures); 
		GlyphIndex++)
	{
		glyph *Glyph = Font->Glyphs + GlyphIndex;
		u8 Pixels[4 * MAX_GLYPH_PIXELS_X * MAX_GLYPH_PIXELS_Y];
		u8 *Dst = Pixels;
		u8 *Src = Font->Glyphs[GlyphIndex].Bitmap;
		for(s32 y = 0; y < Glyph->Height; y++) {
			for(s32 x = 0; x < Glyph->Width; x++) {
				u8 R = *Src++;
				u8 G = *Src++;
				u8 B = *Src++;
				u8 A = *Src++;
				f32 R32 = MapU8ToF32(R);
				f32 G32 = MapU8ToF32(G);
				f32 B32 = MapU8ToF32(B);
				f32 A32 = MapU8ToF32(A);
				*Dst++ = MapF32ToU8(R32 * A32);
				*Dst++ = MapF32ToU8(G32 * A32);
				*Dst++ = MapF32ToU8(B32 * A32);
				*Dst++ = A;
			}
		}
		DebugFont->GlyphTextures[GlyphIndex] = CreateTexture(Renderer, Glyph->Width, Glyph->Height, Pixels);
	}
}

#define MAX_METABALL_OBJECTS 32
struct metaball_object_buffer
{
	u32 Count;
	v2 P[MAX_METABALL_OBJECTS];
};

internal inline void AddMetaballObject(metaball_object_buffer *Buffer, v2 P)
{
	if(Buffer->Count < MAX_METABALL_OBJECTS) {
		Buffer->P[Buffer->Count++] = P;
	}
}

internal void SimulationTick(game_state *State, render_group *RenderGroup, sim_region *SimRegion, game_inputs *Inputs, f32 TimeDelta)
{
	TIMED_FUNCTION();

	particle_cache *ParticleCache = State->ParticleCache;
	level *Level = &State->Level;
	camera *Camera = &Level->Camera;

	// TODO: Properly seperate update and rendering?
	for(sim_entity_item *ListItem = SimRegion->EntityListSentry.Next;
		ListItem;
		ListItem = ListItem->Next)
	{
		TIMED_BLOCK("EntityEntryUpdate");
		sim_entity *Entity = ListItem->Entity;
		Assert(Entity->Type);
		Assert(Entity->Id.Value);
		Assert(Entity->Faction);
		Entity->TexCoords = DefaultTexCoords();

		// NOTE: Update entity (calculate velocity, fire weapons, animation)
		if(Entity->Tick) {
			Entity->Tick(State, Inputs, SimRegion, Camera, Level, Entity, TimeDelta);
		}
		else {
			InvalidCodepath;
		}

		if(!IsFlagSet(Entity, SimEntityFlag_Delete)) {
			// NOTE: Update position
			Entity->NextPosition = Entity->LastPosition + Entity->Velocity * TimeDelta;
		}
	}

	// NOTE: 2nd pass, collision resolution and rendering
	for(sim_entity_item *ListItem = SimRegion->EntityListSentry.Next;
		ListItem;
		ListItem = ListItem->Next)
	{
		TIMED_BLOCK("EntityEntryUpdate2");
		sim_entity *Entity = ListItem->Entity;
		for(sim_entity_item *TestItem = ListItem->Next;
			TestItem;
			TestItem = TestItem->Next)
		{
			// NOTE: Collision testing and resolve
			// TODO: Implement check for previous and next Frame.
			sim_entity *OtherEntity = TestItem->Entity;
			if(Entity->Faction != OtherEntity->Faction) {
				if(Collided(Entity->Col, OtherEntity->Col, Entity->NextPosition, OtherEntity->NextPosition)) {
					if(Entity->Type == EntityType_Projectile && OtherEntity->Type == EntityType_Projectile) {
						// NOTE: At the moment no handling for proj-proj collisions
					}
					else if(Entity->Type != EntityType_Projectile && OtherEntity->Type != EntityType_Projectile) {
						sim_entity *Player = 0;
						sim_entity *Item = 0;

						if(Entity->Type == EntityType_Player && OtherEntity->Type == EntityType_Item) {
							Player = Entity;
							Item = OtherEntity;
						}
						else if(Entity->Type == EntityType_Item && OtherEntity->Type == EntityType_Player) {
							Item = Entity;
							Player = OtherEntity;
						}

						if(Player && Item) {
							Assert(Player->Type == EntityType_Player);
							Assert(Item->Type == EntityType_Item);
							FlagForRemoval(Item);
							switch(Item->ItemType) {
								case ItemType_Shield:
								{
									SpawnShield(State, SimRegion, Player);
								} break;

								InvalidDefaultCase;
							}
						}
						else {
							//NOTE: Ignore collision with shields if it's not with a projectile or enemy.
							if(Entity->Type == EntityType_Shield && OtherEntity->Type != EntityType_Enemy)
								FlagForRemoval(OtherEntity);
							else if(OtherEntity->Type == EntityType_Shield && Entity->Type != EntityType_Enemy)
								FlagForRemoval(Entity);
							else {
								FlagForRemoval(Entity);
								FlagForRemoval(OtherEntity);
							}
						}
					}
					else {
						// NOTE: Either A or B is a projectile
						// TODO: Do we want to remove objects based on their health always? - Might as well, if 1 hit is deadly just set health to 1 or dmg to absurd.
						State->DebugHitCount++;
						sim_entity *Projectile, *Target;
						if(Entity->Type == EntityType_Projectile) {
							Projectile = Entity;
							Target = OtherEntity;
						}
						else {
							Projectile = OtherEntity;
							Target = Entity;
						}

						// NOTE: Ignore Items for now
						// TODO: Do we want to be able to destroy items or should they just drop out of the screen if the player is too slow?
						if(Target->Type != EntityType_Item)
						{
							FlagForRemoval(Projectile);

							// NOTE: Remove the non-projectile if health is low enough.
							Target->Health -= Projectile->DamagePerHit;
							if(Target->Health <= 0) {
								if(Target->Id.Value != State->PlayerRef.Value)
									FlagForRemoval(Target);
								if(Target->Type == EntityType_Enemy) {
									SpawnExplosion(ParticleCache, Entity->LastPosition, 75.f);
									if(GetUniformRandom01(&State->Entropy) <= 0.1f) {
										// NOTE: 10% chance of shield drop
										SpawnItem(State, SimRegion, Target->NextPosition, ItemType_Shield);
									}
								}
							}
						}
					}
				}
			}
			else {
				// NOTE: Entities in same factions don't collide
			}
		}

		layer_id Layer = GET_LAYER(1);
		if(Entity->Type == EntityType_Shield) {
			Layer = GET_LAYER(0);
		}

		PushSprite(RenderGroup, Layer, Entity->NextPosition, &Entity->Sprite);
	}

#if 0
	// NOTE: Draw some shitty blobs
	if(State->IsoGrid.Valid) {
		TIMED_BLOCK("Metaballs");

		v2 *Objects = Metaballs.P;
		u32 ObjectCount = Metaballs.Count;
		BuildIsoGrid(&State->IsoGrid, Objects, ObjectCount);

		if(GlobalDebugState && GlobalDebugState->DrawIsoGrid) {
			DebugDrawIsoGrid(RenderGroup, &State->IsoGrid);
		}

		edge_buffer *EdgeBuffer = AllocateEdgeBuffer(State->Renderer.PerFrameMemory);
		if(EdgeBuffer) {
			GetIsoSurfaceEdges(&State->IsoGrid, Objects, ObjectCount, EdgeBuffer);
			PushEdges(RenderGroup, EdgeBuffer->Edges, EdgeBuffer->Count, V4(1.f, 1.f, 0.f, 1.f));
		}
		else {
			InvalidCodepath;
		}
	}
	else {
		InvalidCodepath;
	}
#endif

	UpdateAndRenderParticles(ParticleCache, Camera, TimeDelta, RenderGroup);
}

internal void CreateTestLevel(game_state *State)
{
	level *Level = &State->Level;
	FreeLevel(Level);
	InitLevel(Level, State->CameraBounds);

	for(u32 i = 0; i < 10; i++) {
		AllocateLevelBlock(Level, &State->MemoryArena);
	}
	
	u32 Index = 0;
	for(level_block *Block = Level->LevelBlocksSentry.Next;
		Block != &Level->LevelBlocksSentry;
		Block = Block->Next, Index++)
	{
		SpawnEnemy(State, MakeWorldPosition(V2(-140.f,  -45.f), Index));
		SpawnEnemy(State, MakeWorldPosition(V2(-120.f,   50.f), Index));
		SpawnEnemy(State, MakeWorldPosition(V2(-110.f,  -10.f), Index));
		SpawnEnemy(State, MakeWorldPosition(V2(-100.f,   25.f), Index));
		SpawnEnemy(State, MakeWorldPosition(V2( -80.f,  -80.f), Index));
		SpawnEnemy(State, MakeWorldPosition(V2( -35.f,  -40.f), Index));
		SpawnEnemy(State, MakeWorldPosition(V2( -10.f,   10.f), Index));
		SpawnEnemy(State, MakeWorldPosition(V2(   5.f,  -25.f), Index));
		SpawnEnemy(State, MakeWorldPosition(V2(  25.f,   75.f), Index));
		SpawnEnemy(State, MakeWorldPosition(V2(  60.f,  -30.f), Index));
		SpawnEnemy(State, MakeWorldPosition(V2(  80.f,  -60.f), Index));
		SpawnEnemy(State, MakeWorldPosition(V2(  95.f,   20.f), Index));
		SpawnEnemy(State, MakeWorldPosition(V2( 100.f,  -90.f), Index));
		SpawnEnemy(State, MakeWorldPosition(V2( 120.f, -100.f), Index));
		SpawnEnemy(State, MakeWorldPosition(V2( 130.f,   20.f), Index));
	}

	Level->Loaded = true;
}

internal inline v2 Screen01ToViewSpace(camera *Camera, v2 ScreenP)
{
	v2 Result = -0.5f * Camera->Bounds + Hadamard(ScreenP, Camera->Bounds);
	return Result;
}

internal void PlayRandomMusic(game_state *State)
{	
	if(State->Music.Valid) {
		u32 MusicIndex = GetUniformRandomU32(&State->Entropy) % State->Music.AudioEntryCount;
		Assert(MusicIndex < State->Music.AudioEntryCount);
		State->CurMusic = CreateSound(State, &State->Music, MusicIndex, 1.f, 1.f);
	}
}

internal game_state * InitializeGame(void *StackMemory, umm StackMemorySize)
{
	game_state *GameState = BootstrapStruct(game_state, MemoryArena);
	GameState->TransientArena = CreateMemoryArena();
	GameState->Renderer.Memory = CreateMemoryArena();
	GameState->Renderer.PerFrameMemory = &GameState->TransientArena;
	
	AllocateParticleCache(GameState);
	
	GlobalStackInitialize(StackMemory, StackMemorySize);

	return GameState;
}

internal void RenderGame(render_state *Renderer, render_group *GameRenderGroup)
{
	TIMED_FUNCTION();

	DrawRenderGroup(Renderer, GameRenderGroup);

}

extern "C" __declspec(dllexport) 
GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
	game_memory *GameMemory = GameMemoryIn;
	window_params *Window = WindowParamsIn;
	game_inputs *Inputs = InputsIn;
	f32 TimeStep = Inputs->TimeStep;
	
	Platform = &GameMemory->PlatformAPI;
	
	if(DllReloaded) {
		GlobalStack = (stack*)GameMemory->StackMemory;
		ReloadRenderBackend();
		if(GlobalDebugState) {
			DebugFreeAllFrames(GlobalDebugState);
		}
		return;
	}
	
#ifdef GAME_DEBUG
	if(!GameMemory->DebugState) {
		GameMemory->DebugState = DEBUGInit();
	}
#endif

	game_state *State = GameMemory->GameState;
	GlobalDebugState = GameMemory->DebugState;

	{ TIMED_BLOCK("GameUpdateAndRender");
    if (!GameMemory->GameState) {
        GameMemory->GameState = State = InitializeGame(GameMemory->StackMemory, GameMemory->StackMemorySize);
        State->TestTextures[0] = CreateDebugTexture(&State->Renderer, V4(1.f, 0.f, 1.f, 0.9f));
        State->TestTextures[1] = CreateDebugTexture(&State->Renderer, V4(1.f, 0.f, 0.f, 0.9f));
        State->TestTextures[2] = CreateDebugTexture(&State->Renderer, V4(0.f, 1.f, 0.f, 0.9f));
        State->TestTextures[3] = CreateDebugTexture(&State->Renderer, V4(0.f, 0.f, 1.f, 0.9f));
        State->TestTextures[4] = CreateDebugTexture(&State->Renderer, V4(1.f, 1.f, 1.f, 0.9f));
        State->TestTextures[5] = CreateDebugTexture(&State->Renderer, V4(0.f, 1.f, 1.f, 0.9f));
        State->TiledTestTexture = CreateTiledDebugTexture(&State->Renderer);
        State->EnemyTexture = LoadTextureFromFile(&State->Renderer, "cirgreed");
        State->FloaterTexture = LoadTextureFromFile(&State->Renderer, "hotpokket");

        // TODO: Remove
        State->Entropy = CreateRNG(Inputs->Seed);
        State->SoundListSentry.Next = &State->SoundListSentry;
        State->SoundListSentry.Prev = &State->SoundListSentry;

        if (GameMemory->DebugGameAudio->Valid) {
            State->Music = CreateAudioFile(GameMemory->DebugGameAudio);
            Assert(State->Music.AudioEntryCount > 0);
        }

        LoadDebugFont(State, &GameMemory->DebugFont);

        level *Level = &State->Level;
        Level->LevelBlocksSentry.Next = Level->LevelBlocksSentry.Prev = &Level->LevelBlocksSentry;

        f32 MetersPerPixel = 300.f / (f32)Window->Width;
        State->CameraBounds = MetersPerPixel * V2((f32)Window->Width, (f32)Window->Height);
        CreateTestLevel(State);
        SaveLevel(State);
        Assert(Level->Loaded);

        State->DebugHitCount = 0;

#if 0
        spritemap_array Spritemaps = AllocateSpritemaps();
        sprite_pixel pixels[SPRITE_WIDTH * SPRITE_HEIGHT];
        for (u32 i = 0; i < ArrayCount(pixels); i++) {
            pixels[i].r = 0xff;
            pixels[i].g = 0xff;
            pixels[i].b = 0xff;
            pixels[i].a = 0xff;
        }
        glBindTexture(GL_TEXTURE_2D_ARRAY, Spritemaps.TexId);
        for (u32 z = 0; z < SPRITEMAP_COUNT; z++) {
            for (u32 y = 0; y < SPRITEMAP_DIM_Y; y++) {
                for (u32 x = 0; x < SPRITEMAP_DIM_X; x++) {
                    u32 OffsetX = x * SPRITE_WIDTH;
                    u32 OffsetY = y * SPRITE_HEIGHT;
                    glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, OffsetX, OffsetY, z, SPRITE_WIDTH, SPRITE_HEIGHT, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
                }
            }
        }
        glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
#else
        spritemap_array Spritemaps = AllocateTestSpritemaps();
#endif
		
        State->Renderer.SpriteProgram = CreateSpriteProgram(&State->Renderer.Memory);
        State->Renderer.Spritemaps = Spritemaps;

		State->Renderer.LineProgram = CreateLineProgram(&State->Renderer.Memory);

		InitializeParticleCache(State->ParticleCache);
		
		State->Initialized = 1;
	}

	render_state *Renderer = &State->Renderer;
	
	v4 ClearColor;
	switch(State->Level.Camera.P.BlockIndex % 4) {
		case 0:
			ClearColor = V4(0.2f, 0.2f, 0.2f, 0.f);
			break;
		case 1:
			ClearColor = V4(0.2f, 0.f, 0.f, 0.f);
			break;
		case 2:
			ClearColor = V4(0.f, 0.2f, 0.f, 0.f);
			break;
		case 3:
			ClearColor = V4(0.0f, 0.f, 0.2f, 0.f);
			break;
	}
	
	viewport Viewport = { 0, 0, Window->Width, Window->Height };
    render_group GameRenderGroup = AllocateRenderGroup(Renderer->PerFrameMemory, MEGABYTES(32));;
	PushClear(&GameRenderGroup, ClearColor);
	PushBlend(&GameRenderGroup, true);
	PushTransformation(&GameRenderGroup, Viewport, CreateViewProjection(V2(), State->Level.Camera.Bounds));
	
	camera *Camera = &State->Level.Camera;
	Camera->P.Offset = Camera->P.Offset + Camera->Speed * TimeStep * V2(1.f, 0.f);
	RecanonicalizeWorldPosition(&State->Level, &Camera->P);
	if(Camera->P.BlockIndex == State->Level.AllocatedBlockCount - 1 && Camera->P.Offset.x >= 0.f) {
		Camera->Speed = 0.f;
		Camera->P.Offset.x = 0.f;
		Camera->Velocity = V2();
	}

	v2 CursorP = Screen01ToViewSpace(Camera, V2(Inputs->MouseX, Inputs->MouseY));
#if 0
	if(!Inputs->State[BUTTON_MOUSE_LEFT] && Inputs->HalfTransitionCount[BUTTON_MOUSE_LEFT]) {
		PushSplinePoint(&State->DebugSpline, CursorP);
	}
	if(!Inputs->State[BUTTON_MOUSE_RIGHT] && Inputs->HalfTransitionCount[BUTTON_MOUSE_RIGHT]) {
		PopSplinePoint(&State->DebugSpline);
	}
#endif

	f32 TimeDelta = TimeStep;
	BeginSimRegion(State);
	SimulationTick(State, &GameRenderGroup, &State->SimRegion, Inputs, TimeDelta);
	// NOTE: Add player if reference lost
	sim_entity *Player = GetSimEntityById(&State->SimRegion, State->PlayerRef);
	if(!Player) {
		SpawnPlayer(State, &State->SimRegion, V2());
	}
	else {
		State->PlayerHealth = (u32)Player->Health;
	}
	EndSimRegion(State);

	// NOTE: Level block boundaries
	v2 BlockHalfDim = 0.5f * State->Level.BlockDim;
	v2 BlockDebug = -State->Level.Camera.P.Offset - V2(State->Level.BlockDim.x, 0.f);
	v4 LevelBlockOutlineColor = V4(1.f, 1.f, 0.f, 1.f);
	for(u32 i = 0 ; i < State->Level.AllocatedBlockCount; i++) {
		v2 LowerLeft = BlockDebug - BlockHalfDim;
		v2 UpperRight = BlockDebug + BlockHalfDim;
		PushRectangleOutline(&GameRenderGroup, MakeRectFromCorners(LowerLeft, UpperRight), 1.f, LevelBlockOutlineColor);

		BlockDebug.x += State->Level.BlockDim.x;
	}

	DebugSpline(&GameRenderGroup, &State->DebugSpline, 0.1f, 25);

	debug_point_buffer *PointBuffer = &State->DebugPointBuffer;
	if(Inputs->State[BUTTON_MOUSE_LEFT] && Inputs->HalfTransitionCount[BUTTON_MOUSE_LEFT]) {
		PointBufferPushPoint(PointBuffer, CursorP);
	}
	if(Inputs->State[BUTTON_MOUSE_RIGHT] && Inputs->HalfTransitionCount[BUTTON_MOUSE_RIGHT]) {
		PointBufferClear(PointBuffer);
	}

	if(PointBuffer->Count >= 2) {
		PushLines(&GameRenderGroup, PointBuffer->Points, 2, 1.f, RenderEntryLinesMode_Smooth, V4(1.f, 1.f, 1.f, 1.f));
	}
	if(PointBuffer->Count == 4) {
		PushLines(&GameRenderGroup, PointBuffer->Points + 2, 2, 1.f, RenderEntryLinesMode_Smooth, V4(1.f, 1.f, 1.f, 1.f));

		f32 t, s;
		ray2 RayA = Ray2FromPoints(PointBuffer->Points[0], PointBuffer->Points[1]);
		ray2 RayB = Ray2FromPoints(PointBuffer->Points[2], PointBuffer->Points[3]);
		if(RayIntersect2(RayA, RayB, &t, &s)) {
			v2 Intersection = RayA.Start + t * RayA.Dir;
			rect2 R = MakeRectDim(Intersection, V2(1.f, 1.f));
			PushRectangle(&GameRenderGroup, R, V4(1.f, 0.f, 0.f, 1.f));
		}
	}

	//loadDialog(State);

	RenderGame(Renderer, &GameRenderGroup);

	if(!State->CurMusic || !State->CurMusic->Valid) {
		PlayRandomMusic(State);
	}

	// NOTE: Sound mixing
	ProcessSound(State, GameMemory->AudioBuffer, &State->TransientArena, TimeStep);
	} // NOTE: TIMED_BLOCK("GameUpdateAndRender");
	
	if(GlobalDebugState) {
		DebugSystem(GlobalDebugState, State, Window, Inputs, GameMemory->AudioBuffer, &State->TransientArena, GameMemory->ConsoleBuffer, GameMemory->ConsoleBufferSize);
        if (GlobalDebugState->MuteAudio) {
            ZeroArray(GameMemory->AudioBuffer->Samples, GameMemory->AudioBuffer->SampleCount);
        }
	}
	
	ResetArena(&State->TransientArena);
}
