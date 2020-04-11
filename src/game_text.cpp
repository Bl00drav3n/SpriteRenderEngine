struct text_extent
{
	v2 UpperLeft;
	v2 LowerRight;
};
internal text_extent GetTextExtent(font *Font, char *Text)
{
	f32 Width = 0.f, Height = 0.f;
	u8 PrevCodepoint = 0;
	f32 TextAt = 0.f;
	for(char *At = Text; *At; At++) {
		u8 Codepoint = *At;
		if(Codepoint >= ASCII_GLYPH_COUNT) {
			Codepoint = '_';
		}
		glyph *Glyph = Font->Data->Glyphs + Codepoint;
		TextAt += Glyph->AdvanceWidth + FontGetKerning(Font->Data, Codepoint, PrevCodepoint);
		PrevCodepoint = Codepoint;

		if(At[0] == '\n' || At[1] == 0) {
			if(TextAt > Width) {
				Width = TextAt;
			}
			Height -= FontGetLineAdvance(Font->Data);
			TextAt = 0.f;
		}
	}

	text_extent Result = { V2(0.f, Font->Data->Ascent), V2(Width, Height + Font->Data->Ascent) };
	return Result;
}

internal v2 PushText(render_group *Group, font *Font, char *Text, v2 P, v4 Color, u8 *LastCodepointInOut = 0)
{
    TIMED_FUNCTION();
	u8 PrevCodepoint = 0;
	if(LastCodepointInOut) {
		PrevCodepoint = *LastCodepointInOut;
	}

	char *At = Text;
	v2 TextAt = P;
	for(char *At = Text; *At; At++) {
		u8 Codepoint = *At;
		if(Codepoint >= ASCII_GLYPH_COUNT) {
			Codepoint = '_';
		}
		if(Codepoint == '\n') {
			TextAt = V2(P.x, TextAt.y - FontGetLineAdvance(Font->Data));
		}

		TextAt.x += FontGetKerning(Font->Data, Codepoint, PrevCodepoint);
		PushGlyph(Group, Font, TextAt, Codepoint);
		
		glyph *Glyph = Font->Data->Glyphs + Codepoint;
		TextAt.x += Glyph->AdvanceWidth;
		PrevCodepoint = Codepoint;
	}

	if(LastCodepointInOut) {
		*LastCodepointInOut = PrevCodepoint;
	}

	return TextAt;
}

internal v2 PushTextU32(render_group *Group, font *Font, u32 Value, v2 P, v4 Color, u8 *LastCodepointInOut)
{
	char Buffer[16];
	Buffer[15] = 0;
	char *At = Buffer + sizeof(Buffer) - 2;
	do {
		Assert(At >= Buffer);
		*At-- = (Value % 10) + '0';
		Value /= 10;
	} while(Value);

	return PushText(Group, Font, At + 1, P, Color, LastCodepointInOut);
}

global_persist formatted_text * g_TextContext = 0;

internal void BeginFormattedText(formatted_text *Context, void *Buffer, u32 BufferSize)
{
	if(Context) {
		g_TextContext = Context;
		Context->CommandCount = 0;
		Context->Commands = (text_cmd*)Buffer;
		Context->MaxCommandCount = BufferSize / sizeof(text_cmd);
	}
}

internal void EndFormattedText()
{
	g_TextContext = 0;
}

internal text_cmd * text_fmt_alloc_cmd(formatted_text *Context)
{
	text_cmd *Result = 0;
	if(Context) {
		if(Context->CommandCount < Context->MaxCommandCount) {
			Result = Context->Commands + Context->CommandCount++;
		}
	}

	return Result;
}

internal void text_fmt_push_string(char *String)
{
	text_cmd *Cmd = text_fmt_alloc_cmd(g_TextContext);
	if(Cmd) {
		Cmd->Type = TextCmd_String;
		Cmd->String = String;
	}
}

internal void text_fmt_push_u32(u32 Value)
{
	text_cmd *Cmd = text_fmt_alloc_cmd(g_TextContext);
	if(Cmd) {
		Cmd->Type = TextCmd_U32;
		Cmd->U32 = Value;
	}
}

internal void text_fmt_push_s32(s32 Value)
{
	text_cmd *Cmd = text_fmt_alloc_cmd(g_TextContext);
	if(Cmd) {
		Cmd->Type = TextCmd_S32;
		Cmd->S32 = Value;
	}
}

internal void text_fmt_push_f32(f32 Value)
{	
	text_cmd *Cmd = text_fmt_alloc_cmd(g_TextContext);
	if(Cmd) {
		Cmd->Type = TextCmd_F32;
		Cmd->F32 = Value;
	}
}

internal void text_fmt_push_decimal_seperator()
{
	text_fmt_push_string(".");
}

// NOTE: text_fmt_push_u64_milli(12345678) => 12345.678
internal void text_fmt_push_u64_milli(u64 Value)
{
	u64 N = Value / 1000;
	text_fmt_push_u32((u32)N);
	text_fmt_push_decimal_seperator();
	text_fmt_push_u32((u32)(Value - N * 1000));
}

internal void text_fmt_push_color(v4 Color)
{
	text_cmd *Cmd = text_fmt_alloc_cmd(g_TextContext);
	if(Cmd) {
		Cmd->Type = TextCmd_Color;
		Cmd->Color = Color;
	}
}

internal void text_fmt_push_newline()
{
	text_cmd *Cmd = text_fmt_alloc_cmd(g_TextContext);
	if(Cmd) {
		Cmd->Type = TextCmd_Newline;
	}
}

