#include "d3d12.h"
#include "dxgi1_6.h"

struct dx_command_buffer
{
  ID3D12CommandAllocator *Allocator;
  ID3D12GraphicsCommandList *List;

  ID3D12Fence *Fence;
  HANDLE FenceEvent;
  u64 FenceWaitValue;
};

  void
DXCommandBufferReset(dx_command_buffer *CommandBuffer)
{
  CommandBuffer->Allocator->Reset();
  CommandBuffer->List->Reset(CommandBuffer->Allocator, 0);
}

  b32
DXCommandBufferFenceWait(dx_command_buffer *CommandBuffer)
{
  b32 Result = true;

  ID3D12Fence *FenceWait = CommandBuffer->Fence;
  if(FenceWait->GetCompletedValue() < CommandBuffer->FenceWaitValue)
  {
    HANDLE FenceEvent = CommandBuffer->FenceEvent;
    if(FenceWait->SetEventOnCompletion(CommandBuffer->FenceWaitValue, FenceEvent) != S_OK)
    {
      Result = false;
    }
    else
    {
      WaitForSingleObject(FenceEvent, INFINITE);
    }
  }

  return Result;
}

  b32
DXCommandBufferFenceSignal(dx_command_buffer *CommandBuffer, ID3D12CommandQueue *CommandQueue)
{
  b32 Result = false;
  if(CommandQueue->Signal(CommandBuffer->Fence, ++CommandBuffer->FenceWaitValue) == S_OK)
  {
    Result = true;
  }

  return Result;
}

  b32
DXCommandBufferExecute(dx_command_buffer *CommandBuffer, ID3D12CommandQueue *CommandQueue)
{
  b32 Result = false;

  if(CommandBuffer->List->Close() == S_OK)
  {
    ID3D12CommandList *ExecuteCommandLists[] = { CommandBuffer->List };
    CommandQueue->ExecuteCommandLists(ArrayLength(ExecuteCommandLists), ExecuteCommandLists);

    Result = DXCommandBufferFenceSignal(CommandBuffer, CommandQueue);
  }
  return Result;
}

struct dx_descriptor_heap
{
  ID3D12DescriptorHeap *Heap;
  u64 Size;
};

#define SwapchainFrameCount 2
struct dx_context
{ 
  ID3D12Device2 *Device;
  ID3D12CommandQueue *CommandQueue;

  b32 SwapChainIsOld;
  u32 SwapChainWidth, SwapChainHeight;
  IDXGISwapChain4 *SwapChain;

  dx_descriptor_heap RTVDescriptorHeap;

  ID3D12Resource *BackBuffers[SwapchainFrameCount];
  struct dx_command_buffer CommandBuffers[SwapchainFrameCount];

  ID3D12RootSignature *RootSignature;
  ID3D12PipelineState *PipelineState;

  D3D12_VERTEX_BUFFER_VIEW VertexBufferView; 
  D3D12_INDEX_BUFFER_VIEW IndexBufferView; 

#if 0
  dx_descriptor_heap SRVDescriptorHeap;
  struct dx_transform *BufferTransforms[SwapchainFrameCount];
  u32 TransformCountMax;
#endif
  u32 FrameIndex;
}; 


IDXGIFactory4 *
DXCreateFactory4()
{
  IDXGIFactory4 *Result;
  UINT Flags = SNAKE_DEBUG ? DXGI_CREATE_FACTORY_DEBUG : 0;
  CreateDXGIFactory2(Flags, IID_PPV_ARGS(&Result));

  return Result;
}

  b32 
DXContextCreateRTVs(dx_context *DXContext)
{
  b32 Result = true;

  D3D12_CPU_DESCRIPTOR_HANDLE RTVHandle = DXContext->RTVDescriptorHeap.Heap->GetCPUDescriptorHandleForHeapStart();

  for(u32 FrameIndex = 0; 
      FrameIndex < SwapchainFrameCount; 
      ++FrameIndex)
  {
    ID3D12Resource *BackBuffer;
    if(DXContext->SwapChain->GetBuffer(FrameIndex, IID_PPV_ARGS(&BackBuffer)) != S_OK)
    {
      Result = false;
      break;
    }
    else
    {
      DXContext->Device->CreateRenderTargetView(BackBuffer, 0, RTVHandle);
      DXContext->BackBuffers[FrameIndex] = BackBuffer;

      RTVHandle.ptr += DXContext->RTVDescriptorHeap.Size;
    }
  }

  return Result;
}

b32
DXDescriptorHeapCreate(dx_context *DXContext, u32 Count, D3D12_DESCRIPTOR_HEAP_TYPE Type, D3D12_DESCRIPTOR_HEAP_FLAGS Flags, dx_descriptor_heap *DescriptorHeap)
{
  b32 Result = false;

  D3D12_DESCRIPTOR_HEAP_DESC DescHeapDesc = {};
  DescHeapDesc.NumDescriptors = Count;
  DescHeapDesc.Type = Type;
  DescHeapDesc.Flags = Flags;

  if(DXContext->Device->CreateDescriptorHeap(&DescHeapDesc, IID_PPV_ARGS(&DescriptorHeap->Heap)) == S_OK)
  {
    Result = true;
    DescriptorHeap->Size = DXContext->Device->GetDescriptorHandleIncrementSize(Type);
  }

  return Result;
}

  ID3D12Resource *
DXCreateBuffer(dx_context *DXContext, D3D12_HEAP_TYPE Type, D3D12_RESOURCE_STATES InitialState, u64 Size)
{
  D3D12_HEAP_PROPERTIES HeapProperties = {};
  HeapProperties.Type = Type;
  HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
  HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
  HeapProperties.CreationNodeMask = 0;
  HeapProperties.VisibleNodeMask = 0;

  D3D12_RESOURCE_DESC BufferDesc = {};
  BufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  BufferDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
  BufferDesc.Width = Size;
  BufferDesc.Height = 1;
  BufferDesc.DepthOrArraySize = 1;
  BufferDesc.MipLevels = 1;
  BufferDesc.Format = DXGI_FORMAT_UNKNOWN;
  BufferDesc.SampleDesc.Count = 1;
  BufferDesc.SampleDesc.Quality = 0;
  BufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  BufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

  ID3D12Resource *Result;
  DXContext->Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAG_NONE, &BufferDesc, InitialState, 0, IID_PPV_ARGS(&Result));

  return Result;
}

  ID3D12Resource *
DXCreateTexture(dx_context *DXContext, D3D12_HEAP_TYPE Type, D3D12_RESOURCE_STATES InitialState, u32 Width, u32 Height)
{
  D3D12_HEAP_PROPERTIES HeapProperties = {};
  HeapProperties.Type = Type;
  HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
  HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
  HeapProperties.CreationNodeMask = 0;
  HeapProperties.VisibleNodeMask = 0;

  D3D12_RESOURCE_DESC TextureDesc = {};
  TextureDesc.MipLevels = 1;
  TextureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  TextureDesc.Width = Width;
  TextureDesc.Height = Height;
  TextureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
  TextureDesc.DepthOrArraySize = 1;
  TextureDesc.SampleDesc.Count = 1;
  TextureDesc.SampleDesc.Quality = 0;
  TextureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

  ID3D12Resource *Result;
  DXContext->Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAG_NONE, &TextureDesc, InitialState, 0, IID_PPV_ARGS(&Result));

  return Result;
}
