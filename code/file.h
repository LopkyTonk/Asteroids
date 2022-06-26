#pragma pack(push, 1)
struct file_header_bmp
{
  u8 Type[2];
  u32 Size;
  u16 Reserved1;
  u16 Reserved2;
  u32 OffsetBits;

  u32 HeaderSize;
  u32 Width;
  u32 Height;
  u16 Planes;
  u16 BitCount;
  u32 Compression;
  u32 SizeImage;
  u32 XPixelsPerMeter;
  u32 YPixelsPerMeter;
  u32 ColorsUsed;
  u32 ColorsImportant;

  u32 RedMask;
  u32 GreenMask;
  u32 BlueMask;
};

#pragma pack(pop)

bitmap
FileLoadBitmap(void *Memory)
{
  file_header_bmp *BmpHeader = (file_header_bmp *)Memory;
  
  u8 *Pixels = (u8 *)Memory + BmpHeader->OffsetBits;
  u32 Width = BmpHeader->Width;
  u32 Height = BmpHeader->Height;
  u32 Pitch = 4 * Width;

  bitmap Result = Bitmap(Pixels, Width, Height, Pitch);

  u32 RMask = BmpHeader->RedMask;
  u32 GMask = BmpHeader->GreenMask;
  u32 BMask = BmpHeader->BlueMask;
  u32 AMask = ~(RMask | GMask | BMask);

  u32 RShift = LSBU32(RMask).Index;
  u32 GShift = LSBU32(GMask).Index;
  u32 BShift = LSBU32(BMask).Index;
  u32 AShift = LSBU32(AMask).Index;

  u8 *PixelsRow = (u8 *)Memory + BmpHeader->OffsetBits;
  for(u32 Y = 0;
      Y < Result.Height;
      ++Y)
  {
    u32 *Pixel = (u32 *)PixelsRow;

    for(u32 X = 0;
        X < Result.Width;
        ++X)
    {
      u32 R = ((*Pixel) >> RShift) & 0xff;
      u32 G = ((*Pixel) >> GShift) & 0xff;
      u32 B = ((*Pixel) >> BShift) & 0xff;
      u32 A = ((*Pixel) >> AShift) & 0xff;

      *Pixel++ = (R << 0) | (G << 8) | (B << 16) | (A << 24);
    }

    PixelsRow += Pitch;
  }
  
  return Result;
}
