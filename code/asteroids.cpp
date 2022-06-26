#include "platform.h"
#include "win32.h"

#include "intrinsics.h"
#include "mem.h"
#include "mat.h"
#include "bitmap.h"
#include "str.h"
#include "file.h"

#include "win32_audio.h"
#include "win32_dx.h"

#include "audio.h"

#define ColorRGB(R, G, B) (V3((r32)R / 255.0f, (r32)G / 255.0f, (r32)B / 255.0f))

#define ForceFullscreen 1

struct vertex
{
  v2 Position;
  v3 Color;
};
vertex PlayerVertices[] = 
{
  vertex{ V2( 0.50f, 0.00f), V3(1.0f, 1.0f, 1.0f) },
  vertex{ V2(-0.50f, 0.50f), V3(1.0f, 1.0f, 1.0f) },
  vertex{ V2(-0.50f, 0.25f), V3(1.0f, 1.0f, 1.0f) },
  vertex{ V2(-0.25f, 0.00f), V3(1.0f, 1.0f, 1.0f) },
  vertex{ V2(-0.50f,-0.25f), V3(1.0f, 1.0f, 1.0f) },
  vertex{ V2(-0.50f,-0.50f), V3(1.0f, 1.0f, 1.0f) },
};
u32 PlayerIndices[] = 
{
  0, 1, 
  1, 2, 
  2, 3,
  3, 4, 
  4, 5,
  5, 0
};

enum asteroid_type
{
  AsteroidType_Small,
  AsteroidType_Medium,
  AsteroidType_Large,

  AsteroidType_Count,
  AsteroidType_First = 0,
  AsteroidType_Last = AsteroidType_Count - 1,
};
#define AsteroidSize(AsteroidType) (0.0f + 0.375f * (1 << AsteroidType))
vertex AsteroidVertices[AsteroidType_Count][4]{};

u32 AsteroidIndices[]
{
  0, 1,
    1, 2,
    2, 3, 
    3, 0
};
vertex LaserVertices[] = 
{
  vertex{ V2(-0.15f, 0.00f), V3(0.25f, 0.87f, 0.82f) },
  vertex{ V2( 0.15f, 0.00f), V3(0.25f, 0.87f, 0.82f) },
};
u32 LaserIndices[] = 
{
  0, 1, 
};


struct transform
{
  v2 Position;
  v2 DeltaPosition;
  r32 DeltaDeltaPosition;

  r32 Rotation;
  r32 DeltaRotation;
};

struct asteroid
{
  asteroid_type Type;
  transform Transform;
};

struct laser
{
  transform Transform;
  u32 LifetimeMs;
};

struct player
{
  transform Transform;
  r32 ForwardImpulse;
  r32 RotationImpulse;
};

struct input
{
  b32 LeftPressed;
  b32 RightPressed;
  b32 UpPressed;
  b32 DownPressed;
  b32 SpacePressed;
};

enum game_sound
{
  GameSound_Music,
  GameSound_Laser,
  GameSound_Explosion,
  GameSound_Count
};

b32
IntersectedLineLine(v2 Start0, v2 End0, v2 Start1, v2 End1)
{
  b32 Result = false;

  v2 Dir0 = End0 - Start0;
  v2 Dir1 = End1 - Start1;

  r32 DirCross0 = Cross(Dir0, Dir1);
  r32 DirCross1 = Cross(Dir1, Dir0);
  if(DirCross0)
  {
    r32 T0 = Cross(Start1 - Start0, Dir1) / DirCross0;
    r32 T1 = Cross(Start0 - Start1, Dir0) / DirCross1;
    
    Result = T0 >= 0 && T0 <= 1.0f && T1 >= 0 && T1 <= 1.0f;
  }

  return Result;
}

struct window_main
{
  HWND Handle;
  b32 KeepRunning;
};


struct
dx_push_transform
{
  v2 Position;
  v2 Scale;
  mat4x4 Rotation;
};

  template <typename subobject_typename> 
struct alignas(void *)
  dx_pipeline_state_subobject
{
  D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type;
  subobject_typename Subobject;
};

window_main WindowMain;
dx_context DXContext;

  LRESULT 
Win32WindowProc(HWND Window, UINT Msg, WPARAM WParam, LPARAM LParam)
{
  LRESULT Result = 0;

  switch(Msg)
  {
  #if 0
    case WM_SIZE:
      {
        u32 NewWidth = ((u32)LParam >> 0) & 0xffff;
        u32 NewHeight = ((u32)LParam >> 16) & 0xffff;

        NewWidth = Max(1, NewWidth);
        NewHeight = Max(1, NewHeight);

        if(DXContext.SwapChainWidth != NewWidth || DXContext.SwapChainHeight != NewHeight)
        {
          DXContext.SwapChainWidth = NewWidth;
          DXContext.SwapChainHeight = NewHeight;
          AtomicExchangeU32(&DXContext.SwapChainIsOld, 1);
        }
      } break;
#endif
    case WM_CLOSE:
      {
        DestroyWindow(Window);
      } break;
    case WM_DESTROY:
      {
        PostQuitMessage(0);
        WindowMain.KeepRunning = false;
      } break;
      case WM_SETCURSOR:
      {
        u32 HitTest = (u32)LParam;
        if(HitTest == HTCLIENT)
        {
          SetCursor(false);
        }
        else
        {
          Result = DefWindowProc(Window, Msg, WParam, LParam);
        }
      } break;
    default:
      {
        Result = DefWindowProc(Window, Msg, WParam, LParam);
      } break;
  }


  return Result;
}


int WinMain(HINSTANCE Instance,
    HINSTANCE PrevInstante,
    LPSTR CmdLine, 
    int ShowCmd)

