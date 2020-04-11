#ifndef __GAME_RNG_H__
#define __GAME_RNG_H__

struct rand_lcg_state
{
	u32 Last;
};

typedef rand_lcg_state rand_state;

internal inline rand_lcg_state CreateRNG(u32 Seed)
{
	rand_lcg_state Result = {};
	Result.Last = Seed;

	return Result;
}

internal inline u32 GetUniformRandomU32(rand_state *State)
{
	u32 Result = (1103515245 * State->Last + 12345) & 0x7fffffff;
	State->Last = Result;

	return Result;
}

internal inline f32 GetUniformRandom01(rand_state *State)
{
	f32 Result = GetUniformRandomU32(State) / (float)(2147483648);

	return Result;
}

#endif