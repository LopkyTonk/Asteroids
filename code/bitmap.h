struct bitmap
{
  // R:0-7 G:8-15 B:16-23 A:24-31
  u8 *Pixels;
  u32 Width;
  u32 Height;
  // in bytes
  u32 Pitch;
};

bitmap
Bitmap(u8 *Pixels, u32 Width, u32 Height, u32 Pitch)
{
  bitmap Result;
  Result.Pixels = Pixels;
  Result.Width = Width;
  Result.Height = Height;
  Result.Pitch = Pitch;

  return Result;
}

bitmap
Bitmap(u8 *Pixels, u32 Width, u32 Height)
{
  bitmap Result = Bitmap(Pixels, Width, Height, 4 * Width);

  return Result;
}

bitmap 
BitmapView(bitmap *SubBitmap, u32 Width, u32 Height, u32 OffsetX, u32 OffsetY)
{
  Assert(Width <= SubBitmap->Width);
  Assert(Height <= SubBitmap->Height);
  Assert(Width + OffsetX <= SubBitmap->Width);
  Assert(Height + OffsetY <= SubBitmap->Height);
  

  bitmap Result;
  Result.Pixels = (u8 *)SubBitmap->Pixels + OffsetY * SubBitmap->Pitch + 4 * OffsetX;
  Result.Width = Width;
  Result.Height = Height;
  Result.Pitch = SubBitmap->Pitch;

  return Result;
}

bitmap 
BitmapView(bitmap *SubBitmap, v2 Size, v2 Offset)
{
  u32 Width = (u32)Floor(Size.X);
  u32 Height = (u32)Floor(Size.Y);
  u32 OffsetX = (u32)Floor(Offset.X);
  u32 OffsetY = (u32)Floor(Offset.Y);

  bitmap Result = BitmapView(SubBitmap, Width, Height, OffsetX, OffsetY);
  return Result;
}

  void
BitmapClear(bitmap Bitmap, u32 Color)
{
  if(Bitmap.Pitch == Bitmap.Width)
  {
    MemorySetSize(Bitmap.Pixels, 4 * Bitmap.Width * Bitmap.Height, Color);
  }
  else
  {
    for(u32 RowIndex = 0;
        RowIndex < Bitmap.Height;
        ++RowIndex)
    {
      MemorySetSize(Bitmap.Pixels + RowIndex * Bitmap.Pitch, Bitmap.Pitch, Color);
    }
  }
}

  void 
BitmapCopy(bitmap Src, bitmap Dst)
{
  Assert(Src.Width == Dst.Width);
  Assert(Src.Height == Dst.Height);

  if(Src.Pitch == Dst.Pitch)
  {
    MemoryCopy(Src.Pixels, Dst.Pixels, Src.Height * Src.Pitch);
  }
  else
  {
    u32 Pitch = Min(Src.Pitch, Dst.Pitch);
    for(u32 Y = 0;
        Y < Src.Height;
        ++Y)
    {
      MemoryCopy(Src.Pixels + Y * Src.Pitch, Dst.Pixels + Y * Dst.Pitch, Pitch);
    }
  }
}