{
  win32_audio Audio;
  if(!Win32AudioInitialize(&Audio))
  {
    OutputDebugStringA("Failed to initailize audio");
    return -1;
  }
  CreateThread(0, 0, Win32AudioThread, &Audio, 0, 0);

  char ClassName[] = "WINDOW_ASTEROIDS";
  WNDCLASS WC = {};
  WC.lpfnWndProc = Win32WindowProc;
  WC.hInstance = Instance;
  WC.lpszClassName = ClassName;
  WC.hCursor = 0; 

  RegisterClass(&WC);
  
  WindowMain.Handle = CreateWindowExA(WS_EX_APPWINDOW, ClassName, "asteroids", WS_SYSMENU, 
      CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
      0, 0, Instance, 0);
      ShowCursor(false);
  if(!WindowMain.Handle)
  {
    OutputDebugStringA("Unable to create window");
    return -1;
  }

#if ForceFullscreen
  {
    HDC WindowDC = GetWindowDC(0); 
    SetWindowLong(WindowMain.Handle, GWL_STYLE, 0);
    SetWindowPos(WindowMain.Handle, 0, 0, 0, GetDeviceCaps(WindowDC, HORZRES), GetDeviceCaps(WindowDC, VERTRES), SWP_FRAMECHANGED);
  }
#endif
  ShowWindow(WindowMain.Handle, SW_SHOW);
  WindowMain.KeepRunning = true;

#if SNAKE_DEBUG
  ID3D12Debug* DebugController;
  D3D12GetDebugInterface(IID_PPV_ARGS(&DebugController));
  if(!DebugController)
  {
    OutputDebugStringA("Unable to get debug interface");
    return -1;
  }
  DebugController->EnableDebugLayer(); 
#endif

  IDXGIFactory4 *Factory = DXCreateFactory4();
  if(!Factory)
  {
    OutputDebugStringA("Unable to create factory");
    return -1;
  }

  IDXGIAdapter1 *Adapter1 = 0;
  SIZE_T MaxDedicatedVideoMemory = 0;
#define D3D12_VERSION D3D_FEATURE_LEVEL_11_0 
  for (u32 AdapterIndex = 0;;
      ++AdapterIndex)
  {
    IDXGIAdapter1 *QueryAdapter;
    HRESULT EnumResult = Factory->EnumAdapters1(AdapterIndex, &QueryAdapter);

    if(EnumResult == DXGI_ERROR_NOT_FOUND)
    {
      break;
    }
    else
    {
      DXGI_ADAPTER_DESC1 AdapterDescriptor;
      QueryAdapter->GetDesc1(&AdapterDescriptor);

      if((AdapterDescriptor.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0)
      {
        HRESULT CreateResult = D3D12CreateDevice(QueryAdapter, D3D12_VERSION, __uuidof(ID3D12Device), 0);
        if(CreateResult == S_FALSE && MaxDedicatedVideoMemory < AdapterDescriptor.DedicatedVideoMemory)
        {
          MaxDedicatedVideoMemory = AdapterDescriptor.DedicatedVideoMemory;
          Adapter1 = QueryAdapter;
        }
      }
    }
  }
  if(!Adapter1)
  {
    OutputDebugStringA("Unable to find suitable GPU");
    return -1;
  }
  IDXGIAdapter4 *Adapter;
  if(Adapter1->QueryInterface(IID_PPV_ARGS(&Adapter)) != S_OK)
  {
    OutputDebugStringA("Unablet to get Adapter4 from Adapter1!");
    return -1;
  }
  if(D3D12CreateDevice(Adapter, D3D12_VERSION, IID_PPV_ARGS(&DXContext.Device)) != S_OK)
  {
    OutputDebugStringA("Unable to create device");
    return -1;
  }
#if SNAKE_DEBUG
  {
    ID3D12InfoQueue* InfoQueue;
    if(DXContext.Device->QueryInterface(IID_PPV_ARGS(&InfoQueue)) != S_OK)
    {
      OutputDebugStringA("Failed to query info queue");
      return -1;
    }
    InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
    InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
    InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
    D3D12_MESSAGE_SEVERITY Severities[] =
    {
      D3D12_MESSAGE_SEVERITY_INFO 
    };
    // Suppress individual messages by their ID
    D3D12_MESSAGE_ID DenyIds[] =
    {
      D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,   // I'm really not sure how to avoid this message.
      D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,                         // This warning occurs when using capture frame while graphics debugging.
      D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,                       // This warning occurs when using capture frame while graphics debugging.
    };

    D3D12_INFO_QUEUE_FILTER NewFilter = {};
#if 0
    NewFilter.DenyList.NumCategories = _countof(Categories);
    NewFilter.DenyList.pCategoryList = Categories;
#endif
    NewFilter.DenyList.NumSeverities = ArrayLength(Severities);
    NewFilter.DenyList.pSeverityList = Severities;
    NewFilter.DenyList.NumIDs = ArrayLength(DenyIds);
    NewFilter.DenyList.pIDList = DenyIds;
    if(InfoQueue->PushStorageFilter(&NewFilter) != S_OK)
    {
      OutputDebugStringA("Failed to push storage filter");
      return -1;
    }
  }
#endif


  {
    D3D12_COMMAND_QUEUE_DESC Desc = {};
    Desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    Desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    Desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    Desc.NodeMask = 0;

    if(DXContext.Device->CreateCommandQueue(&Desc, IID_PPV_ARGS(&DXContext.CommandQueue)) != S_OK)
    {
      OutputDebugStringA("Failed to create command queue");
      return -1;
    }
  }

  {
    IDXGIFactory4 *SwapChainFactory = DXCreateFactory4();

    RECT WindowRect;
    GetClientRect(WindowMain.Handle, &WindowRect);

    u32 Width = WindowRect.right - WindowRect.left;
    u32 Height = WindowRect.bottom - WindowRect.top;


    DXContext.SwapChainWidth = Width;
    DXContext.SwapChainHeight = Height;

    DXGI_SWAP_CHAIN_DESC1 SwapChainDesc = {};
    SwapChainDesc.Width = Width;
    SwapChainDesc.Height = Height;
    SwapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    SwapChainDesc.Stereo = FALSE;
    SwapChainDesc.SampleDesc = { 1, 0 };
    SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    SwapChainDesc.BufferCount = SwapchainFrameCount;
    SwapChainDesc.Scaling = DXGI_SCALING_NONE;
    SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    SwapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED; 

    IDXGISwapChain1 *SwapChain1;
    if(SwapChainFactory->CreateSwapChainForHwnd(DXContext.CommandQueue, WindowMain.Handle, &SwapChainDesc, 0, 0, &SwapChain1) != S_OK)
    {
      OutputDebugStringA("Failed to create swapchain queue");
      return -1;
    }
    if(SwapChain1->QueryInterface(IID_PPV_ARGS(&DXContext.SwapChain)) != S_OK)
    {
      OutputDebugStringA("Unablet to get SwapChain4 from SwapChain1!");
      return -1;
    }

    SwapChainFactory->MakeWindowAssociation(WindowMain.Handle, DXGI_MWA_NO_ALT_ENTER);
  }


  {
    if(!DXDescriptorHeapCreate(&DXContext, SwapchainFrameCount, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, &DXContext.RTVDescriptorHeap))
    {
      OutputDebugStringA("Failed to create RTV descriptor heap");
      return -1;
    }
#if 0
    u32 SRVDescriptorCount = SwapchainFrameCount;
    if(!DXDescriptorHeapCreate(&DXContext, SRVDescriptorCount, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, &DXContext.SRVDescriptorHeap))
    {
      OutputDebugStringA("Failed to create SRV descriptor heap");
      return -1;
    }
#endif
  }

  {
    if(!DXContextCreateRTVs(&DXContext))
    {
      OutputDebugStringA("Failed to create RTVs");
      return -1;
    }
  }

  {
    for(u32 FrameIndex = 0; 
        FrameIndex < SwapchainFrameCount; 
        ++FrameIndex)
    {
      dx_command_buffer *CommandBuffer = DXContext.CommandBuffers + FrameIndex;
      if(DXContext.Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&CommandBuffer->Allocator)) != S_OK)
      {
        OutputDebugStringA("Failed to create command allocator");
        return -1;
      }

      if(DXContext.Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, CommandBuffer->Allocator, 0, IID_PPV_ARGS(&CommandBuffer->List)) != S_OK)
      {
        OutputDebugStringA("Failed to create command list");
        return -1;
      }
      CommandBuffer->List->Close();

      if(DXContext.Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&CommandBuffer->Fence)) != S_OK)
      {
        OutputDebugStringA("Failed to create command list");
        return -1;
      }
      if((CommandBuffer->FenceEvent = CreateEvent(0, 0, 0, 0)) == INVALID_HANDLE_VALUE)
      {
        OutputDebugStringA("Failed to create fence event");
        return -1;
      }
    }
  }

  {
    for(asteroid_type AsteroidType = AsteroidType_First;
        AsteroidType <= AsteroidType_Last;
        AsteroidType = (asteroid_type)(AsteroidType + 1))
    {
      vertex *DstVertices = AsteroidVertices[AsteroidType];
      r32 AsteroidSizeHalf = AsteroidSize(AsteroidType) / 2.0f;

      DstVertices[0].Position = V2(-AsteroidSizeHalf,-AsteroidSizeHalf);
      DstVertices[1].Position = V2( AsteroidSizeHalf,-AsteroidSizeHalf);
      DstVertices[2].Position = V2( AsteroidSizeHalf, AsteroidSizeHalf);
      DstVertices[3].Position = V2(-AsteroidSizeHalf, AsteroidSizeHalf);

      DstVertices[0].Color = DstVertices[1].Color = DstVertices[2].Color = DstVertices[3].Color = V3(1.0f, 1.0f, 0.0f);
    }

    u64 VertexBufferSize = sizeof(PlayerVertices) + sizeof(AsteroidVertices) + sizeof(LaserVertices);
    ID3D12Resource *VertexBuffer = DXCreateBuffer(&DXContext, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_COPY_DEST, VertexBufferSize);
    if(!VertexBuffer)
    {
      OutputDebugStringA("Failed to create vertex buffer");
      return -1;
    }

    ID3D12Resource *IntVertexBuffer = DXCreateBuffer(&DXContext, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ, VertexBufferSize);
    if(!IntVertexBuffer)
    {
      OutputDebugStringA("Failed to create intermediate vertex buffer");
      return -1;
    }
    {
      void *DstVertexBuffer;
      if(IntVertexBuffer->Map(0, 0, &DstVertexBuffer) != S_OK)
      {
        OutputDebugStringA("Failed to map vertex buffer");
        return -1;
      }
      vertex *DstVertex = (vertex*) MemoryCopy(PlayerVertices, DstVertexBuffer, sizeof(PlayerVertices));
      DstVertex = (vertex*)MemoryCopy(AsteroidVertices, DstVertex, sizeof(AsteroidVertices));
      MemoryCopy(LaserVertices, DstVertex, sizeof(LaserVertices));

      IntVertexBuffer->Unmap(0, 0);
    }

    DXContext.VertexBufferView.BufferLocation = VertexBuffer->GetGPUVirtualAddress();
    DXContext.VertexBufferView.SizeInBytes = (UINT)VertexBufferSize;
    DXContext.VertexBufferView.StrideInBytes = sizeof(PlayerVertices[0]);


    u64 IndexBufferSize =  sizeof(PlayerIndices) + sizeof(AsteroidIndices) + sizeof(LaserIndices);
    ID3D12Resource *IndexBuffer = DXCreateBuffer(&DXContext, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_COPY_DEST, IndexBufferSize);
    if(!IndexBuffer)
    {
      OutputDebugStringA("Failed to create index buffer");
      return -1;
    }

    ID3D12Resource *IntIndexBuffer = DXCreateBuffer(&DXContext, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ, IndexBufferSize);
    if(!IntIndexBuffer)
    {
      OutputDebugStringA("Failed to create intermediate index buffer");
      return -1;
    }
    {
      void *DstIndexBuffer;
      if(IntIndexBuffer->Map(0, 0, &DstIndexBuffer) != S_OK)
      {
        OutputDebugStringA("Failed to map index buffer");
        return -1;
      }
      u32 *DstIndex = (u32 *)MemoryCopy(PlayerIndices, DstIndexBuffer, sizeof(PlayerIndices));
      DstIndex = (u32 *)MemoryCopy(AsteroidIndices, (u8*)DstIndexBuffer + sizeof(PlayerIndices), sizeof(AsteroidIndices));
      MemoryCopy(LaserIndices, DstIndex, sizeof(LaserIndices));

      IntIndexBuffer->Unmap(0, 0);
    }
    DXContext.IndexBufferView.BufferLocation = IndexBuffer->GetGPUVirtualAddress();
    DXContext.IndexBufferView.SizeInBytes = (UINT)IndexBufferSize;
    DXContext.IndexBufferView.Format = DXGI_FORMAT_R32_UINT;

    {
      dx_command_buffer *CommandBuffer = DXContext.CommandBuffers + 0;
      DXCommandBufferReset(CommandBuffer);

      CommandBuffer->List->CopyBufferRegion(VertexBuffer, 0, IntVertexBuffer, 0, VertexBufferSize);
      CommandBuffer->List->CopyBufferRegion(IndexBuffer, 0, IntIndexBuffer, 0, IndexBufferSize);

      if(!DXCommandBufferExecute(CommandBuffer, DXContext.CommandQueue))
      {
        OutputDebugStringA("[ERROR]: Failed to execute command list"); 
        return -1; 
      }
      if(!DXCommandBufferFenceWait(CommandBuffer))
      {
        OutputDebugStringA("[ERROR]: Failed to complete command list"); 
        return -1; 
      }
    }

#if 0
    ID3D12Resource *SB = DXCreateBuffer(&DXContext, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ, SBSizeTotal);
    if(!SB)
    {
      OutputDebugStringA("Failed to create SB buffer");
      return -1;
    }

    dx_transform *SBData;
    SB->Map(0, 0, (void **)&SBData);

    for(u32 FrameIndex = 0; 
        FrameIndex < SwapchainFrameCount; 
        ++FrameIndex)
    {
      DXContext.BufferTransforms[FrameIndex] = SBData + FrameIndex * SBCount;

      D3D12_SHADER_RESOURCE_VIEW_DESC SRVViewDesc	= {};
      SRVViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
      SRVViewDesc.Format = DXGI_FORMAT_UNKNOWN;
      SRVViewDesc.ViewDimension= D3D12_SRV_DIMENSION_BUFFER;
      SRVViewDesc.Buffer.FirstElement= FrameIndex * SBCount;
      SRVViewDesc.Buffer.NumElements= SBCount;
      SRVViewDesc.Buffer.StructureByteStride	= sizeof(dx_transform);
      SRVViewDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

      D3D12_CPU_DESCRIPTOR_HANDLE SRVHandle = DXContext.SRVDescriptorHeap.Heap->GetCPUDescriptorHandleForHeapStart();
      SRVHandle.ptr += FrameIndex * DXContext.SRVDescriptorHeap.Size;

      DXContext.Device->CreateShaderResourceView(SB, &SRVViewDesc, SRVHandle);
    }
#endif 
  }

