void ReloadOpenGL()
{
	// NOTE: Some tiny meta-preprocessing so we only need to add things once
#define GL_MACRO(name, ...) name = (_##name*)Platform->LoadProcAddressGL(#name)
#include "game_opengl.h"
}

internal void AllocateRenderGroups(render_state *Renderer)
{
    Renderer->GameRenderGroup = AllocateRenderGroup(Renderer->PerFrameMemory, MEGABYTES(32));
}

internal spritemap_array AllocateSpritemaps()
{
    spritemap_array Result = {};

    GLuint TexId;
    glGenTextures(1, &TexId);
    if (TexId) {
        u32 TextureWidth = SPRITEMAP_DIM_X * SPRITE_WIDTH;
        u32 TextureHeight = SPRITEMAP_DIM_Y * SPRITE_HEIGHT;

        glBindTexture(GL_TEXTURE_2D_ARRAY, TexId);
        glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, TextureWidth, TextureHeight, SPRITEMAP_COUNT, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);  
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

        Result.TexId = TexId;
        Result.TextureWidth = TextureWidth;
        Result.TextureHeight = TextureHeight;
    }

    return Result;
}

void RandomSpriteColor(rand_lcg_state *Entropy, sprite_pixel *p)
{
    p->r = MapF32ToU8(GetUniformRandom01(Entropy));
    p->g = MapF32ToU8(GetUniformRandom01(Entropy));
    p->b = MapF32ToU8(GetUniformRandom01(Entropy));
    p->a = 0xff;
}

//#include <stdlib.h>
spritemap_array AllocateTestSpritemaps()
{
    spritemap_array Spritemaps = AllocateSpritemaps();
    rand_lcg_state Entropy = CreateRNG(0x38a923f4);
    glBindTexture(GL_TEXTURE_2D_ARRAY, Spritemaps.TexId);
    for (u32 z = 0; z < SPRITEMAP_COUNT; z++) {
#if 0
        const u32 COUNT_X = SPRITEMAP_DIM_X * SPRITE_WIDTH;
        const u32 COUNT_Y = SPRITEMAP_DIM_Y * SPRITE_HEIGHT;
        sprite_pixel *cleared = (sprite_pixel*)malloc(COUNT_X * COUNT_Y * sizeof(*cleared));
        for (u32 i = 0; i < COUNT_X * COUNT_Y; i++) {
            cleared[i].a = 0xff;
            cleared[i].r = 0xff;
            cleared[i].g = 0x00;
            cleared[i].b = 0xff;
        }
        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, z, COUNT_X, COUNT_Y, 1, GL_RGBA, GL_UNSIGNED_BYTE, cleared);
#endif
        for (u32 y = 0; y < SPRITEMAP_DIM_Y; y++) {
            for (u32 x = 0; x < SPRITEMAP_DIM_X; x++) {
                sprite_pixel pixels[SPRITE_WIDTH * SPRITE_HEIGHT];
                sprite_pixel pixel;
                RandomSpriteColor(&Entropy, &pixel);
                for (u32 i = 0; i < ArrayCount(pixels); i++) {
                    sprite_pixel *p = pixels + i;
                    *p = pixel;
                }
                u32 OffsetX = x * SPRITE_WIDTH;
                u32 OffsetY = y * SPRITE_HEIGHT;
                glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, OffsetX, OffsetY, z, SPRITE_WIDTH, SPRITE_HEIGHT, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
            }
        }
    }
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

    return Spritemaps;
}

internal texture * CreateTexture(render_state *State, u32 Width, u32 Height, u8 *Data)
{
	texture * Result = 0;

	if(State->FirstFreeTexture) {
		Result = State->FirstFreeTexture;
		State->FirstFreeTexture = Result->NextFree;
	}
	else {
		Result = push_type(&State->Memory, texture);
		glGenTextures(1, &Result->Handle);
	}

	if(Result) 
	{
		glBindTexture(GL_TEXTURE_2D, Result->Handle);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, Width, Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, Data);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		
		Result->Width = Width;
		Result->Height = Height;
		Result->HasBeenFreed = false;

		// TODO: Store more information about the texture? (size, bpp, type, ...)
		State->TextureMemoryUsed += 4 * Width * Height;
	}

	return Result;
}

