internal image LoadImageFromFile(mem_arena *Memory, char *Filename)
{
	image Result = {};
	file_info File = Platform->OpenFile(Filename, FileType_Image);
	if(File.Valid) {
		void *FileData = PushSize(Memory, File.Size);
		if(Platform->ReadEntireFile(&File, FileData)) {
			stbi_set_flip_vertically_on_load(true);
			Result.Pixels = stbi_load_from_memory((u8*)FileData, File.Size, &Result.Width, &Result.Height, &Result.Comp, 4);
			if(Result.Pixels && Result.Comp == 4) {
				Result.Valid = true;
			}
			else {
				DebugConsolePushString((char*)stbi_failure_reason(), MSG_ERROR);
			}
		}
	}
	Platform->CloseFile(&File);

	return Result;
}

internal void RandomSpriteColor(rand_lcg_state *Entropy, sprite_pixel *p)
{
    p->r = MapF32ToU8(GetUniformRandom01(Entropy));
    p->g = MapF32ToU8(GetUniformRandom01(Entropy));
    p->b = MapF32ToU8(GetUniformRandom01(Entropy));
    p->a = 0xff;
}

internal void InitializeRenderer(render_state *Renderer)
{
	GfxInitializeRenderBackend(Renderer);

	u8 white[4] = { 0xff, 0xff, 0xff, 0xff };
	Renderer->WhiteTexture = GfxCreateTexture(Renderer, 1, 1, white);
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
	image Image = LoadImageFromFile(State->PerFrameMemory, Filename);
	if(Image.Valid) {
		Result = GfxCreateTexture(State, Image.Width, Image.Height, Image.Pixels);
	}

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
	
	return GfxCreateTexture(State, ImageWidth, ImageHeight, ImageData);
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

	return GfxCreateTexture(State, ImageWidth, ImageHeight, ImageData);
}

void LoadShaderSources(mem_arena *Memory, char *ShaderSourceNames[3], char *ShaderSources[3]) {
	for(u32 i = 0; i < 3; i++) {
		ShaderSources[i] = 0;
		if(ShaderSourceNames[i]) {
			file_info File = Platform->OpenFile(ShaderSourceNames[i], FileType_Shader);
			if(File.Valid) {
				void *Data = PushSize(Memory, File.Size + 1);
				if(Data && Platform->ReadEntireFile(&File, Data)) {
					((char*)Data)[File.Size] = 0;
					ShaderSources[i] = (char*)Data;
				}
			}
			Platform->CloseFile(&File);
		}
	}
}

#include "game_renderer_opengl.cpp"