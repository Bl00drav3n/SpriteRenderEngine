#ifndef __FONT_H__
#define __FONT_H__

#define MAX_GLYPH_PIXELS_X 32
#define MAX_GLYPH_PIXELS_Y 32
struct glyph
{
	int Width;
	int Height;
	int OffsetX;
	int OffsetY;
	unsigned char Bitmap[4 * MAX_GLYPH_PIXELS_X * MAX_GLYPH_PIXELS_Y];

	float AdvanceWidth;
	float LeftSideBearing;
};

#define ASCII_GLYPH_COUNT 128
struct ascii_font
{
	float PixelSize;
	float Ascent;
	float Descent;
	float LineGap;

	glyph Glyphs[ASCII_GLYPH_COUNT];
	float KerningTable[ASCII_GLYPH_COUNT * ASCII_GLYPH_COUNT];
};

static inline float FontGetLineAdvance(ascii_font *Font)
{
	float Result = Font->Ascent - Font->Descent + Font->LineGap;
	return Result;
}

static inline float FontGetKerning(ascii_font *Font, unsigned char Codepoint, unsigned char PreviousCodepoint)
{
	float Result = Font->KerningTable[PreviousCodepoint * ASCII_GLYPH_COUNT + Codepoint];
	return Result;
}

#endif