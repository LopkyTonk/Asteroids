struct push_transform
{
  float2 Position;
  float2 Scale;
  float4x4 Rotation;
};
ConstantBuffer<push_transform> PushTransform : register(b0, space0);

struct vertex_shader_input
{
  float2 Position : POSITION;
  float3 Color : COLOR;
};

struct vertex_shader_output
{
  float4 Position : SV_POSITION;
  float3 Color : COLOR;
};


  vertex_shader_output
MainVS(vertex_shader_input In)
{
  float4 Position = mul(PushTransform.Rotation, float4(In.Position, 0.5f, 1.0f));
  Position += float4(PushTransform.Position, 0.0f, 0.0f);

  float4 Scale = float4(PushTransform.Scale, 1.0f, 1.0f);
  Position = Scale * Position;

  vertex_shader_output Out;
  Out.Position = Position;
  Out.Color = In.Color;

  return Out;
}

float4 
MainPS(vertex_shader_output In) : SV_TARGET
{
  return float4(In.Color, 1.0f);
}
