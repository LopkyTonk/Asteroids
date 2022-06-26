struct sound_loaded
{
  s16 *Samples;
  u32 SampleCount;
};


struct wave_header
{
   u32 RIFFID;
   u32 Size;
   u32 WAVEID;
};
#define RIFF_CODE(a, b, c, d) ((static_cast<u32>(a) << 0) | (static_cast<u32>(b) << 8) | (static_cast<u32>(c) << 16) | (static_cast<u32>(d) << 24))
enum
{
   WAVE_CHUNKID_RIFF = RIFF_CODE('R', 'I', 'F', 'F'),
   WAVE_CHUNKID_fmt = RIFF_CODE('f', 'm', 't', ' '),
   WAVE_CHUNKID_WAVE = RIFF_CODE('W', 'A', 'V', 'E'),
   WAVE_CHUNKID_data = RIFF_CODE('d', 'a', 't', 'a'),
};
struct wave_chunk
{
   u32 ID;
   u32 Size;
};

struct wave_fmt
{
   u16 FormatTag;
   u16 Channels;
   u32 SamplesPerSec;
   u32 AvgBytesPerSec;
   u16 BlockAlign;
   u16 BitsPerSample;
   u16 ValidBitsPerSample;
   u32 ChannelMask;
   u8 SubFormat[16];
};

struct riff_iter
{
   u8 *At;
   u8 *End;
};
riff_iter
RIFFParseChunkAt(void *At, void *End)
{
   riff_iter Result;
   Result.At = reinterpret_cast<u8 *>(At);
   Result.End = reinterpret_cast<u8 *>(End);

   return Result;
}

b32
RIFFValid(riff_iter Iter)
{
   b32 Result = Iter.At < Iter.End;

   return Result;
}
riff_iter
RIFFNextChunk(riff_iter Iter)
{
   wave_chunk *Chunk = (wave_chunk *)Iter.At;

   u32 Size = (Chunk->Size + 1) & ~1;
   Iter.At += sizeof(wave_chunk) + Size;

   return Iter;
}

u32
RIFFType(riff_iter Iter)
{
   wave_chunk *Chunk = (wave_chunk *)Iter.At;
   u32 Result = Chunk->ID;

   return Result;
}

void *
RIFFChunkData(riff_iter Iter)
{
   void *Result = Iter.At + sizeof(wave_chunk);

   return Result;
}

u32
RIFFChunkDataSize(riff_iter Iter)
{
   wave_chunk *Chunk = (wave_chunk *)Iter.At;
   u32 Result = Chunk->Size;

   return Result;
}
sound_loaded
SoundLoad(char *FileName)
{
  sound_loaded Result = {};
  win32_file File = Win32FileRead(FileName);

  if(File.Memory)
  {
    wave_header *Header = (wave_header *)File.Memory;
    Assert(Header->RIFFID == WAVE_CHUNKID_RIFF);
    Assert(Header->WAVEID == WAVE_CHUNKID_WAVE);

    u32 Channels = 0;
    s16 *SampleData = 0;
    u32 SampleDataSize = 0;

    for(riff_iter Iter = RIFFParseChunkAt(Header + 1,(u8 *)(Header + 1) + Header->Size - 4);
        RIFFValid(Iter);
        Iter = RIFFNextChunk(Iter))
    {
      switch(RIFFType(Iter))
      {
        case WAVE_CHUNKID_fmt:
          {
            wave_fmt *FMT = (wave_fmt *)RIFFChunkData(Iter);
            Assert(FMT->FormatTag == 1);
            Assert(FMT->SamplesPerSec == 48'000);
            Assert(FMT->BitsPerSample == 16);
            Assert(FMT->BlockAlign == 2 * FMT->Channels);

            Channels = FMT->Channels;
          } break;
        case WAVE_CHUNKID_data:
          {
            SampleData = (s16 *)RIFFChunkData(Iter);
            SampleDataSize = RIFFChunkDataSize(Iter);
          } break;
      }
    }

    Assert(Channels && SampleData && SampleDataSize);
  
    Result.Samples = SampleData;
    Result.SampleCount = SampleDataSize / (Channels * sizeof(s16));

    if(Channels == 1)
    {
    }
    else if(Channels == 2)
    {
      s16 *DstSample = SampleData + 1;
      s16 *SrcSample = SampleData + 2;

      for(u32 CopySampleIndex = 1;
          CopySampleIndex < Result.SampleCount;
          ++CopySampleIndex)
      {
        *DstSample = *SrcSample;

        ++DstSample;
        SrcSample += 2;
      }
    }
    else
    {
      InvalidCodePath;
    }
  }

  return Result;
}

