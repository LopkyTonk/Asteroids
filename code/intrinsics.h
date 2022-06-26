#pragma once

u32 
AtomicAddU32(volatile u32 *Dst, u32 Value)
{
  u32 Result = (u32)_InterlockedExchangeAdd(Dst, Value);
  return Result;
}
u32 
AtomicExchangeU32(volatile u32 *Dst, u32 Value)
{
  u32 Result = (u32)_InterlockedExchange(Dst, Value);
  return Result;
}
u64 
AtomicExchangeU64(volatile u64 *Dst, u64 Value)
{
  u64 Result = (u64)_InterlockedExchange64((volatile s64 *)Dst, (s64)Value);
  return Result;
}

#include <math.h>
r32 Round(r32 Val)
{
  r32 Result = roundf(Val);
  return Result;
}
r32 
Ceil(r32 Val)
{
  r32 Result = ceilf(Val);
  return Result;
}
r32 
Floor(r32 Val)
{
  r32 Result = floorf(Val);
  return Result;
}

r32
Abs(r32 Val)
{
  r32 Result= fabsf(Val);
  return Result;
}

uptr
NextPow2(uptr Val)
{
  uptr Result = 1;
  while(Result < Val) Result <<= 1;
  return Result;
}

r32
Sin(r32 Radians)
{
  r32 Result = sinf(Radians);
  return Result;
}
r32
Cos(r32 Radians)
{
  r32 Result = cosf(Radians);
  return Result;
}

r32
SafeDivide0(r32 Numerator, r32 Denominator)
{
  r32 Result = 0;
  if(Denominator != 0.0f)
  {
    Result = Numerator / Denominator;
  }

  return Result;
}

r32 
Square(r32 Val)
{
  r32 Result = Val * Val;
  return Result;
}

r32 
Sqrt(r32 Val)
{
  r32 Result = sqrtf(Val);
  return Result;
}

r32 
Lerp(r32 A, r32 B, r32 T)
{
  r32 Result = A +  (B - A) * T;
  return Result;
}

s8 
Clamp(s8 Val, s8 Min, s8 Max)
{
  s8 Result;
  if(Val < Min)
  {
    Result = Min; 
  }
  else if(Val > Max) 
  { 
    Result = Max; 
  }
  else
  {
    Result = Val;
  }

  return Result;
}

s16 
Clamp(s16 Val, s16 Min, s16 Max)
{
  s16 Result;
  if(Val < Min)
  {
    Result = Min; 
  }
  else if(Val > Max) 
  { 
    Result = Max; 
  }
  else
  {
    Result = Val;
  }

  return Result;
}

s32 
Clamp(s32 Val, s32 Min, s32 Max)
{
  s32 Result;
  if(Val < Min)
  {
    Result = Min; 
  }
  else if(Val > Max) 
  { 
    Result = Max; 
  }
  else
  {
    Result = Val;
  }

  return Result;
}

u32 
Clamp(u32 Val, u32 Min, u32 Max)
{
  u32 Result;
  if(Val < Min)
  {
    Result = Min; 
  }
  else if(Val > Max) 
  { 
    Result = Max; 
  }
  else
  {
    Result = Val;
  }

  return Result;
}

r32 
Clamp(r32 Val, r32 Min, r32 Max)
{
  r32 Result;
  if(Val < Min)
  {
    Result = Min; 
  }
  else if(Val > Max) 
  { 
    Result = Max; 
  }
  else
  {
    Result = Val;
  }

  return Result;
}

struct bit_scan_result
{
   b32 Found;
   u32 Index;
};
bit_scan_result 
LSBU32(u32 Mask)
{
   bit_scan_result Result = {};
   Result.Found = _BitScanForward(reinterpret_cast<unsigned long*>(&Result.Index), Mask);

   return Result;
}
