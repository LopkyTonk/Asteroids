#pragma once
struct v2
{
  r32 X;
  r32 Y;
};

v2
V2()
{
  v2 Result;
  Result.X = 0.0f;
  Result.Y = 0.0f;

  return Result;
}


v2
V2(r32 Scalar)
{
  v2 Result;
  Result.X = Scalar;
  Result.Y = Scalar;

  return Result;
}

v2
V2(r32 X, r32 Y)
{
  v2 Result;
  Result.X = X;
  Result.Y = Y;

  return Result;
}

v2 
V2S32(s32 X, s32 Y)
{
  v2 Result;
  Result.X = (r32)X;
  Result.Y = (r32)Y;

  return Result;

}

v2 
V2U32(u32 S)
{
  v2 Result;
  Result.X = (r32)S;
  Result.Y = (r32)S;

  return Result;

}
v2 
V2U32(u32 X, u32 Y)
{
  v2 Result;
  Result.X = (r32)X;
  Result.Y = (r32)Y;

  return Result;

}
 

v2 &
operator*=(v2 &Result, r32 Scalar)
{
  Result.X *= Scalar;
  Result.Y *= Scalar;

  return Result;
}
v2
operator*(r32 Scalar, v2 Vec)
{
  v2 Result;
  Result.X = Scalar * Vec.X;
  Result.Y = Scalar * Vec.Y;

  return Result;
}

v2 
operator-(v2 V)
{
  v2 Result;
  Result.X = -V.X;
  Result.Y = -V.Y;

  return Result;
}

v2
operator-(v2 Vector, r32 Scalar)
{
  v2 Result;
  Result.X = Vector.X - Scalar;
  Result.Y = Vector.Y - Scalar;

  return Result;
}

v2
operator+(v2 Vector, r32 Scalar)
{
  v2 Result;
  Result.X = Vector.X + Scalar;
  Result.Y = Vector.Y + Scalar;

  return Result;
}

v2
operator-(v2 Lhs, v2 Rhs)
{
  v2 Result;
  Result.X = Lhs.X - Rhs.X;
  Result.Y = Lhs.Y - Rhs.Y;

  return Result;
}

v2&
operator-=(v2& Lhs, v2 Rhs)
{
  Lhs.X -= Rhs.X;
  Lhs.Y -= Rhs.Y;

  return Lhs;
}

v2 
operator+(v2 V)
{
  v2 Result;
  Result.X = +V.X;
  Result.Y = +V.Y;

  return Result;
}

v2&
operator+=(v2 &Lhs, v2 Rhs)
{
  Lhs.X += Rhs.X;
  Lhs.Y += Rhs.Y;

  return Lhs;
}

v2
operator+(v2 Lhs, v2 Rhs)
{
  v2 Result;
  Result.X = Lhs.X + Rhs.X;
  Result.Y = Lhs.Y + Rhs.Y;

  return Result;
}


v2
Hadamard(v2 Lhs, v2 Rhs)
{
  v2 Result;
  Result.X = Lhs.X * Rhs.X;
  Result.Y = Lhs.Y * Rhs.Y;

  return Result;
}

r32
Length(v2 Vec)
{
  r32 Result = Sqrt(Square(Vec.X) + Square(Vec.Y)); 
  return Result;
}

r32 Inner(v2 Lhs, v2 Rhs)
{
  r32 Result = Lhs.X * Rhs.X + Lhs.Y * Rhs.Y;
  return Result;
}
r32 Inner(v2 Vec)
{
  r32 Result = Inner(Vec, Vec);
  return Result;
}
r32
Cross(v2 Lhs, v2 Rhs)
{
  r32 Result = Lhs.X * Rhs.Y - Lhs.Y * Rhs.X;
  return Result;
}

v2 
Lerp(r32 T, v2 A, v2 B)
{
  v2 Result = A + (1.0f - T) * (B - A);
  return Result;
}

struct v3
{
  union
  {
    struct
    {
      r32 X;
      r32 Y;
      r32 Z;
    };
    struct
    {
      r32 R;
      r32 G;
      r32 B;
    };
    struct 
    {
      v2 XY;
      r32 Z;
    };
  };
};
v3
V3(r32 S)
{
  v3 Result;
  Result.X = S;
  Result.Y = S;
  Result.Z = S;

  return Result;
}
v3
V3(r32 X, r32 Y, r32 Z)
{
  v3 Result;
  Result.X = X;
  Result.Y = Y;
  Result.Z = Z;

  return Result;
}
v3
V3(v2 XY, r32 Z)
{
  v3 Result;
  Result.XY = XY;
  Result.Z = Z;

  return Result;
}
v3
operator*(r32 Scalar, v3 Vec)
{
  v3 Result;
  Result.X = Scalar * Vec.X;
  Result.Y = Scalar * Vec.Y;
  Result.Z = Scalar * Vec.Z;

  return Result;
}

v3
Hadamard(v3 V0, v3 V1)
{
  v3 Result;
  Result.X = V0.X * V1.X;
  Result.Y = V0.Y * V1.Y;
  Result.Z = V0.Z * V1.Z;

  return Result;
}