#define ADDRESSOF(v)   (&reinterpret_cast<const char &>(v))
#define INTSIZEOF(n)   ((sizeof(n) + sizeof(int) - 1) & ~(sizeof(int) - 1))

typedef u8 * vararg_list;
#define vararg_start(ap,v)  ( ap = (vararg_list)ADDRESSOF(v) + INTSIZEOF(v) )
#define vararg_arg(ap,t)    ( *(t*)((ap += INTSIZEOF(t)) - INTSIZEOF(t)) )
#define vararg_end(ap)      ( ap = (vararg_list)0 )

internal inline void text_put_char_safe(char C, char **Dst, char *DstEnd)
{
	if(*Dst < DstEnd) {
		*(*Dst)++ = C;
	}
}

internal void text_fmt_vprintf(char *Buffer, u32 BufferSize, char *Format, vararg_list Args)
{
	Assert(BufferSize > 0);

	char *End = Buffer + BufferSize - 1;
	*End = 0;

	char *Out = Buffer;
	for(char *At = Format; *At && Out < End; At++) {
		if(*At != '%') {
			*Out++ = *At;
		}
		else {
			At++;
			switch(*At) {
				case 's': {
					// NOTE: String copy
					char * Str = vararg_arg(Args, char*);
					while(*Str && Out < End) {
						*Out++ = *Str++;
					}
				} break;
				case 'c': {
					// NOTE: 8bit ascii character
					u8 Char = vararg_arg(Args, u8);
					*Out++ = Char;
			    } break;
				case 'd': {
					// NOTE: s32
					s32 Value = vararg_arg(Args, s32);
					if(Value < 0) {
						text_put_char_safe('-', &Out, End);
						Value = -Value;
					}
					char Digits[16];
					char *DigitAt = Digits - 1;
					do {
						*++DigitAt = '0' + (Value % 10);
						Value /= 10;
					} while(Value);
					for(; DigitAt >= Digits; DigitAt--) {
						text_put_char_safe(*DigitAt, &Out, End);
					}
				} break;
				case 'u': {
					// NOTE: u32
					u32 Value = vararg_arg(Args, u32);
					char Digits[16];
					char *DigitAt = Digits - 1;
					do {
						*++DigitAt = '0' + (Value % 10);
						Value /= 10;
					} while(Value);
					for(; DigitAt >= Digits; DigitAt--) {
						text_put_char_safe(*DigitAt, &Out, End);
					}
				} break;
				case 'f': {
					// NOTE: 32bit fixed comma
					NotImplemented;
				} break;
				case '%':
					// NOTE: % character
					*Out++ = '%';
					break;
				
				InvalidDefaultCase;
			}
		}
	}

	*Out = 0;
}

// TODO: Get rid of c standard lib
#include <stdio.h>
#include <stdarg.h>
#pragma warning(disable : 4996)
internal void text_fmt_sprintf(char *Buffer, u32 BufferSize, char *Format, ...)
{
#if 1
	va_list Args;
	va_start(Args, Format);
	vsnprintf(Buffer, BufferSize, Format, (va_list)Args);
	va_end(Args);
#else
	vararg_list Args;
	vararg_start(Args, Format);
	text_fmt_vprintf(Buffer, BufferSize, Format, Args);
	vararg_end(Args);
#endif
}

// TODO: Instead of immediate mode rendering, we should process the command buffer to generate a string
// and only render that string. This way we can easily implement GetTextExtend etc. (usefull for drawing text backdrops)
internal v2 PushFormattedText(render_group *Group, font *Font, formatted_text *Text, v2 Pos)
{
	u8 PrevCodepoint = 0;
	v2 TextAt = Pos;
	v4 Color = V4(1.f, 1.f, 1.f, 1.f);
	for(u32 i = 0; i < Text->CommandCount; i++) {
		text_cmd *Cmd = Text->Commands + i;
		switch(Cmd->Type) {
			case TextCmd_Color:
				Color = Cmd->Color;
				break;
			case TextCmd_U32:
			{
				u32 Value = Cmd->U32;

				char Buffer[16];
				Buffer[15] = 0;
				char *At = Buffer + sizeof(Buffer) - 2;
				do {
					Assert(At >= Buffer);
					*At-- = (Value % 10) + '0';
					Value /= 10;
				} while(Value);
				TextAt = PushText(Group, Font, At + 1, TextAt, Color, &PrevCodepoint);
			} break;
			case TextCmd_S32:
			{
				s32 Sign;
				s32 Value = Cmd->S32;
				if(Value >= 0) {
					Sign = 1;
				}
				else {
					Sign = -1;
					Value = -Value;
				}

				char Buffer[16];
				Buffer[15] = 0;
				char *At = Buffer + sizeof(Buffer) - 2;
				do {
					Assert(At >= Buffer);
					*At-- = (Value % 10) + '0';
					Value /= 10;
				} while(Value);
				if(Sign == -1) {
					*At-- = '-';
				}
				TextAt = PushText(Group, Font, At + 1, TextAt, Color, &PrevCodepoint);
			} break;
			case TextCmd_F32:
			{
				// TODO: Implement!
				f32 Value = Cmd->F32;
				NotImplemented;
			} break;
			case TextCmd_String:
				TextAt = PushText(Group, Font, Cmd->String, TextAt, Color, &PrevCodepoint);
				break;
			case TextCmd_Newline:
				TextAt.x = Pos.x;
				TextAt.y -= FontGetLineAdvance(Font->Data);
				break;
			
			InvalidDefaultCase;
		}
	}

	return TextAt;
}