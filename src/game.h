#ifndef __GAME_H__
#define __GAME_H__

#define MAX_SOUND_COUNT 16
#define MAX_WEAPON_COUNT 256
#define MAX_ENTITY_COUNT 1024

#define MAX_SPLINE_CTRL_PTS 16

#define LEVEL_FILE_MAGIC 0xFEA9

#define GLYPH_ATLAS_COUNT_X 16
#define GLYPH_ATLAS_COUNT_Y 16

enum sprite_type {
	SPRITE_INVALID,

	SPRITE_PLAYER,
	SPRITE_PROJECTILE,
	SPRITE_ENEMY,
	SPRITE_FLOATER,
	SPRITE_SHIELD,
	SPRITE_EXPLOSION,
	SPRITE_JET_EXHAUST,
	SPRITE_FLOATER_SINGLE_SHOT,
	SPRITE_ENEMY_SINGLE_SHOT,
	SPRITE_ITEM,

	SPRITE_COUNT
};

struct parametric_spline
{
	u32 Count;
	f32 t[MAX_SPLINE_CTRL_PTS];
	f32 x[MAX_SPLINE_CTRL_PTS];
	f32 x2[MAX_SPLINE_CTRL_PTS];
	f32 y[MAX_SPLINE_CTRL_PTS];
	f32 y2[MAX_SPLINE_CTRL_PTS];
};

enum layer_enum
{
	LAYER_0,
	LAYER_1,
	LAYER_2,

	LayerCount
};

struct world_position
{
	u32 BlockIndex;
	v2  Offset;
};

enum collision_type
{
	CollisionType_Box,
	CollisionType_Sphere,
};

struct collision_volume
{

	collision_type Type;

	union
	{
		f32 Radius;
		v2 XYExtendFromOrigin;
	};
};

internal inline collision_volume MakeBoxCollision(v2 XYExtend)
{
	collision_volume Result;
	Result.Type = CollisionType_Box;
	Result.XYExtendFromOrigin = XYExtend;

	return Result;
}

internal inline collision_volume MakeSphereCollision(f32 Radius)
{
	collision_volume Result;
	Result.Type = CollisionType_Sphere;
	Result.Radius = Radius;

	return Result;
}

enum item_type
{
	ItemType_Shield,
};

enum faction_type
{
	FactionType_Null,

	FactionType_Player,
	FactionType_Enemy,
	FactionType_Collectible,
};

struct sprite
{
	sprite_type Type;
	basis2d Basis;
	v4 Tint;
};

internal inline sprite MakeSprite(sprite_type Type, v4 Tint = V4(1, 1, 1, 1))
{
	sprite Result = {};
	Result.Type = Type;
	Result.Basis = CanonicalBasis();
	Result.Tint = Tint;

	return Result;
}

struct projectile
{
	u32              Damage;
	sprite           Sprite;
	collision_volume Collision;
};

enum weapon_type
{
	WeaponType_SpreadShot,
	WeaponType_EnemySingleShot,
	WeaponType_FloaterSingleShot,
};

struct weapon
{
	weapon_type Type;
	f32         tCooldown;
	f32         Cooldown;
	f32         LaunchSpeed;
	v2          MuzzleDir;
};

struct animated_object
{
	v2     Pos;
	f32    RotSpeed;
	sprite Sprite;
};

struct camera
{
	world_position P;
	v2             Bounds;
	f32            Speed;
	v2             Velocity;
};

struct sound
{
	u32     SampleCount;
	s16 *   Samples;
		    
	u32     PlayCursor;
	f32     StartDelay;
	f32     Volume;
		    
	s32     Loop;	// NOTE: use negative value for infinite loop

	b32     Valid;
	sound * NextFree;
	sound * Prev;
	sound * Next;
};

struct audio_file
{
	b32                Valid;
	u8 *               Base;
	u32                AudioEntryCount;
	audio_data_entry * AudioTable;
};

#include "game_entity.h"

struct sim_entity_item
{
	sim_entity *      Entity;
	sim_entity_item * Next;
};

#define MAX_SIM_ENTITY_COUNT 4096
struct sim_region
{
	sim_entity      EntityTable[MAX_SIM_ENTITY_COUNT];	// NOTE: Hash table with internal chaining (linear probing)
	sim_entity_item EntityListSentry;
	mem_arena *     TransMemory;	// NOTE: Is cleared every frame
	u32             DebugLoadedEntityCount;
};

struct weapon_list_item
{
	weapon *Weapon;
	weapon_list_item *Next;
};

#include "game_level.h"

struct debug_point_buffer
{
	u32 Count;
	v2 Points[4];
};

static_assert(SPRITE_COUNT < (SPRITEMAP_DIM_X * SPRITEMAP_DIM_Y * SPRITEMAP_COUNT), "SPRITE_COUNT must be less than total spritemap capacity");

internal inline void PointBufferPushPoint(debug_point_buffer *Buffer, v2 P)
{
	if(Buffer->Count < ArrayCount(Buffer->Points)) {
		v2 *Point = Buffer->Points + Buffer->Count++;
		*Point = P;
	}
}

internal inline void PointBufferClear(debug_point_buffer *Buffer)
{
	Buffer->Count = 0;
}

struct game_state
{
	b32               Initialized;
					  
	texture *         TestTextures[6];
	texture *         TiledTestTexture;
					  	  
	level             Level;
	sim_region        SimRegion;
	entity_id         PlayerRef;
	u32               PlayerHealth;
	v2                CameraBounds;
					  
	u32               LoadedLevelBlocksCount;
	u32               LoadedLevelBlocks[3];
	
	weapon            *FirstFreeWeapon;

	rand_state        Entropy;
					  
	mem_arena         MemoryArena;
	mem_arena         TransientArena;
	render_state      Renderer;
	
	audio_file        Music;
	sound *           CurMusic;
	u32               SoundCount;
	sound *           FirstFreeSound;
	sound             SoundListSentry;

	particle_cache *  ParticleCache;

	u32               DebugPlayerBlock;
	u32               DebugHitCount;
	parametric_spline DebugSpline;

	debug_point_buffer DebugPointBuffer;

	stack			  Stack;
};

#endif