#ifndef __GAME_RENDERER_H__
#define __GAME_RENDERER_H__

#define MAX_NUM_SPRITE_VERTICES 8192

struct render_group;

struct texture
{
	u32 Handle;

	u32 Width;
	u32 Height;

	b32 HasBeenFreed;
	texture *NextFree;
};

#define GET_LAYER(Id) U32ToLayerId(Id)
struct layer_id
{
	u32 Id;
};

static inline layer_id U32ToLayerId(u32 Id)
{
	layer_id Result;
	Result.Id = Id;

	return Result;
}

struct viewport
{
	s32 X;
	s32 Y;
	u32 Width;
	u32 Height;
};

struct tex_coords
{
	v2 LowerLeft;
	v2 UpperRight;
};

struct sprite
{
    s32 SpritePosX;
    s32 SpritePosY;
    s32 SpritemapIndex;
};

struct sprite_vertex
{
	v2 Position;
	v2 TexCoord;
};

struct rect_vertex
{
	v2 Position;
};

struct spritemap_offset
{
    s32 X;
    s32 Y;
    s32 Z;
};

struct sprite_pixel
{
    u8 r;
    u8 g;
    u8 b;
    u8 a;
};

struct spritemap_vertex
{
    v2 Position;
    spritemap_offset Offset;
    v4 Tint;
};

// NOTE: SPRITMAP_COUNT x SPRITEMAP_DIM_X x SPRITEMAP_DIM_Y sprites of dimension SPRITE_WIDTH x SPRITE_HEIGHT pixels
#define SPRITE_WIDTH 32
#define SPRITE_HEIGHT 32

#define SPRITEMAP_COUNT 4
#define SPRITEMAP_DIM_X 16
#define SPRITEMAP_DIM_Y 16
#define SPRITEMAP_LAYER_WIDTH (SPRITEMAP_DIM_X * SPRITE_WIDTH)
#define SPRITEMAP_LAYER_HEIGHT (SPRITEMAP_DIM_Y * SPRITE_HEIGHT)

struct spritemap_array
{
    u32 TexId;
    u32 TextureWidth;
    u32 TextureHeight;
};

struct sprite_program
{
    u32 Id;
};

struct line_program
{
	u32 Id;
};

struct render_state
{
	mem_arena   Memory;
	mem_arena * PerFrameMemory;

	u32         TextureMemoryUsed;
	texture *   FirstFreeTexture;

    spritemap_array Spritemaps;
    sprite_program SpriteProgram;
	line_program LineProgram;
};

static inline f32 MapU8ToF32(u8 Value)
{
	f32 Result = (f32)Value / 255.f;

	return Result;
}

static inline u8 MapF32ToU8(f32 Value)
{
	u8 Result = (u8)(Value * 255.f + 0.5f);

	return Result;
}

static inline tex_coords DefaultTexCoords()
{
	tex_coords Result;
	Result.LowerLeft = V2(0.f, 0.f);
	Result.UpperRight = V2(1.f, 1.f);

	return Result;
}

static inline tex_coords MakeTexCoords(v2 LowerLeft, v2 UpperRight)
{
	tex_coords Result;
	Result.LowerLeft = LowerLeft;
	Result.UpperRight = UpperRight;

	return Result;
}

static inline tex_coords MakeTexCoords(rect2 Rect)
{
	tex_coords Result;
	Result.LowerLeft = Rect.Center - Rect.HalfDim;
	Result.UpperRight = Rect.Center + Rect.HalfDim;

	return Result;
}

static inline v4 PremultiplyAlpha(v4 ColorIn)
{
    v4 Result;
    Result.r = ColorIn.r * ColorIn.a;
    Result.g = ColorIn.g * ColorIn.a;
    Result.b = ColorIn.b * ColorIn.a;
    Result.a = ColorIn.a;

    return Result;
}

internal void ReloadRenderBackend();
internal texture * CreateTexture(render_state *State, u32 Width, u32 Height, u8 *Data);
internal spritemap_array AllocateSpritemaps();
internal void UpdateSpritemapSprite(spritemap_array *Spritemaps, u32 OffsetX, u32 OffsetY, u32 Index, sprite_pixel *Pixels);
internal sprite_program CreateSpriteProgram(mem_arena *Memory);
internal void DrawRenderGroup(render_state *Renderer, render_group *Group);

#endif