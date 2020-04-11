#ifndef __GAME_TEXT_H__
#define __GAME_TEXT_H__

enum text_cmd_type
{
	TextCmd_String,
	TextCmd_U32,
	TextCmd_S32,
	TextCmd_F32,
	TextCmd_Color,
	TextCmd_Newline,
};

// TODO: Enhance (float, vectors, etc.)
// TODO: Replace with variably sized struct?
struct text_cmd
{
	text_cmd_type Type;

	union {
		char *String;	// TODO: Should characters be copied to some local buffer?
		u32 U32;
		s32 S32;
		f32 F32;
		v4 Color;
	};
};

// NOTE: Open a new context with BeginFormattedText and issue text_fmt_push* calls
// to build a text buffer that can be passed to DrawFormattedText. Call EndFormattedText when
// you are finished submitting push calls.
struct formatted_text
{
	u32 CommandCount;
	u32 MaxCommandCount;
	text_cmd *Commands;
};

#endif