#if 0
  {
    win32_file TextureFile = Win32FileRead("player.bmp");
    if(!Win32FileIsValid(TextureFile))
    {
      OutputDebugStringA("Failed to open file \"player.bmp\" buffer");
      return -1;
    }


    // Describe and create a Texture2D.
    bitmap TextureBitmap = FileLoadBitmap(TextureFile.Memory);

    u32 DXTextureWidth = TextureBitmap.Width;
    u32 DXTextureHeight = TextureBitmap.Height;
    u32 DXTexturePitch = 4 * AlignNext(TextureBitmap.Width, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
    u32 DXTextureSize = DXTextureHeight * DXTexturePitch;
    ID3D12Resource *TextureBuffer = DXCreateBuffer(&DXContext, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ, DXTextureSize);

    u8 *DXPixels;
    TextureBuffer->Map(0, 0, (void **)&DXPixels);
    BitmapCopy(TextureBitmap, Bitmap(DXPixels, DXTextureWidth, DXTextureHeight, DXTexturePitch));

    ID3D12Resource *Texture = DXCreateTexture(&DXContext, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_COPY_DEST, DXTextureWidth, DXTextureHeight);
    dx_command_buffer *CommandBuffer = DXContext.CommandBuffers;
    DXCommandBufferReset(CommandBuffer);

    D3D12_TEXTURE_COPY_LOCATION Src = {};
    Src.pResource = TextureBuffer;
    Src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    Src.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    Src.PlacedFootprint.Footprint.Depth = 1;
    Src.PlacedFootprint.Footprint.Width = DXTextureWidth;
    Src.PlacedFootprint.Footprint.Height = DXTextureHeight;
    Src.PlacedFootprint.Footprint.RowPitch = DXTexturePitch;

    D3D12_BOX SrcBox;
    SrcBox.left = 0;
    SrcBox.top = 0;
    SrcBox.front = 0;
    SrcBox.right = DXTextureWidth;
    SrcBox.bottom = DXTextureHeight;
    SrcBox.back = 1;

    D3D12_TEXTURE_COPY_LOCATION Dst = {};
    Dst.pResource = Texture;
    Dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    Dst.SubresourceIndex = 0;


    CommandBuffer->List->CopyTextureRegion(&Dst, 0, 0, 0, &Src, &SrcBox);

    D3D12_RESOURCE_BARRIER TextureBarrier;
    TextureBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    TextureBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    TextureBarrier.Transition.pResource = Texture;
    TextureBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    TextureBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    TextureBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

    CommandBuffer->List->ResourceBarrier(1, &TextureBarrier);


    if(!DXCommandBufferExecute(CommandBuffer, DXContext.CommandQueue))
    {
      OutputDebugStringA("[ERROR]: Failed to execute command list"); 
      return -1; 
    }
    if(!DXCommandBufferFenceWait(CommandBuffer))
    {
      OutputDebugStringA("[ERROR]: Failed to complete command list"); 
      return -1; 
    }
    // Describe and create a SRV for the texture.
    D3D12_SHADER_RESOURCE_VIEW_DESC TextureViewDesc = {};
    TextureViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    TextureViewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    TextureViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    TextureViewDesc.Texture2D.MipLevels = 1;

    D3D12_CPU_DESCRIPTOR_HANDLE TextureDescriptorHandle = DXContext.SRVDescriptorHeap.Heap->GetCPUDescriptorHandleForHeapStart();
    TextureDescriptorHandle.ptr += SwapchainFrameCount * DXContext.SRVDescriptorHeap.Size;
    DXContext.Device->CreateShaderResourceView(Texture, &TextureViewDesc, TextureDescriptorHandle);
  }
#endif 

  {
    win32_file VertexShaderFile = Win32FileRead("standard.vs");
    win32_file PixelShaderFile = Win32FileRead("standard.ps");

    D3D12_SHADER_BYTECODE VertexShader;
    VertexShader.pShaderBytecode = VertexShaderFile.Memory;
    VertexShader.BytecodeLength = VertexShaderFile.Size;

    D3D12_SHADER_BYTECODE PixelShader;
    PixelShader.pShaderBytecode = PixelShaderFile.Memory;
    PixelShader.BytecodeLength = PixelShaderFile.Size;

    D3D12_INPUT_ELEMENT_DESC InputLayouts[2];
    InputLayouts[0].SemanticName = "POSITION";
    InputLayouts[0].SemanticIndex = 0;
    InputLayouts[0].Format = DXGI_FORMAT_R32G32_FLOAT;
    InputLayouts[0].InputSlot = 0;
    InputLayouts[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT ;
    InputLayouts[0].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
    InputLayouts[0].InstanceDataStepRate = 0;

    InputLayouts[1].SemanticName = "COLOR";
    InputLayouts[1].SemanticIndex = 0;
    InputLayouts[1].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    InputLayouts[1].InputSlot = 0;
    InputLayouts[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT ;
    InputLayouts[1].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
    InputLayouts[1].InstanceDataStepRate = 0;


    D3D12_ROOT_PARAMETER RootParameters[2];
    RootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    RootParameters[0].Constants.RegisterSpace = 0;
    RootParameters[0].Constants.ShaderRegister = 0;
    RootParameters[0].Constants.Num32BitValues = sizeof(dx_push_transform) / 4;
    RootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

    D3D12_DESCRIPTOR_RANGE SBRange;
    SBRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    SBRange.NumDescriptors = 1;
    SBRange.RegisterSpace = 0;
    SBRange.BaseShaderRegister = 0;
    SBRange.OffsetInDescriptorsFromTableStart = 0;

    RootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    RootParameters[1].DescriptorTable.NumDescriptorRanges = 1;
    RootParameters[1].DescriptorTable.pDescriptorRanges = &SBRange;
    RootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;


    D3D12_DESCRIPTOR_RANGE TRange;
    TRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    TRange.NumDescriptors = 1;
    TRange.RegisterSpace = 1;
    TRange.BaseShaderRegister = 0;
    TRange.OffsetInDescriptorsFromTableStart = 0;

#if 0
    RootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    RootParameters[2].DescriptorTable.NumDescriptorRanges = 1;
    RootParameters[2].DescriptorTable.pDescriptorRanges = &TRange;
    RootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_STATIC_SAMPLER_DESC Sampler = {};
    Sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
    Sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    Sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    Sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    Sampler.MipLODBias = 0;
    Sampler.MaxAnisotropy = 0;
    Sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    Sampler.MinLOD = 0.0f;
    Sampler.MaxLOD = D3D12_FLOAT32_MAX;
    Sampler.RegisterSpace = 1;
    Sampler.ShaderRegister = 0;
    Sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
#endif 

    D3D12_ROOT_SIGNATURE_DESC RootSignatureDesc = {};
    RootSignatureDesc.NumParameters = ArrayLength(RootParameters);
    RootSignatureDesc.pParameters = RootParameters;
#if 0
    RootSignatureDesc.NumStaticSamplers = 1;
    RootSignatureDesc.pStaticSamplers = &Sampler;
#endif
    RootSignatureDesc.Flags = 
      D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
      D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
      D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
      D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

    ID3DBlob *RootSignatureBlob = 0;
    ID3DBlob *ErrorBlob = 0;
    if(D3D12SerializeRootSignature(&RootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &RootSignatureBlob, &ErrorBlob) != S_OK)
    {
      OutputDebugStringA("Failed to serialize root signature");
      return -1;
    }
    if(DXContext.Device->CreateRootSignature(0, RootSignatureBlob->GetBufferPointer(), RootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&DXContext.RootSignature)) != S_OK)
    {
      OutputDebugStringA("Failed to create root signature");
      return -1;
    }

    struct 
    {
      dx_pipeline_state_subobject<ID3D12RootSignature *> RootSignature;
      dx_pipeline_state_subobject<D3D12_INPUT_LAYOUT_DESC> InputLayoutDesc;
      dx_pipeline_state_subobject<D3D12_SHADER_BYTECODE> PixelShader;
      dx_pipeline_state_subobject<D3D12_SHADER_BYTECODE> VertexShader;
      dx_pipeline_state_subobject<D3D12_RT_FORMAT_ARRAY> RTFormats;
      dx_pipeline_state_subobject<D3D12_PRIMITIVE_TOPOLOGY_TYPE> PrimitiveTopology;
      dx_pipeline_state_subobject<D3D12_BLEND_DESC> Blend;
    } PPS = {};

    PPS.RootSignature.Type = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE;
    PPS.RootSignature.Subobject = DXContext.RootSignature;

    PPS.InputLayoutDesc.Type = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_INPUT_LAYOUT;
    PPS.InputLayoutDesc.Subobject = D3D12_INPUT_LAYOUT_DESC{InputLayouts, ArrayLength(InputLayouts)};

    PPS.VertexShader.Type = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VS;
    PPS.VertexShader.Subobject = VertexShader;

    PPS.PixelShader.Type = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS;
    PPS.PixelShader.Subobject = PixelShader;

    PPS.RTFormats.Type = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RENDER_TARGET_FORMATS;
    PPS.RTFormats.Subobject.NumRenderTargets = 1;
    PPS.RTFormats.Subobject.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

    PPS.PrimitiveTopology.Type = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PRIMITIVE_TOPOLOGY;
    PPS.PrimitiveTopology.Subobject = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;

    PPS.Blend.Type = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_BLEND;
    PPS.Blend.Subobject.AlphaToCoverageEnable = false; 
    PPS.Blend.Subobject.IndependentBlendEnable = false;
    PPS.Blend.Subobject.RenderTarget[0].BlendEnable = true;
    PPS.Blend.Subobject.RenderTarget[0].LogicOpEnable = false;
    PPS.Blend.Subobject.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
    PPS.Blend.Subobject.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    PPS.Blend.Subobject.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    PPS.Blend.Subobject.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
    PPS.Blend.Subobject.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
    PPS.Blend.Subobject.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
    PPS.Blend.Subobject.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
    PPS.Blend.Subobject.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    D3D12_PIPELINE_STATE_STREAM_DESC PPSD; 
    PPSD.SizeInBytes = sizeof(PPS);
    PPSD.pPipelineStateSubobjectStream = &PPS;

    if(DXContext.Device->CreatePipelineState(&PPSD, IID_PPV_ARGS(&DXContext.PipelineState)) != S_OK)
    {
      OutputDebugStringA("Failed to pipeline state");
      return -1;
    }
  }

  LARGE_INTEGER Freq, LastFrame;
  QueryPerformanceFrequency(&Freq);
  QueryPerformanceCounter(&LastFrame);


  u64 DeltaTimeMs = 0;
  r32 DeltaTime = 0;
  
  b32 PlayerAlive = false;
  transform PlayerTransform = {};
  input PlayerInput = {};

#define LasersPerSecond 15
#define LaserLifetime 10
//#define LaserCountMax LasersPerSecond * LaserLifetime + 10
#define LaserCountMax 100
  laser Lasers[LaserCountMax] = {};
  u32 FirstLaser = 0;
  u32 LaserCount = 0;
  u64 LaserShootCooldownMs = 0;

  auto LaserGetIndex_ = [&](u32 Index) -> u32
  {
    u32 Result = FirstLaser + Index;
    if(Result >= ArrayLength(Lasers))
      Result -= ArrayLength(Lasers);

    return Result;
  };
  auto LaserGet = [&](u32 Index) -> laser *
  {
    laser *Result = Lasers + LaserGetIndex_(Index);
    return Result;
  };
  auto LaserPopFirst = [&]()
  {
    --LaserCount;
    if(++FirstLaser == ArrayLength(Lasers))
      FirstLaser = 0;
  };
  auto LaserPush = [&]() -> laser *
  {
    laser *Result;
    if(LaserCount == ArrayLength(Lasers))
    {
      Result = Lasers + FirstLaser++;
      if(FirstLaser == ArrayLength(Lasers))
      {
        FirstLaser = 0;
      }
    }
    else
    {
      Result = Lasers + LaserGetIndex_(LaserCount++);
    }

    return Result;
  };
  auto LaserPop = [&](u32 Index)
  {
    Index = LaserGetIndex_(Index);
    u32 LastLaser = LaserGetIndex_(LaserCount - 1);
    if(Index == FirstLaser)
    {
      LaserPopFirst();
    }
    else if(Index == LastLaser)
    {
      --LaserCount;
    }
    else if(FirstLaser < Index)
    {
      for(u32 CopyIndex = Index;
          CopyIndex > FirstLaser;
          --CopyIndex)
      {
        Lasers[CopyIndex] = Lasers[CopyIndex - 1];
      }
      LaserPopFirst();
    }
    else
    {
      --LaserCount;
      for(u32 CopyIndex = Index;
          CopyIndex < LastLaser;
          ++CopyIndex)
      {
        Lasers[CopyIndex] = Lasers[CopyIndex + 1];
      }
    }
  };

#define AsteroidSpawnCountMax 32
#define AsteroidCountMax (AsteroidSpawnCountMax << AsteroidType_Last)

  u64 AsteroidSpawnCooldownMs = 0;

  asteroid Asteroids[AsteroidCountMax] = {};
  u32 AsteroidCount = 0;

  auto AsteroidPush = [&Asteroids, &AsteroidCount]() -> asteroid *
  {
    asteroid *Result = Asteroids + AsteroidCount++;
    return Result;
  };
  auto AsteroidPop = [&Asteroids, &AsteroidCount](u32 Index)
  {
    Asteroids[Index] = Asteroids[--AsteroidCount];
  };

  auto ResetGame = [&]()
  {
    PlayerAlive = true;
    PlayerTransform = {};
    PlayerInput = {};
    FirstLaser = LaserCount = 0;
    LaserShootCooldownMs = 0;
    AsteroidCount = 0;
    AsteroidSpawnCooldownMs = 0;
  };
  ResetGame();

  u32 Generator_ = 1;
  auto RandomU32 = [&Generator_](u32 Min, u32 Max)
  {
    Generator_ += 0xe120fc15;

    u64 Tmp;
    Tmp = (u64)Generator_ * 0x4a39b70d;
    u32 M1 = (u32)((Tmp >> 32) ^ Tmp);
    Tmp = (u64)M1 * 0x12fad5c9;
    u32 M2 = (u32)((Tmp >> 32) ^ Tmp);

    u32 Result = M2 % (Max - Min) + Min;
    return Result;
  };
  auto RandomR32 = [&RandomU32]()
  {
    r32 Result = (r32)RandomU32(0, U32Max) / (r32)U32Max;
    return Result;
  };
  
  sound_loaded LoadedSounds[GameSound_Count];

  LoadedSounds[GameSound_Music]= SoundLoad("wave.wav");
  LoadedSounds[GameSound_Laser] = SoundLoad("laser.wav");
  LoadedSounds[GameSound_Explosion] = SoundLoad("explosion.wav");

  struct sound_played
  {
    sound_played *Prev;
    sound_played *Next;
    sound_loaded *Loaded;
  
    u32 SamplesPlayed;
    b32 Loop;
    r32 Volume;
  };

  sound_played SoundPlayedBuffer_[64] = {};
  sound_played *FirstFreeSoundPlayed =  SoundPlayedBuffer_;
  for(u32 SoundPlayedIndex = 0;
      SoundPlayedIndex < ArrayLength(SoundPlayedBuffer_) - 1;
      ++SoundPlayedIndex)
  {
    SoundPlayedBuffer_[SoundPlayedIndex].Next = SoundPlayedBuffer_ + SoundPlayedIndex + 1;
  }
  sound_played *FirstSoundPlayed = 0;

  sound_played *FirstNewSoundPlayed = 0;
  sound_played *LastNewSoundPlayed = 0;
  
  auto SoundPlay_ = [&](game_sound Sound, r32 Volume, b32 Loop)
  {
    sound_played *SoundPlayed;
    if(FirstFreeSoundPlayed)
    {
      SoundPlayed = FirstFreeSoundPlayed;
      FirstFreeSoundPlayed = FirstFreeSoundPlayed->Next;
    }
    else
    {
      SoundPlayed = FirstSoundPlayed;
      sound_played *PrevSoundPlayed = 0;
      while(SoundPlayed->Next)
      {
        PrevSoundPlayed = SoundPlayed;
        SoundPlayed = SoundPlayed->Next;
      }

      PrevSoundPlayed->Next = 0;
    }
    
    if(!FirstNewSoundPlayed)
    {
      FirstNewSoundPlayed = LastNewSoundPlayed = SoundPlayed;
      SoundPlayed->Prev = SoundPlayed->Next = 0;
    }
    else
    {
      LastNewSoundPlayed->Next = SoundPlayed;
      SoundPlayed->Prev = LastNewSoundPlayed;
      SoundPlayed->Next = 0;
      LastNewSoundPlayed = SoundPlayed;
    }

    SoundPlayed->Loaded = LoadedSounds + Sound;
    SoundPlayed->SamplesPlayed = 0;
    SoundPlayed->Loop = Loop;
    SoundPlayed->Volume = Volume;
  };
  auto SoundPlay = [&](game_sound Sound, r32 Volume, b32 Loop = false)
  {
    SoundPlay_(Sound, Volume, Loop);
  };
  
  r32 PlaySize = (r32)Min(DXContext.SwapChainWidth, DXContext.SwapChainHeight) / 30.0f;
  v2 PlayAreaHalf = (0.5f / PlaySize) * V2U32(DXContext.SwapChainWidth, DXContext.SwapChainHeight);
  v2 AsteroidAreaHalf = PlayAreaHalf + V2(0.5f * (r32)AsteroidSize(AsteroidType_Last));
  
  //SoundPlay(GameSound_Music, true);
  while(WindowMain.KeepRunning)
  {
#if 0
    if(AtomicExchangeU32(&DXContext.SwapChainIsOld, 0))
    {
      for(u32 FrameIndex = 0; 
          FrameIndex < SwapchainFrameCount; 
          ++FrameIndex)
      {
        dx_command_buffer *CommandBuffer = DXContext.CommandBuffers + FrameIndex;
        DXCommandBufferFenceSignal(CommandBuffer, DXContext.CommandQueue);
        DXCommandBufferFenceWait(CommandBuffer);

        DXContext.BackBuffers[FrameIndex]->Release();
      }
      DXContext.FrameIndex = 0;


      IDXGISwapChain4 *SwapChain = DXContext.SwapChain;
      DXGI_SWAP_CHAIN_DESC SwapChainDesc;
      SwapChain->GetDesc(&SwapChainDesc);

      HRESULT ResizeResult = SwapChain->ResizeBuffers(0, DXContext.SwapChainWidth, DXContext.SwapChainHeight, DXGI_FORMAT_UNKNOWN, SwapChainDesc.Flags);
      if(ResizeResult != S_OK)
      {
        OutputDebugStringA("[ERROR]: Failed to resize swapchain"); 
        break;
      }
      if(!DXContextCreateRTVs(&DXContext))
      {
        OutputDebugStringA("Failed to create RTVs");
        break;
      }
    }
#endif

    MSG Message;

    b32 AddBodyPart = false;
    while(PeekMessageW(&Message, WindowMain.Handle, 0, 0, PM_REMOVE))
    { 
      switch(Message.message)
      {
        case WM_KEYUP:
          {
            u32 Keycode = (u32)Message.wParam;

            switch(Keycode)
            {
              case VK_LEFT:
              case 'A':
                {
                  PlayerInput.LeftPressed = false;
                } break;
              case VK_RIGHT:
              case 'D':
                {
                  PlayerInput.RightPressed = false;
                } break;
              case VK_DOWN:
              case 'S':
                {
                  PlayerInput.DownPressed = false;
                } break;
              case VK_UP:
              case 'W':
                {
                  PlayerInput.UpPressed = false;
                } break;
              case VK_SPACE:
                {
                  PlayerInput.SpacePressed = false;
                } break;
            }
          } break;
        case WM_KEYDOWN:
          {
            u32 Keycode = (u32)Message.wParam;
            switch(Keycode)
            {
              case VK_LEFT:
              case 'A':
                {
                  PlayerInput.LeftPressed = true;
                } break;
              case 'D':
              case VK_RIGHT:
                {
                  PlayerInput.RightPressed = true;
                } break;
              case VK_DOWN:
              case 'S':
                {
                  PlayerInput.DownPressed = true;
                } break;
              case VK_UP:
              case 'W':
                {
                  PlayerInput.UpPressed = true;
                } break;
              case VK_SPACE:
                {
                  PlayerInput.SpacePressed = true;
                } break;
              case 'R':
                {
                  ResetGame();
                } break;
            }
          };
      }
      TranslateMessage(&Message);
      DispatchMessage(&Message);
    }

    auto UpdateCooldown = [&DeltaTimeMs](u64 *Cooldown) ->b32
    {
      b32 Result;
      if(*Cooldown > DeltaTimeMs)
      {
        *Cooldown -= DeltaTimeMs;
        Result = false;
      }
      else
      {
        Result = true;
        *Cooldown = 0;
      }

      return Result;
    };

    if(PlayerAlive)
    {
      if(PlayerTransform.Position.X < -PlayAreaHalf.X)
      {
        PlayerTransform.Position.X = -PlayAreaHalf.X;
        PlayerTransform.DeltaPosition.X = 0;
      }
      else if (PlayerTransform.Position.X > PlayAreaHalf.X)
      {
        PlayerTransform.Position.X = PlayAreaHalf.X;
        PlayerTransform.DeltaPosition.X = 0;
      }
      if(PlayerTransform.Position.Y < -PlayAreaHalf.Y)
      {
        PlayerTransform.Position.Y = -PlayAreaHalf.Y;
        PlayerTransform.DeltaPosition.Y = 0;
      }
      else if (PlayerTransform.Position.Y > PlayAreaHalf.Y)
      {
        PlayerTransform.Position.Y = PlayAreaHalf.Y;
        PlayerTransform.DeltaPosition.Y = 0;
      }

      r32 DeltaRotation = DeltaTime * 8 * Pi32;
      r32 DeltaRotationMax = 1.5f * Pi32;
      if(PlayerInput.LeftPressed)
      {
        PlayerTransform.DeltaRotation = Min(PlayerTransform.DeltaRotation + DeltaRotation, DeltaRotationMax);
      }
      if(PlayerInput.RightPressed)
      {
        PlayerTransform.DeltaRotation = Max(PlayerTransform.DeltaRotation - DeltaRotation, -DeltaRotationMax);
      }


      v2 PlayerDirection = V2(Cos(PlayerTransform.Rotation), Sin(PlayerTransform.Rotation));
      v2 DeltaPosition = DeltaTime * 20 * PlayerDirection;
      if(PlayerInput.UpPressed)
      {
        PlayerTransform.DeltaPosition += DeltaPosition;
      }
      if(PlayerInput.DownPressed)
      {
        PlayerTransform.DeltaPosition -= DeltaPosition;
      }

      r32 DeltaPositionLengthMax = 10;
      r32 DeltaPositionLength = Length(PlayerTransform.DeltaPosition);
      if(DeltaPositionLength > DeltaPositionLengthMax)
      {
        r32 InvDeltaPositionLength = 1.0f / DeltaPositionLength;
        PlayerTransform.DeltaPosition *= InvDeltaPositionLength * DeltaPositionLengthMax;
      }

      u32 LaserShootInterval = 1000 / LasersPerSecond;
      if(UpdateCooldown(&LaserShootCooldownMs))
      {
        if(PlayerInput.SpacePressed)
        {
          LaserShootCooldownMs = LaserShootInterval;

          laser *Laser = LaserPush();
          Laser->Transform = PlayerTransform;
          Laser->Transform.Position += 0.5f * PlayerDirection;
          Laser->Transform.DeltaPosition += 14.0f * PlayerDirection;
          Laser->Transform.DeltaRotation = 0;
          Laser->LifetimeMs = 1000 * LaserLifetime;

          SoundPlay(GameSound_Laser, 0.05f); 
        }
      }
    }

    for(u32 AsteroidIndex = 0;
        AsteroidIndex < AsteroidCount;)
    {
      asteroid *Asteroid = Asteroids + AsteroidIndex;
      
      if(Asteroid->Transform.Position.X < -AsteroidAreaHalf.X || Asteroid->Transform.Position.X > AsteroidAreaHalf.X || 
          Asteroid->Transform.Position.Y < -AsteroidAreaHalf.Y || Asteroid->Transform.Position.Y > AsteroidAreaHalf.Y)
      {
        Asteroids[AsteroidIndex] = Asteroids[--AsteroidCount];
      }
      else
      {
        vertex *Vertices = AsteroidVertices[Asteroid->Type];

        auto IntersectedAsteroid = [&Asteroid, &Vertices](v2 LineStart, v2 LineEnd) -> b32
        {
          b32 Result = false;

          u32 AsteroidSideCount = ArrayLength(AsteroidVertices[0]);
          for(u32 AsteroidSide = 0;
              AsteroidSide < AsteroidSideCount;
              ++AsteroidSide)
          {
            u32 FirstVertex = AsteroidSide;
            u32 SecondVertex = AsteroidSide + 1 == AsteroidSideCount ? 0 : AsteroidSide + 1;

            v2 AsteroidStart = Vertices[FirstVertex].Position;
            v2 AsteroidEnd = Vertices[SecondVertex].Position;

            if(IntersectedLineLine(AsteroidStart, AsteroidEnd, LineStart, LineEnd))
            {
              Result = true;
              break;
            }
          }

          return Result;
        };

        b32 NoHit = true;

        if(NoHit)
        {
          for(u32 LaserIndex = 0;
              LaserIndex < LaserCount;
              ++LaserIndex)
          {
            laser *Laser =  LaserGet(LaserIndex);
            r32 Rotation = Laser->Transform.Rotation - Asteroid->Transform.Rotation ;

            mat2x2 RotationMatrix = Mat2x2Rotation(Rotation);

            v2 LaserPosition = Laser->Transform.Position - Asteroid->Transform.Position;
            v2 LaserStart = RotationMatrix * LaserVertices[0].Position + LaserPosition;
            v2 LaserEnd = RotationMatrix * LaserVertices[1].Position + LaserPosition;

            if(IntersectedAsteroid(LaserStart, LaserEnd))
            {
              SoundPlay(GameSound_Explosion, 0.2f); 

              LaserPop(LaserIndex);
              NoHit = false;

              break;
            }
          }
        }

        if(NoHit)
        {
          if(PlayerAlive)
          {
            r32 DistanceSquared = Inner(PlayerTransform.Position - Asteroid->Transform.Position);
            r32 MinDistanceSquared = Square(1.0f + 2.0f * AsteroidSize(Asteroid->Type));
            if(DistanceSquared <= MinDistanceSquared)
            {
              r32 Rotation = PlayerTransform.Rotation - Asteroid->Transform.Rotation ;
              mat2x2 RotationMatrix = Mat2x2Rotation(Rotation);

              u32 PlayerSideCount = ArrayLength(PlayerVertices);
              for(u32 PlayerSide = 0;
                  PlayerSide < PlayerSideCount;
                  ++PlayerSide)
              {
                u32 PlayerFirstVertex = PlayerSide;
                u32 PlayerSecondVertex = PlayerSide + 1 == PlayerSideCount ? 0 : PlayerSide + 1;

                v2 PlayerPosition = PlayerTransform.Position - Asteroid->Transform.Position;
                v2 PlayerStart = RotationMatrix * PlayerVertices[PlayerFirstVertex].Position + PlayerPosition;
                v2 PlayerEnd = RotationMatrix * PlayerVertices[PlayerSecondVertex].Position + PlayerPosition;

                if(IntersectedAsteroid(PlayerStart, PlayerEnd))
                {
                  SoundPlay(GameSound_Explosion, 0.3f);

                  NoHit = false;
                  PlayerAlive = false;
                  break;
                }
              }
            }
          }
        }

        //#error Make asteroid split into two and collide with player and boundaries and despawn and sound
        if(NoHit)
        {
          ++AsteroidIndex;
        }
        else
        {
          if(Asteroid->Type == AsteroidType_Small)
          {
            Asteroids[AsteroidIndex] = Asteroids[--AsteroidCount];
          }
          else
          {
            Asteroid->Type = (asteroid_type)(Asteroid->Type - 1);

            asteroid *Split = AsteroidPush(); 
            *Split = *Asteroid;

            v2 OldDeltaPosition = Asteroid->Transform.DeltaPosition;

            auto RandomBias = [&RandomR32]() -> r32
            {
              r32 Result = 0.75f + RandomR32() / 4.0f;
              return Result;
            };
            v2 SplitDirection = V2(-OldDeltaPosition.Y, OldDeltaPosition.X);
            Asteroid->Transform.DeltaPosition = 0.5f * Lerp(RandomBias(), SplitDirection, OldDeltaPosition); 
            Split->Transform.DeltaPosition = 0.5f * Lerp(RandomBias(), -SplitDirection, OldDeltaPosition); 

            ++AsteroidIndex;
          }
        }
      }
    }

    
    if(UpdateCooldown(&AsteroidSpawnCooldownMs))
    {
      AsteroidSpawnCooldownMs = 250;
      if(AsteroidCount < AsteroidSpawnCountMax)
      {
        asteroid *Asteroid = AsteroidPush();
        Asteroid->Type = (asteroid_type)RandomU32(AsteroidType_First, AsteroidType_Last + 1);
        
        Asteroid->Transform.Position = {};
        
        u32 RandomTransform = RandomU32(0, 3);
        if(RandomTransform < 2)
        {
          Asteroid->Transform.Position.Y = (2.0f * RandomR32() - 1.0f) * AsteroidAreaHalf.Y;

          r32 BiasY = (Asteroid->Transform.Position.Y / AsteroidAreaHalf.Y) / 4.0f;
          Asteroid->Transform.DeltaPosition.Y = -BiasY;

          if(RandomTransform == 0)
          {
            Asteroid->Transform.Position.X = -AsteroidAreaHalf.X;
            Asteroid->Transform.DeltaPosition.X = (1.0f - BiasY);
          }
          else
          {
            Asteroid->Transform.Position.X = AsteroidAreaHalf.X;
            Asteroid->Transform.DeltaPosition.X = -(1.0f - BiasY);
          }
        }
        else
        {
          Asteroid->Transform.Position.X = (2.0f * RandomR32() - 1.0f) * AsteroidAreaHalf.X;

          r32 BiasX = (Asteroid->Transform.Position.X / AsteroidAreaHalf.X) / 2.0f + RandomR32() / 2.0f;
          Asteroid->Transform.DeltaPosition.X = -BiasX;

          if(RandomTransform == 2)
          {
            Asteroid->Transform.Position.Y = -AsteroidAreaHalf.Y;
            Asteroid->Transform.DeltaPosition.Y = (1.0f - BiasX);
          }
          else
          {
            Asteroid->Transform.Position.Y = AsteroidAreaHalf.Y;
            Asteroid->Transform.DeltaPosition.Y = -(1.0f - BiasX);
          }
        }

        r32 SpeedMax = 8.0f;
        r32 SpeedMin = 3.0f;
        r32 Speed = SpeedMin + RandomR32() * (SpeedMax - SpeedMin);
        Asteroid->Transform.DeltaPosition *= Speed;
        
        r32 RandomRotation = 2.0f * RandomR32() - 1.0f;
        Asteroid->Transform.DeltaRotation = 2 * Pi32 * (0.1f + 0.4f * RandomRotation);

        Asteroid->Transform.Rotation = Pi32 / 4; 
      }
    }
    
    {
      for(u32 LaserIndex = 0;
          LaserIndex < LaserCount;
          ++LaserIndex)
      {
        if(LaserGet(LaserIndex)->LifetimeMs <= DeltaTimeMs)
        {
          LaserPopFirst();
        }
        else
        {
          break;
        }
      }
      for(u32 LaserIndex = 0;
          LaserIndex < LaserCount;
          ++LaserIndex)
      {
        LaserGet(LaserIndex)->LifetimeMs -= (u32)DeltaTimeMs;
      }
    }

// AUDIO
    {
      s16 *AudioSamples;
      u32 AudioFramesPlayedCount;
      Win32AudioGetBuffer(&Audio, &AudioSamples, &AudioFramesPlayedCount);

      {
        for(sound_played *SoundPlayed = FirstSoundPlayed;
            SoundPlayed;)
        {
          SoundPlayed->SamplesPlayed += AudioFramesPlayedCount;
          if(SoundPlayed->SamplesPlayed >= SoundPlayed->Loaded->SampleCount)
          {
            if(SoundPlayed->Loop)
            {
              SoundPlayed->SamplesPlayed %= SoundPlayed->Loaded->SampleCount;
              SoundPlayed = SoundPlayed->Next;
            }
            else
            {
              auto FreeSoundPlayed = [&](sound_played *Free)
              {
                Free->Next = FirstFreeSoundPlayed;
                FirstFreeSoundPlayed = Free;
              };

              if(SoundPlayed == FirstSoundPlayed)
              {
                FirstSoundPlayed = FirstSoundPlayed->Next;

                sound_played *Free = SoundPlayed;
                SoundPlayed = SoundPlayed->Next;
                FreeSoundPlayed(Free);

                if(SoundPlayed)
                {
                  SoundPlayed->Prev = 0;
                }
              }
              else
              {
                sound_played *Prev = SoundPlayed->Prev;
                sound_played *Next = SoundPlayed->Next;

                FreeSoundPlayed(SoundPlayed);
                SoundPlayed = Next;

                Prev->Next = Next;
                if(Next)
                {
                  Next->Prev = Prev;
                }
              }
            }
          }
          else
          {
            SoundPlayed = SoundPlayed->Next;
          }
        }
      }

      if(FirstNewSoundPlayed)
      {
        if(FirstSoundPlayed)
        {
          FirstSoundPlayed->Prev = LastNewSoundPlayed;
        }

        LastNewSoundPlayed->Next = FirstSoundPlayed;
        FirstSoundPlayed = FirstNewSoundPlayed;
        FirstNewSoundPlayed = LastNewSoundPlayed = 0;
      }

      for(sound_played *SoundPlayed = FirstSoundPlayed;
          SoundPlayed;)
      {
        u32 RemainingSampleCount = SoundPlayed->Loaded->SampleCount - SoundPlayed->SamplesPlayed;
        u32 SampleCount = Min(RemainingSampleCount, Audio.BufferFrameCount);
        
        s16 *DstSample = AudioSamples;
        {
          s16 *SrcSample = SoundPlayed->Loaded->Samples + SoundPlayed->SamplesPlayed;
          for(u32 SampleIndex = 0;
              SampleIndex < SampleCount;
              ++SampleIndex)
          {
            s16 Value = (s16)(SoundPlayed->Volume * (*SrcSample));
            DstSample[0] += Value;
            DstSample[1] += Value;

            ++SrcSample;
            DstSample += 2;
          }
        }
        
        if(RemainingSampleCount < Audio.BufferFrameCount && SoundPlayed->Loop)
        {
          u32 LoopSampleCount = RemainingSampleCount - SampleCount;
          s16 *SrcSample = SoundPlayed->Loaded->Samples;
          for(u32 SampleIndex = 0;
              SampleIndex < LoopSampleCount;
              ++SampleIndex)
          {
            s16 Value = (s16)(SoundPlayed->Volume * (*SrcSample));
            DstSample[0] += Value;
            DstSample[1] += Value;

            ++SrcSample;
            DstSample += 2;
          }
        }

        SoundPlayed = SoundPlayed->Next;
      }

      Win32AudioSwapBuffers(&Audio);
    }
// GRAPHICS

    u32 FrameIndex = DXContext.FrameIndex;
    dx_command_buffer *CommandBuffer = DXContext.CommandBuffers + FrameIndex;
    if(!DXCommandBufferFenceWait(CommandBuffer))
    {
      OutputDebugStringA("[ERROR]: Command buffer was unable to complete!"); 
      DestroyWindow(WindowMain.Handle);
    }
    DXCommandBufferReset(CommandBuffer);

    ID3D12Resource *BackBuffer = DXContext.BackBuffers[FrameIndex];
    // Clear the render target.
    {
      D3D12_RESOURCE_BARRIER Barrier = {};
      Barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
      Barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
      Barrier.Transition.pResource = BackBuffer;
      Barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
      Barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
      Barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

      CommandBuffer->List->ResourceBarrier(1, &Barrier);


      dx_descriptor_heap *RTVDescriptorHeap = &DXContext.RTVDescriptorHeap;
      D3D12_CPU_DESCRIPTOR_HANDLE RTV = RTVDescriptorHeap->Heap->GetCPUDescriptorHandleForHeapStart();
      RTV.ptr += FrameIndex * RTVDescriptorHeap->Size;

      {
        v3 DarkGrey = ColorRGB(0x29, 0x29, 0x29);
        FLOAT ClearColor[] = { DarkGrey.R, DarkGrey.G, DarkGrey.B, 0.0f };
        CommandBuffer->List->ClearRenderTargetView(RTV, ClearColor, 0, 0);
      }

      u32 ClearCenterX = DXContext.SwapChainWidth / 2;
      u32 ClearCenterY = DXContext.SwapChainHeight / 2;

      D3D12_VIEWPORT Viewport = {};
      Viewport.Width = (r32)DXContext.SwapChainWidth;
      Viewport.Height = (r32)DXContext.SwapChainHeight;
      Viewport.MaxDepth = 1.0f;
      CommandBuffer->List->RSSetViewports(1, &Viewport);

      D3D12_RECT Scissors = {};
      Scissors.right = DXContext.SwapChainWidth;
      Scissors.bottom = DXContext.SwapChainHeight;
      CommandBuffer->List->RSSetScissorRects(1, &Scissors);

      CommandBuffer->List->OMSetRenderTargets(1, &RTV, 0, 0);

      CommandBuffer->List->SetPipelineState(DXContext.PipelineState);
      CommandBuffer->List->SetGraphicsRootSignature(DXContext.RootSignature);

      CommandBuffer->List->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);

      CommandBuffer->List->IASetVertexBuffers(0, 1, &DXContext.VertexBufferView);
      CommandBuffer->List->IASetIndexBuffer(&DXContext.IndexBufferView);

#if 0
      CommandBuffer->List->SetDescriptorHeaps(1, &DXContext.SRVDescriptorHeap.Heap);
      D3D12_GPU_DESCRIPTOR_HANDLE SBHandle = DXContext.SRVDescriptorHeap.Heap->GetGPUDescriptorHandleForHeapStart();
      SBHandle.ptr += FrameIndex * DXContext.SRVDescriptorHeap.Size;
      CommandBuffer->List->SetGraphicsRootDescriptorTable(1, SBHandle);

      D3D12_GPU_DESCRIPTOR_HANDLE TextureHandle = DXContext.SRVDescriptorHeap.Heap->GetGPUDescriptorHandleForHeapStart();
      TextureHandle.ptr += SwapchainFrameCount * DXContext.SRVDescriptorHeap.Size;
      CommandBuffer->List->SetGraphicsRootDescriptorTable(2, TextureHandle);
#endif

      /*
       */
      v2 Scale = PlaySize * V2(2.0f / (r32)DXContext.SwapChainWidth, 2.0f / (r32)DXContext.SwapChainHeight);
      u32 ScaleOffset = offsetof(dx_push_transform, Scale);
      CommandBuffer->List->SetGraphicsRoot32BitConstants(0, sizeof(Scale) / 4, &Scale, ScaleOffset / 4);

      auto UpdateAndPush = [&DeltaTime, &CommandBuffer](transform *Transform, u32 IndicesCount, u32 FirstIndex, u32 FirstVertex)
      {
        Transform->Position += DeltaTime * Transform->DeltaPosition;
        Transform->Rotation += DeltaTime * Transform->DeltaRotation;

        u32 PositionOffset = offsetof(dx_push_transform, Position);
        CommandBuffer->List->SetGraphicsRoot32BitConstants(0, sizeof(Transform->Position) / 4, &Transform->Position, PositionOffset / 4);

        mat2x2 Rotation2D = Mat2x2Rotation(Transform->Rotation);
        mat4x4 Rotation = Mat4x4Identity();
        Rotation.X.XY = Rotation2D.X;
        Rotation.Y.XY = Rotation2D.Y;

        u32 RotationOffset = offsetof(dx_push_transform, Rotation);
        CommandBuffer->List->SetGraphicsRoot32BitConstants(0, sizeof(Rotation) / 4, &Rotation, RotationOffset / 4);
        CommandBuffer->List->DrawIndexedInstanced(IndicesCount, 1, FirstIndex, FirstVertex, 0);
      };

      if(PlayerAlive)
      {
        UpdateAndPush(&PlayerTransform, ArrayLength(PlayerIndices), 0, 0);
        PlayerTransform.DeltaRotation -= 4 * DeltaTime * PlayerTransform.DeltaRotation;
        if(Abs(PlayerTransform.DeltaRotation) < Pi32 / 24)
        {
          PlayerTransform.DeltaRotation = 0;
        }
        PlayerTransform.DeltaPosition -= 1.5f * DeltaTime * PlayerTransform.DeltaPosition;
      }

      u32 AsteroidFirstIndex = ArrayLength(PlayerIndices);
      u32 AsteroidFirstVertex = ArrayLength(PlayerVertices);

      for(u32 AsteroidIndex = 0;
          AsteroidIndex < AsteroidCount;
          ++AsteroidIndex)
      {
        asteroid *Asteroid = Asteroids + AsteroidIndex;

        u32 FirstIndex = AsteroidFirstIndex;
        u32 FirstVertex = AsteroidFirstVertex + Asteroid->Type * ArrayLength(AsteroidVertices[0]);
        UpdateAndPush(&Asteroid->Transform, ArrayLength(AsteroidIndices), FirstIndex, FirstVertex);
      }

      u32 LaserFirstIndex = AsteroidFirstIndex + ArrayLength(AsteroidIndices);
      u32 LaserFirstVertex = AsteroidFirstVertex + ArrayLength(AsteroidVertices) * ArrayLength(AsteroidVertices[0]);


      for(u32 LaserIndex = 0;
          LaserIndex < LaserCount;
          ++LaserIndex)
      {
        UpdateAndPush(&LaserGet(LaserIndex)->Transform, ArrayLength(LaserIndices), LaserFirstIndex, LaserFirstVertex);
      }
    }

    // Present
    {
      D3D12_RESOURCE_BARRIER Barrier = {};
      Barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
      Barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
      Barrier.Transition.pResource = BackBuffer;
      Barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
      Barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
      Barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
      CommandBuffer->List->ResourceBarrier(1, &Barrier);

      DXContext.CommandQueue->Signal(CommandBuffer->Fence, 0);
      if(!DXCommandBufferExecute(CommandBuffer, DXContext.CommandQueue))
      {
        OutputDebugStringA("[ERROR]: Failed to execute command list"); 
        break; 
      }
      else
      {
        if(DXContext.SwapChain->Present(1, 0) != S_OK)
        {
          OutputDebugStringA("[ERROR]: Failed to present\n"); 
          break; 
        }
      }

      u32 NextFrameIndex = FrameIndex + 1;
      if(NextFrameIndex >= SwapchainFrameCount)
      {
        NextFrameIndex = 0;
      }
      DXContext.FrameIndex = NextFrameIndex;
    }

    LARGE_INTEGER CurrentFrame;
    QueryPerformanceCounter(&CurrentFrame);

#if 1
    DeltaTime = (r32)(CurrentFrame.QuadPart - LastFrame.QuadPart) / (r32)Freq.QuadPart;
    DeltaTimeMs = (1000 * (CurrentFrame.QuadPart - LastFrame.QuadPart)) / Freq.QuadPart;
    #else
    DeltaTime = 1.0f / 60.0f;
    DeltaTimeMs = 1000 / 60;
    #endif
    LastFrame = CurrentFrame;
  }

  return 0;
}