internal texture * LoadTextureFromFile(render_state *State, char *Filename)
{
	texture *Result = 0;
	file_info File = Platform->OpenFile(Filename, FileType_Image);
	if(File.Valid) {
		void *FileData = PushSize(State->PerFrameMemory, File.Size);
		if(Platform->ReadEntireFile(&File, FileData)) {
			int Width, Height, Comp;
			stbi_set_flip_vertically_on_load(true);
			u8 * ImageData = stbi_load_from_memory((u8*)FileData, File.Size, &Width, &Height, &Comp, 4);
			if(ImageData) {
				Result = CreateTexture(State, (u32)Width, (u32)Height, ImageData);
			}
			else {
				DebugConsolePushString((char*)stbi_failure_reason(), MSG_ERROR);
			}
		}
	}
	Platform->CloseFile(&File);

	return Result;
}

internal void FreeTexture(render_state *State, texture *Texture)
{
	Assert(!Texture->HasBeenFreed);

	Texture->NextFree = State->FirstFreeTexture;
	Texture->HasBeenFreed = true;
	State->FirstFreeTexture = Texture;
	State->TextureMemoryUsed -= 4 * Texture->Width * Texture->Height;
}

internal texture * CreateDebugTexture(render_state *State, v4 Color)
{
	u8 R, G, B, A;
	const u32 ImageWidth = 32;
	const u32 ImageHeight = 32;
	u8 ImageData[4 * ImageWidth * ImageHeight];

	R = MapF32ToU8(Color.r * Color.a);
	G = MapF32ToU8(Color.g * Color.a);
	B = MapF32ToU8(Color.b * Color.a);
	A = MapF32ToU8(Color.a * Color.a);

	u8 *ImageDataP = ImageData;
	for(u32 y = 0; y < ImageHeight; y++) {
		for(u32 x = 0; x < ImageWidth; x++) {
			ImageDataP[0] = R;
			ImageDataP[1] = G;
			ImageDataP[2] = B;
			ImageDataP[3] = A;
			ImageDataP += 4;
		}
	}
	
	return CreateTexture(State, ImageWidth, ImageHeight, ImageData);
}

texture * CreateTiledDebugTexture(render_state *State)
{
	u8 R, G, B, A;
	const u32 ImageWidth = 32;
	const u32 ImageHeight = 32;
	const u32 ImagePitch = 4 * ImageWidth;
	u8 ImageData[ImagePitch * ImageHeight];

	v4 Colors[4] = {
		V4(1.f, 0.f, 0.f, 1.f),
		V4(0.f, 1.f, 0.f, 1.f),
		V4(0.f, 0.f, 1.f, 1.f),
		V4(1.f, 1.f, 1.f, 1.f),
	};
	
	u32 TileWidth = ImageWidth / 2;
	u32 TileHeight = ImageHeight / 2;
	v4 *TileColor = Colors;
	for(u32 TileY = 0; TileY < 2; TileY++) {
		for(u32 TileX = 0; TileX < 2; TileX++) {
			R = MapF32ToU8(TileColor->r * TileColor->a);
			G = MapF32ToU8(TileColor->g * TileColor->a);
			B = MapF32ToU8(TileColor->b * TileColor->a);
			A = MapF32ToU8(TileColor->a * TileColor->a);

			u8 *ImageDataP = ImageData + 4 * TileY * ImageWidth * TileHeight + 4 * TileX * TileWidth;
			for(u32 y = 0; y < TileHeight; y++) {
				for(u32 x = 0; x < TileWidth; x++) {
					ImageDataP[0] = R;
					ImageDataP[1] = G;
					ImageDataP[2] = B;
					ImageDataP[3] = A;
					ImageDataP += 4;
				}
				ImageDataP += 4 * TileWidth;
			}

			TileColor++;
		}
	}

	return CreateTexture(State, ImageWidth, ImageHeight, ImageData);
}

sprite CreateSprite(s32 SpritePosX, s32 SpritePosY, s32 SpritemapIndex)
{
    Assert(SpritePosX < SPRITEMAP_DIM_X);
    Assert(SpritePosY < SPRITEMAP_DIM_Y);
    Assert(SpritemapIndex < SPRITEMAP_COUNT);

	sprite Result;
    Result.SpritePosX = SpritePosX;
    Result.SpritePosY = SpritePosY;
    Result.SpritemapIndex = SpritemapIndex;

	return Result;
}

