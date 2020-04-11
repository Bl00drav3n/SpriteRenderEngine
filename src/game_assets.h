#ifndef __GAME_ASSETS_H__
#define __GAME_ASSETS_H__

// TODO: DO

enum asset_type
{
	AssetType_Invalid,

	AssetType_Bitmap,
	AssetType_Audio,
};

struct bitmap
{
	u32 Width;
	u32 Height;
	u8 *Pixels;
};

struct audio
{
	u16 NumSamples;
	u16 NumChannels;
	u8 *Samples;
};

struct asset_state
{
	u8 *Data;
};

#endif