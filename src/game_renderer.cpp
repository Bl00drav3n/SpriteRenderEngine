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
				UpdateSpritemapSprite(&Spritemaps, OffsetX, OffsetY, z, pixels);
            }
        }
    }

    return Spritemaps;
}

texture * AllocateTexture(render_state *State) {
	texture *Result = 0;
	if(State->FirstFreeTexture) {
		Result = State->FirstFreeTexture;
		State->FirstFreeTexture = Result->NextFree;
	}
	else {
		Result = push_type(&State->Memory, texture);
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