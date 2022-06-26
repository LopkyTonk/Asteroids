#include "mmdeviceapi.h"
#include "mmreg.h"
#include "Audioclient.h"

struct win32_audio_buffer
{
  s16 *Samples;
  u32 FrameIndex;
};
struct win32_audio
{
  IMMDevice* Device;
  IAudioClient2* Client;
  IAudioRenderClient* RenderClient;
  HANDLE BufferMutex;
  

  u32 SamplesPerSecond;
  u32 ChannelCount;
  u32 BytesPerChannel;
  u32 FrameSize;
  u32 BytesPerSecond;

  u32 FrameCount;
  u32 LatencyFrameCount;

  u32 BufferFrameCount;
  win32_audio_buffer Buffers[2];

  u32 CurrentBufferIndex_;
  u32 FramesPlayedCount_;
};

b32
Win32AudioInitialize(win32_audio *Audio)
{
  if(CoInitialize(0) != S_OK)
  {
    return false;
  }

  IMMDeviceEnumerator *Enumerator;
  if(CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void **)(&Enumerator)) != S_OK)
  {
    return false;
  }
  IMMDeviceCollection *Collection;
  if(Enumerator->EnumAudioEndpoints(EDataFlow::eRender, DEVICE_STATE_ACTIVE, &Collection) != S_OK)
  {
    return false;
  }
 
  Enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &Audio->Device);
  if(Audio->Device->Activate(__uuidof(IAudioClient2), CLSCTX_ALL, 0, (void **)(&Audio->Client)) != S_OK)
  {
    return false;
  }

  WAVEFORMATEX CheckFormat = {};
  CheckFormat.wFormatTag = WAVE_FORMAT_PCM;
  CheckFormat.nSamplesPerSec = 48'000;
  CheckFormat.nChannels = 2;
  CheckFormat.wBitsPerSample = 16;
  CheckFormat.nBlockAlign = CheckFormat.nChannels * CheckFormat.wBitsPerSample / 8;
  CheckFormat.nAvgBytesPerSec = CheckFormat.nSamplesPerSec * CheckFormat.nBlockAlign;
  CheckFormat.cbSize = 0;

  WAVEFORMATEX *ClosestMatch;
  if(Audio->Client->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED, &CheckFormat, &ClosestMatch) != S_OK)
  {
    return false;
  }
  CoTaskMemFree(ClosestMatch);

  Audio->SamplesPerSecond = CheckFormat.nSamplesPerSec;
  Audio->ChannelCount = CheckFormat.nChannels;
  Audio->BytesPerChannel = CheckFormat.wBitsPerSample / 8;
  Audio->FrameSize = CheckFormat.nBlockAlign;
  Audio->BytesPerSecond = CheckFormat.nAvgBytesPerSec;

  REFERENCE_TIME DevicePeriodicy, MinPeriodicy;
  Audio->Client->GetDevicePeriod(&DevicePeriodicy, &MinPeriodicy);
  Audio->Client->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, 0, 0, &CheckFormat, 0);

  u32 DevicePeriodicyMs = (u32)DevicePeriodicy / 10000;
  u32 MinPeriodicyMs = (u32)MinPeriodicy / 10000;
  u32 SamplesPerMs = (u32)CheckFormat.nSamplesPerSec / 1000;
  Audio->LatencyFrameCount = SamplesPerMs / DevicePeriodicyMs;

  REFERENCE_TIME MinDuration, MaxDuration;
  Audio->Client->GetBufferSizeLimits(&CheckFormat, false, &MinDuration, &MaxDuration);

  
  Audio->BufferMutex = CreateMutexExW(0, 0, 0, 0);

  Audio->Client->GetBufferSize(&Audio->FrameCount);

  if(Audio->Client->Start() != S_OK)
  {
    return false;
  }
  Audio->Client->GetService(IID_PPV_ARGS(&Audio->RenderClient));
  
  u32 BufferSeconds = 2;
  Audio->BufferFrameCount = BufferSeconds * Audio->SamplesPerSecond;
  
  u64 BufferMemorySize = BufferSeconds * Audio->BytesPerSecond;
  u8 *BufferMemory = (u8 *)Win32Alloc(2 * BufferMemorySize);

  Audio->Buffers[0] = {};
  Audio->Buffers[1] = {};

  Audio->Buffers[0].Samples = (s16 *)(BufferMemory + 0);
  Audio->Buffers[1].Samples = (s16 *)(BufferMemory + BufferMemorySize);

  Audio->CurrentBufferIndex_ = 0;

  return true;
}

DWORD
Win32AudioThread(void *Data)
{
  win32_audio *Audio = (win32_audio *)Data;

  while(true)
  {
    u32 AudioFramePadding;
    Audio->Client->GetCurrentPadding(&AudioFramePadding);

    u32 AudioFrameCount = Audio->FrameCount - AudioFramePadding;
    if(AudioFrameCount)
    {
      WaitForSingleObject(Audio->BufferMutex, INFINITE);

      win32_audio_buffer *CurrentBuffer = Audio->Buffers + Audio->CurrentBufferIndex_;

      BYTE *DstBuffer;
      Audio->RenderClient->GetBuffer(AudioFrameCount, &DstBuffer);
    
      u64 FirstBatchCount = Min(Audio->BufferFrameCount - CurrentBuffer->FrameIndex, AudioFrameCount);
      DstBuffer = (BYTE *)MemoryCopy((u8 *)CurrentBuffer->Samples + Audio->FrameSize * CurrentBuffer->FrameIndex, DstBuffer, Audio->FrameSize * FirstBatchCount);

      u64 SecondBatchCount = AudioFrameCount - FirstBatchCount;
      MemoryCopy(CurrentBuffer->Samples, DstBuffer, Audio->FrameSize * SecondBatchCount);

      Audio->RenderClient->ReleaseBuffer(AudioFrameCount, 0);
      AtomicAddU32(&Audio->FramesPlayedCount_, AudioFrameCount); 

      CurrentBuffer->FrameIndex += AudioFrameCount; 

      ReleaseMutex(Audio->BufferMutex);
    }
  }

  return 0;
}

void 
Win32AudioSwapBuffers(win32_audio *Audio)
{
  WaitForSingleObject(Audio->BufferMutex, INFINITE);

  win32_audio_buffer *NewBuffer = Audio->Buffers + (Audio->CurrentBufferIndex_ ^= 1);
  NewBuffer->FrameIndex = Audio->FramesPlayedCount_;

  ReleaseMutex(Audio->BufferMutex);
}

void
Win32AudioGetBuffer(win32_audio *Audio, s16 **Samples, u32 *AudioFramesPlayedCount)
{
  WaitForSingleObject(Audio->BufferMutex, INFINITE);
  
  win32_audio_buffer *Buffer = Audio->Buffers + (Audio->CurrentBufferIndex_ ^ 1);
  *Samples = Buffer->Samples;
  *AudioFramesPlayedCount = AtomicExchangeU32(&Audio->FramesPlayedCount_, 0);

  ReleaseMutex(Audio->BufferMutex);

  MemoryZeroSize(Buffer->Samples, Audio->FrameSize * Audio->BufferFrameCount);
}
