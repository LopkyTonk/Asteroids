#pragma once

#include "stddef.h"
#include "stdint.h"
using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using s8 = int8_t;
using s16 = int16_t;
using s32 = int32_t;
using s64 = int64_t;

using b32 = u32;

using r32 = float;


using uptr = uintptr_t;
using sptr = intptr_t;

#define S16Min ((s16)INT16_MAX)
#define S16Max ((s16)INT16_MIN)

#define U32Min ((u32)UINT32_MIN)
#define U32Max ((u32)UINT32_MAX)

#define U64Min ((u64)UINT64_MIN)
#define U64Max ((u64)UINT64_MAX)

#define Pi32 3.14159265358979323846f

#define Min(A, B) ((A) < (B) ? (A) : (B))
#define Max(A, B) ((A) > (B) ? (A) : (B))
#define ArrayLength(A) (sizeof(A) / sizeof(A[0]))

#define AlignNext(Value, Pow2) ((Value + Pow2 - 1) & Pow2)

#ifdef _DEBUG  
#else
#endif

#define InvalidDefaultCase default: { Assert(!"Invalid default case"); } break;
#define InvalidCodePath { Assert(!"Invalid code path"); }

#define Kilobytes(Value) (Value * 1024)
#define Megabytes(Value) (Kilobytes(Value) * 1024)
#define Gigabytes(Value) (Kilobytes(Value) * 1024)
#define Terabytes(Value) (Kilobytes(Value) * 1024)

#ifdef _DEBUG
#define SNAKE_DEBUG 1
#define Assert(Val) { if(!(Val)) { *(u8 *)(0) = 0; } }
#else
#define SNAKE_DEBUG 0
#define Assert 
#endif