internal GLuint CreateShader(char *src, GLenum Type)
{
	GLuint Shader = glCreateShader(Type);
	glShaderSource(Shader, 1, (const char**)&src, 0);
	glCompileShader(Shader);
	
#if 1
	GLint Status;
	glGetShaderiv(Shader, GL_COMPILE_STATUS, &Status);
	if(!Status) {
		DebugConsolePushString("Shader compilation failed.", MSG_ERROR);
		GLsizei Length;
		char Buffer[1024];
		glGetShaderInfoLog(Shader, sizeof(Buffer), &Length, Buffer);
		if(Length) {
			DebugConsolePushString(Buffer);
		}
	}
#endif

	return Shader;
}

internal void BindGlobalVertexAttributes(GLuint Program) {
    Assert(ArrayCount(GlobalVertexAttribs) == VertexAttrib_Count);
    for (u32 i = 0; i < ArrayCount(GlobalVertexAttribs); i++) {
        vertex_attrib *Attrib = GlobalVertexAttribs + i;
        glBindAttribLocation(Program, Attrib->Id, Attrib->Name);
    }
}

internal GLuint CreateShaderProgram(char *SrcVS, char *SrcGS, char *SrcFS)
{
    GLuint Program = glCreateProgram();

    char *Sources[3] = { SrcVS, SrcGS, SrcFS };
    GLenum Types[3] = { GL_VERTEX_SHADER, GL_GEOMETRY_SHADER, GL_FRAGMENT_SHADER };
    for (u32 i = 0; i < 3; i++) {
        char *Src = Sources[i];
        GLenum Type = Types[i];
        if (Src) {
            GLuint Shader = CreateShader(Src, Type);
            glAttachShader(Program, Shader);
        }
    }

    BindGlobalVertexAttributes(Program);

    glBindFragDataLocation(Program, 0, "FinalColor");
    
	glLinkProgram(Program);

	GLint Status;
	glGetProgramiv(Program, GL_LINK_STATUS, &Status);
	if(!Status) {
		DebugConsolePushString("Could not create shader program.", MSG_ERROR);

		GLsizei Length;
		char Buffer[1024];
		glGetProgramInfoLog(Program, sizeof(Buffer), &Length, Buffer);
		if(Length) {
			DebugConsolePushString(Buffer);
		}
	}

    return Program;
}

internal sprite_program CreateSpriteProgram(char *VS, char *GS, char *FS)
{
    sprite_program Result;
    GLuint Program = CreateShaderProgram(VS, GS, FS);
    Result.Id = Program;
    Result.ProjectionTransformLoc = glGetUniformLocation(Program, "ProjectionTransform");
    Result.SpriteHalfDimLoc = glGetUniformLocation(Program, "SpriteHalfDim");
    Result.SpriteTexDimLoc = glGetUniformLocation(Program, "SpriteTexDim");
    Result.SpritemapsLoc = glGetUniformLocation(Program, "Spritemaps");

    return Result;
}

internal inline void BakeParametricSpline(parametric_spline *S)
{
	if(S->Count > 1) {
		build_spline(S->t, S->x, S->x2, S->Count);
		build_spline(S->t, S->y, S->y2, S->Count);
	}
}

internal inline v2 SplineAtTime(parametric_spline *S, f32 t)
{
	v2 Result;
	t = Clamp(t, 0.f, (f32)S->Count);
	cubic_interp(S->t, S->x, S->x2, S->Count, t, &Result.x);
	cubic_interp(S->t, S->y, S->y2, S->Count, t, &Result.y);

	return Result;
}

internal void PushSplinePoint(parametric_spline *S, v2 P)
{
	if(S->Count < MAX_SPLINE_CTRL_PTS) {
		S->t[S->Count] = (f32)S->Count;
		S->x[S->Count] = P.x;
		S->y[S->Count] = P.y;
		S->Count++;
		BakeParametricSpline(S);
	}
}

internal void PopSplinePoint(parametric_spline *S)
{
	if(S->Count) {
		S->Count--;
		if(S->Count > 0) {
			BakeParametricSpline(S);
		}
	}
}