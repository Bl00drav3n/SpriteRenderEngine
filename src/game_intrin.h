#ifndef __GAME_INTRIN_H__
#define __GAME_INTRIN_H__

#define U8_MAX  ((u8)(255))
#define U8_MIN  ((u8)(0))
#define U16_MAX ((u16)(65535))
#define U16_MIN ((u16)(0))
#define U32_MAX ((u32)(4294967295))
#define U32_MIN ((u32)(0))

// TODO: Figure out why literals are so confusing when casting to float
#define S8_MAX  ((s8)(127))
#define S8_MIN  ((s8)(-S8_MAX - 1))
#define S16_MAX ((s16)(32767))
#define S16_MIN ((s16)(-S16_MAX - 1))
#define S32_MAX ((s32)(2147483647))
#define S32_MIN ((s32)(-S32_MAX - 1))

#include <intrin.h>

internal inline u32 GetThreadID() {
	u32 ThreadID;

#ifdef _WIN64
	u8 *ThreadLocalStorage = (u8*)__readgsqword(0x30);
	ThreadID = *(u32*)(ThreadLocalStorage + 0x48);
#else
	u8 *ThreadLocalStorage = (u8*)__readfsdword(0x18);
	ThreadID = *(u32*)(ThreadLocalStorage + 0x24);
#endif

	return ThreadID;
}

internal inline u32 SafeTruncateU64ToU32(u64 Value)
{
	Assert(Value < 0x100000000L);
	u32 Result = (u32)Value;
	return Result;
}

inline u64 AtomicAddU64(volatile u64 *Addend, u64 Value)
{
#ifdef _WIN64
	u64 Result = _InterlockedExchangeAdd64((volatile s64*)Addend, Value);
#else
	u64 Result;
	long *Src = (long*)Addend;
	long *ValuePtr = (long*)&Value;
	long *ResultPtr = (long*)&Result;
	ResultPtr[0] = _InterlockedExchangeAdd(Src + 0, ValuePtr[0]);
	ResultPtr[1] = _InterlockedExchangeAdd(Src + 1, ValuePtr[1]);
#endif
	return Result;
}

struct ticket_mutex
{
	volatile u64 Ticket;
	volatile u64 Serving;
};

void BeginTicketMutex(ticket_mutex *Mutex)
{
	u64 Ticket = AtomicAddU64(&Mutex->Ticket, 1);
	while(Ticket != Mutex->Serving);
}

void EndTicketMutex(ticket_mutex *Mutex)
{
	AtomicAddU64(&Mutex->Serving, 1);
}

#endif