// Copyright Epic Games, Inc. All Rights Reserved.
// Copyright (c) 2022 Computer Vision Center (CVC) at the Universitat Autonoma
// de Barcelona (UAB).

#pragma once
#include "CoreMinimal.h"
#include <atomic>



enum class EGBufferTextureID : uint8_t
{
  SceneColor,
  SceneDepth,
  SceneStencil,
  GBufferA,
  GBufferB,
  GBufferC,
  GBufferD,
  GBufferE,
  GBufferF,
  Velocity,
  SSAO,
  CustomDepth,
  CustomStencil,

  EnumMax
};

class RENDERER_API FGBufferRequest
{
public:

  FViewInfo* InitAndFindSceneView(TArrayView<FViewInfo> Views);

  static constexpr size_t TextureCount = (size_t)EGBufferTextureID::EnumMax;

  void MarkAsRequested(EGBufferTextureID TextureID);
  bool IsRequested(EGBufferTextureID TextureID) const;
  bool IsDiscarded(EGBufferTextureID TextureID) const;
  bool WaitForTextureTransfer(EGBufferTextureID TextureID);
  void MapTextureData(
      EGBufferTextureID TextureID,
      void*& Mapping,
      int32& SourcePitch,
      FIntPoint& Extent);
  void MapTextureData(
      class FRHICommandListImmediate& RHICmdList,
      EGBufferTextureID TextureID,
      void*& Mapping,
      int32& SourcePitch,
      FIntPoint& Extent);
  void UnmapTextureData(EGBufferTextureID TextureID);

  TUniquePtr<FRHIGPUTextureReadback> Readbacks[TextureCount];
  uint64_t DesiredTexturesMask;
  std::atomic_uint64_t DiscardedTextureMask;
  FIntRect ViewRect;
  const AActor* OwningActor;
};
