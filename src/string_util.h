#define CHAR_NEWLINE '\n'
global_persist char Whitespace[] = {
	' ', '\t', '\v', '\f', '\r', '\n'
};

internal inline b32 IsWhitespace(char C)
{
	for(u32 i = 0; i < ArrayCount(Whitespace); i++) {
		if(Whitespace[i] == C) return true;
	}

	return false;
}

internal inline b32 StringsAreEqual(char *A, char *B)
{
	b32 Result = true;
	while(*A && *B) {
		if(*A++ != *B++) {
			Result = false;
			break;
		}
	}

	if(Result) {
		Result = (*A == *B) && !*A;
	}

	return Result;
}

internal inline b32 StringsAreEqual(char *A, u32 LenA, char *B)
{
	b32 Result = true;
	u32 i;
	for(i = 0; i < LenA && *B; i++) {
		if(*A++ != *B++) {
			Result = false;
			break;
		}
	}

	if(Result) {
		Result = (i == LenA) && !*B;
	}

	return Result;
}

internal inline b32 StringStartsWith(char *Pattern, u32 PatternLen, char *Str)
{
	b32 Result = true;
	char *PatternEnd = Pattern + PatternLen;
	for(u32 i = 0; i < PatternLen && *Str; i++) {
		if(*Pattern++ != *Str++) {
			Result = false;
			break;
		}
	}

	Result = Result && (Pattern == PatternEnd);

	return Result;
}

internal inline u32 GetStringLength(char *Str)
{
	u32 Result = 0;
	if(Str) {
		char *At;
		for(At = Str; *At; At++);
		Result = (u32)(At - Str);
	}
	return Result;
}

struct split_str_result
{
	char *Lhs;
	u32 LhsLen;
	char *Rhs;
	u32 RhsLen;
};

internal inline split_str_result SplitOnFirstToken(char *Str, u32 Len, char Token)
{
	split_str_result Result = {};

	char *Found = 0;
	char *End = Str + Len;
	for(char *Ptr = Str; Ptr < End; Ptr++) {
		if(Ptr[0] == Token) {
			Found = Ptr;
			break;
		}
	}

	if(Found) {
		Result.Lhs = Str;
		Result.LhsLen = (u32)(Found - Str);
		if(Found + 1 < End) {
			Result.Rhs = ++Found;
			Result.RhsLen = (u32)(End - Found);
		}
	}
	else {
		Result.Lhs = Str;
		Result.LhsLen = Len;
	}

	return Result;
}

internal inline char * EatAllWhitespace(char *Str)
{
	char *Result = Str;
	while(*Result) {
		if(!IsWhitespace(*Result))
			break;
		Result++;
	}
	return Result;
}

internal inline char * AdvanceToNextWord(char *Str, u32 *StrLen)
{
	char *Result = 0;
	split_str_result SplitResult = SplitOnFirstToken(Str, *StrLen, ' ');
	if(SplitResult.Rhs) {
		Assert(SplitResult.RhsLen > 0);
		Result = SplitResult.Rhs;
		*StrLen = SplitResult.RhsLen;
	}

	return Result;
}

struct string_buffer
{
	char *Data;
	char *End;

	char *At;
};

internal inline string_buffer CreateStringBuffer(void *Buffer, u32 Size)
{
	string_buffer Result = {};
	Result.Data = (char*)Buffer;
	Result.End = (char*)Buffer + Size;
	Result.At = (char*)Buffer;

	return Result;
}

internal inline b32 CatString(string_buffer *Buffer, char *Str)
{
	b32 Result = true;
	while(*Str) {
		if(Buffer->At < Buffer->End) {
			*Buffer->At++ = *Str++;
		}
		else {
			Result = false;
			break;
		}
	}

	return Result;
}

internal inline char * NullTerminateString(string_buffer *Buffer)
{
	char *Result = 0;
	if(Buffer->At < Buffer->End) {
		*Buffer->At++ = 0;
		Result = Buffer->Data;
	}

	return Result;
}

struct str_t
{
	u32 Len;
	char *Data;
};

internal inline u32 StrLen(char *Str)
{
	u32 Result = 0;
	for(char *At = Str; At[0]; At++, Result++);
	return Result;
}

internal inline str_t StrReadTextLine(char **Str, u32 Len)
{
	str_t Result = {};
	char *At = *Str;
	while(At < *Str + Len && At[0] != CHAR_NEWLINE) {
		At++;
	}
	Result.Data = *Str;
	Result.Len = (u32)(At - *Str);
	*Str = At + 1;

	return Result;
}

// NOTE: Find string length by null-terminator
internal inline str_t StrReadTextLine(char **Str)
{
	str_t Result = StrReadTextLine(Str, StrLen(*Str));
	return Result;
}