struct v4
{
  union
  {
    struct
    {
      v2 XY;
      v2 ZW;
    };
    
    struct
    {
      r32 X;
      r32 Y;
      r32 Z;
      r32 W;
    };
  };
};

v4
V4(r32 X, r32 Y, r32 Z, r32 W)
{
  v4 Result;
  Result.X = X;
  Result.Y = Y;
  Result.Z = Z;
  Result.W = W;

  return Result;
}

struct rect
{
  v2 Min;
  v2 Max;
};


rect 
RectHalfDim(v2 HalfDim)
{
  rect Result;
  Result.Min = -HalfDim;
  Result.Max = HalfDim;

  return Result;
}
rect
RectMinMax(r32 MinX, r32 MinY, r32 MaxX, r32 MaxY)
{
  rect Result;
  Result.Min = V2(MinX, MinY);
  Result.Max = V2(MaxX, MaxY);

  return Result;
}

rect
RectMinMax(v2 Min, v2 Max)
{
  rect Result;
  Result.Min = Min;
  Result.Max = Max;

  return Result;
}

rect
RectCenterSize(v2 Center, v2 Size)
{
  rect Result;
  Result.Min = Center - (0.5f * Size);
  Result.Max = Center + (0.5f * Size);

  return Result;
}
rect
RectMinSize(v2 Min, v2 Size)
{
  rect Result;
  Result.Min = Min;
  Result.Max = Min + Size;

  return Result;
}

r32 
RectWidth(rect R)
{
  r32 Result = R.Max.X - R.Min.X;
  return Result;
}
r32 
RectHeight(rect R)
{
  r32 Result = R.Max.Y - R.Min.Y;
  return Result;
}
v2 RectSize(rect R)
{
  v2 Result;
  Result.X = RectWidth(R);
  Result.Y = RectHeight(R);

  return Result;
}

b32 RectHasArea(rect Rect)
{
  b32 Result = (Rect.Min.X < Rect.Max.X &&
                Rect.Min.Y < Rect.Max.Y);

  return Result;
}

v2 
RectClampPoint(rect R, v2 P)
{
  v2 Result;
  Result.X = Clamp(P.X, R.Min.X, R.Max.X);
  Result.Y = Clamp(P.Y, R.Min.Y, R.Max.Y);

  return Result;
}

b32
RectPointInside(rect R, v2 P)
{
   b32 Result = (P.X >= R.Min.X && P.X < R.Max.X && P.Y >= R.Min.Y && P.Y < R.Max.Y);
   return Result;
}

rect
RectSetIntersection(rect RectA, rect RectB)
{
  rect Result;
  Result.Min.X = Max(RectA.Min.X, RectB.Min.X);
  Result.Min.Y = Max(RectA.Min.Y, RectB.Min.Y);
  Result.Max.X = Min(RectA.Max.X, RectB.Max.X);
  Result.Max.Y = Min(RectA.Max.Y, RectB.Max.Y);

  return Result;
}

rect
RectSetUnion(rect RectA, rect RectB)
{
  rect Result;
  Result.Min.X = Min(RectA.Min.X, RectB.Min.X);
  Result.Min.Y = Min(RectA.Min.Y, RectB.Min.Y);
  Result.Max.X = Max(RectA.Max.X, RectB.Max.X);
  Result.Max.Y = Max(RectA.Max.Y, RectB.Max.Y);

  return Result;
}

rect
RectShrinkSize(rect RectA, v2 Size)
{
  rect Result;
  Result.Min = RectA.Min + 0.5f * Size;
  Result.Max = RectA.Max - 0.5f * Size;

  return Result;
}

struct
mat2x2
{
  union 
  {
    struct
    {
      v2 X;
      v2 Y;
    };
    v2 E[2];
  };
};

mat2x2
Mat2x2Identity()
{
  mat2x2 Result = {};
  Result.X.X = 1;
  Result.Y.Y = 1;

  return Result;
}

mat2x2
Mat2x2Rotation(r32 Radians)
{
  mat2x2 Result = {};
  Result.X.X = Cos(Radians);
  Result.X.Y = Sin(Radians);
  Result.Y.X = -Sin(Radians);
  Result.Y.Y = Cos(Radians);

  return Result;
}

v2 
operator*(mat2x2 Mat, v2 Vec)
{
  v2 Result;
  Result.X = Mat.X.X * Vec.X + Mat.X.Y * Vec.Y; 
  Result.Y = Mat.Y.X * Vec.X + Mat.Y.Y * Vec.Y; 

  return Result;
}

struct
mat4x4
{
  union 
  {
    struct
    {
      v4 X;
      v4 Y;
      v4 Z;
      v4 W;
    };
    v4 E[4];
  };
};

mat4x4
Mat4x4Identity()
{
  mat4x4 Result = {};
  Result.X.X = 1;
  Result.Y.Y = 1;
  Result.Z.Z = 1;
  Result.W.W = 1;

  return Result;
}
