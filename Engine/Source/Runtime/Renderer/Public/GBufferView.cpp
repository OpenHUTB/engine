// Copyright Epic Games, Inc. All Rights Reserved.
// Copyright (c) 2022 Computer Vision Center (CVC) at the Universitat Autonoma
// de Barcelona (UAB).

#include "GBufferView.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "SceneRendering.h"
#include <type_traits>
#include <thread>



DECLARE_LOG_CATEGORY_EXTERN(LogRenderer, Log, All);

FViewInfo* FGBufferRequest::InitAndFindSceneView(TArrayView<FViewInfo> Views)
{
  // Find from which view we are drawing and determine the size of the GBuffer textures.
  FViewInfo* SourceView = nullptr;
  for (auto& View : Views)
    if (View.ViewActor == OwningActor)
      SourceView = &View;
  check(SourceView != nullptr);
  ViewRect = SourceView->ViewRect;
  return SourceView;
}

void FGBufferRequest::MarkAsRequested(EGBufferTextureID TextureID)
{
  auto Index = ((std::underlying_type_t<EGBufferTextureID>)TextureID);
  DesiredTexturesMask |= UINT64_C(1) << Index;
  Readbacks[Index] = MakeUnique<FRHIGPUTextureReadback>(TEXT("GBUFFER READBACK"));
}

bool FGBufferRequest::IsRequested(EGBufferTextureID TextureID) const
{
  auto Mask = UINT64_C(1) << ((std::underlying_type_t<EGBufferTextureID>)TextureID);
  return (DesiredTexturesMask & Mask) != 0;
}

bool FGBufferRequest::IsDiscarded(EGBufferTextureID TextureID) const
{
  auto Mask = UINT64_C(1) << ((std::underlying_type_t<EGBufferTextureID>)TextureID);
  return (DiscardedTextureMask.load(std::memory_order_acquire) & Mask) != 0;
}

bool FGBufferRequest::WaitForTextureTransfer(EGBufferTextureID TextureID)
{
  auto Index = (std::underlying_type_t<EGBufferTextureID>)TextureID;
  auto Mask = UINT64_C(1) << Index;
  while (!Readbacks[Index]->IsReady())
  {
    if (IsDiscarded(TextureID))
      return false;
    std::this_thread::yield();
  }
  return true;
}

void FGBufferRequest::MapTextureData(
    EGBufferTextureID TextureID,
    void*& Mapping,
    int32& SourcePitch,
    FIntPoint& Extent)
{
  MapTextureData(
    FRHICommandListExecutor::GetImmediateCommandList(),
    TextureID,
    Mapping,
    SourcePitch,
    Extent);
}

void FGBufferRequest::MapTextureData(
    class FRHICommandListImmediate& RHICmdList,
    EGBufferTextureID TextureID,
    void*& Mapping,
    int32& SourcePitch,
    FIntPoint& Extent)
{
  auto Index = (std::underlying_type_t<EGBufferTextureID>)TextureID;
  auto Readback = Readbacks[Index].Get();
  check(Readback->IsReady());
  Readback->LockTexture(RHICmdList, Mapping, SourcePitch);
  check(Mapping != nullptr);
  auto Extent3D = Readback->GetSizeXYZ();
  Extent.X = Extent3D.X;
  Extent.Y = Extent3D.Y;
}

void FGBufferRequest::UnmapTextureData(EGBufferTextureID TextureID)
{
  auto Index = (std::underlying_type_t<EGBufferTextureID>)TextureID;
  auto& Readback = Readbacks[Index];
  Readback->Unlock();
}
