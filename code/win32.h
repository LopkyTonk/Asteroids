#pragma once

#define WIN32_LEAN_AND_MEAN
#include "windows.h"
#include "winsock2.h"
#include "ws2tcpip.h"

void *
Win32Alloc(uptr Size)
{
  void *Result = VirtualAlloc(0, Size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE); 
  return Result;
}

void
Win32Free(void *Free)
{
  VirtualFree(Free, 0, MEM_RELEASE);
}

struct win32_file
{
  HANDLE Handle_;

  void *Memory;
  u64 Size;
};

#define Win32FileIsValid(Win32File)(Win32File.Handle_ != INVALID_HANDLE_VALUE)

win32_file
Win32FileRead(char *Filename)
{
  win32_file Result = {};
  Result.Handle_ = CreateFileA(Filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, 0);

  if(Result.Handle_ != INVALID_HANDLE_VALUE)
  {
    LARGE_INTEGER FileSize;
    GetFileSizeEx(Result.Handle_, &FileSize);

    Result.Size = (u64)FileSize.QuadPart;
    Result.Memory = Win32Alloc(Result.Size);
    if(!Result.Memory)
    {
      CloseHandle(Result.Handle_);
      Result.Handle_ = INVALID_HANDLE_VALUE;
    }
    else
    {
      DWORD BytesRead;
      ReadFile(Result.Handle_, Result.Memory, (DWORD)Result.Size, &BytesRead, 0);

      if((u64)BytesRead != Result.Size)
      {
        CloseHandle(Result.Handle_);
        Win32Free(Result.Memory);

        Result.Handle_ = INVALID_HANDLE_VALUE;
      }
    }
  }

  return Result;
}

void
Win32FileFree(win32_file File)
{
  Assert(File.Handle_ != INVALID_HANDLE_VALUE);
  Assert(File.Memory);
  Assert(File.Size);

  CloseHandle(File.Handle_);
  Win32Free(File.Memory);
}
