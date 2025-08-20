// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Stats/Stats.h"
#include "UObject/ObjectMacros.h"
#include "Async/AsyncWork.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "StaticMeshResources.h"
#include "Engine/InstancedStaticMesh.h"
#include "StaticMeshResources.h"

#include "HierarchicalInstancedStaticMeshComponent.generated.h"

class FClusterBuilder;
class FStaticLightingTextureMapping_InstancedStaticMesh;

// Due to BulkSerialize we can't edit the struct, so we must deprecated this one and create a new one
USTRUCT()
struct FClusterNode_DEPRECATED
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FVector BoundMin;
	UPROPERTY()
	int32 FirstChild;
	UPROPERTY()
	FVector BoundMax;
	UPROPERTY()
	int32 LastChild;
	UPROPERTY()
	int32 FirstInstance;
	UPROPERTY()
	int32 LastInstance;

	FClusterNode_DEPRECATED()
		: BoundMin(MAX_flt, MAX_flt, MAX_flt)
		, FirstChild(-1)
		, BoundMax(MIN_flt, MIN_flt, MIN_flt)
		, LastChild(-1)
		, FirstInstance(-1)
		, LastInstance(-1)
	{
	}

	friend FArchive& operator<<(FArchive& Ar, FClusterNode_DEPRECATED& NodeData)
	{
		// @warning BulkSerialize: FClusterNode is serialized as memory dump
		// See TArray::BulkSerialize for detailed description of implied limitations.
		Ar << NodeData.BoundMin;
		Ar << NodeData.FirstChild;
		Ar << NodeData.BoundMax;
		Ar << NodeData.LastChild;
		Ar << NodeData.FirstInstance;
		Ar << NodeData.LastInstance;

		return Ar;
	}
};


USTRUCT()
struct FClusterNode
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FVector BoundMin;
	UPROPERTY()
	int32 FirstChild;
	UPROPERTY()
	FVector BoundMax;
	UPROPERTY()
	int32 LastChild;
	UPROPERTY()
	int32 FirstInstance;
	UPROPERTY()
	int32 LastInstance;

	UPROPERTY()
	FVector MinInstanceScale;
	UPROPERTY()
	FVector MaxInstanceScale;

	FClusterNode()
		: BoundMin(MAX_flt, MAX_flt, MAX_flt)
		, FirstChild(-1)
		, BoundMax(MIN_flt, MIN_flt, MIN_flt)
		, LastChild(-1)
		, FirstInstance(-1)
		, LastInstance(-1)
		, MinInstanceScale(MAX_flt)
		, MaxInstanceScale(-MAX_flt)
	{
	}

	FClusterNode(const FClusterNode_DEPRECATED& OldNode)
		: BoundMin(OldNode.BoundMin)
		, FirstChild(OldNode.FirstChild)
		, BoundMax(OldNode.BoundMax)
		, LastChild(OldNode.LastChild)
		, FirstInstance(OldNode.FirstChild)
		, LastInstance(OldNode.LastInstance)
		, MinInstanceScale(MAX_flt)
		, MaxInstanceScale(-MAX_flt)
	{
	}

	friend FArchive& operator<<(FArchive& Ar, FClusterNode& NodeData)
	{
		// @warning BulkSerialize: FClusterNode is serialized as memory dump
		// See TArray::BulkSerialize for detailed description of implied limitations.
		Ar << NodeData.BoundMin;
		Ar << NodeData.FirstChild;
		Ar << NodeData.BoundMax;
		Ar << NodeData.LastChild;
		Ar << NodeData.FirstInstance;
		Ar << NodeData.LastInstance;
		Ar << NodeData.MinInstanceScale;
		Ar << NodeData.MaxInstanceScale;
		
		return Ar;
	}
};

UCLASS(ClassGroup=Rendering, meta=(BlueprintSpawnableComponent))
class ENGINE_API UHierarchicalInstancedStaticMeshComponent : public UInstancedStaticMeshComponent
{
	GENERATED_UCLASS_BODY()

	~UHierarchicalInstancedStaticMeshComponent();

	TSharedPtr<TArray<FClusterNode>, ESPMode::ThreadSafe> ClusterTreePtr;

	// Table for remaping instances from cluster tree to PerInstanceSMData order
	UPROPERTY()
	TArray<int32> SortedInstances;

	// The number of instances in the ClusterTree. Subsequent instances will always be rendered.
	UPROPERTY()
	int32 NumBuiltInstances;

	// Normally equal to NumBuiltInstances, but can be lower if density scaling is in effect
	int32 NumBuiltRenderInstances;

	// Bounding box of any built instances (cached from the ClusterTree)
	UPROPERTY()
	FBox BuiltInstanceBounds;

	// Bounding box of any unbuilt instances
	UPROPERTY()
	FBox UnbuiltInstanceBounds;

	// Bounds of each individual unbuilt instance, used for LOD calculation
	UPROPERTY()
	TArray<FBox> UnbuiltInstanceBoundsList;

	// Enable for detail meshes that don't really affect the game. Disable for anything important.
	// Typically, this will be enabled for small meshes without collision (e.g. grass) and disabled for large meshes with collision (e.g. trees)
	UPROPERTY()
	uint32 bEnableDensityScaling:1;

	// Current value of density scaling applied to this component
	float CurrentDensityScaling;

	// The number of nodes in the occlusion layer
	UPROPERTY()
	int32 OcclusionLayerNumNodes;

	// The last mesh bounds that was cache
	UPROPERTY()
	FBoxSphereBounds CacheMeshExtendedBounds;

	UPROPERTY()
	bool bDisableCollision;

	// Instances to render (including removed one until the build is complete)
	UPROPERTY()
	int32 InstanceCountToRender;

	bool bIsAsyncBuilding : 1;
	bool bIsOutOfDate : 1;
	bool bConcurrentChanges : 1;
	bool bAutoRebuildTreeOnInstanceChanges : 1;

#if WITH_EDITOR
	// in Editor mode we might disable the density scaling for edition
	bool bCanEnableDensityScaling : 1;
#endif

	// Apply the results of the async build
	void ApplyBuildTreeAsync(ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent, TSharedRef<FClusterBuilder, ESPMode::ThreadSafe> Builder, double StartTime);

public:

	//Begin UObject Interface
	virtual void Serialize(FArchive& Ar) override;
	virtual void GetResourceSizeEx(FResourceSizeEx& CumulativeResourceSize) override;
	virtual void PostLoad() override;
	virtual void PostEditImport() override;
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	virtual FBoxSphereBounds CalcBounds(const FTransform& BoundTransform) const override;
#if WITH_EDITOR
	virtual void PostEditUndo() override;
	virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) override;
#endif

	// UInstancedStaticMesh interface
	virtual int32 AddInstance(const FTransform& InstanceTransform) override;
	virtual TArray<int32> AddInstances(const TArray<FTransform>& InstanceTransforms, bool bShouldReturnIndices) override;
	virtual bool RemoveInstance(int32 InstanceIndex) override;
	virtual bool UpdateInstanceTransform(int32 InstanceIndex, const FTransform& NewInstanceTransform, bool bWorldSpace, bool bMarkRenderStateDirty = false, bool bTeleport = false) override;
	virtual bool SetCustomDataValue(int32 InstanceIndex, int32 CustomDataIndex, float CustomDataValue, bool bMarkRenderStateDirty = false) override;
	virtual bool SetCustomData(int32 InstanceIndex, const TArray<float>& InCustomData, bool bMarkRenderStateDirty = false) override;
	virtual bool BatchUpdateInstancesTransforms(int32 StartInstanceIndex, const TArray<FTransform>& NewInstancesTransforms, bool bWorldSpace=false, bool bMarkRenderStateDirty=false, bool bTeleport=false) override;
	virtual bool BatchUpdateInstancesTransform(int32 StartInstanceIndex, int32 NumInstances, const FTransform& NewInstancesTransform, bool bWorldSpace=false, bool bMarkRenderStateDirty=false, bool bTeleport=false) override;
	virtual bool BatchUpdateInstancesData(int32 StartInstanceIndex, int32 NumInstances, FInstancedStaticMeshInstanceData* StartInstanceData, bool bMarkRenderStateDirty = false, bool bTeleport = false) override;

	virtual void ClearInstances() override;
	virtual TArray<int32> GetInstancesOverlappingSphere(const FVector& Center, float Radius, bool bSphereInWorldSpace = true) const override;
	virtual TArray<int32> GetInstancesOverlappingBox(const FBox& Box, bool bBoxInWorldSpace = true) const override;
	virtual void PreAllocateInstancesMemory(int32 AddedInstanceCount) override;

	/** Removes all the instances with indices specified in the InstancesToRemove array. Returns true on success. */
	UFUNCTION(BlueprintCallable, Category = "Components|InstancedStaticMesh")
	bool RemoveInstances(const TArray<int32>& InstancesToRemove);

	/** Get the number of instances that overlap a given sphere */
	int32 GetOverlappingSphereCount(const FSphere& Sphere) const;
	/** Get the number of instances that overlap a given box */
	int32 GetOverlappingBoxCount(const FBox& Box) const;
	/** Get the transforms of instances inside the provided box */
	void GetOverlappingBoxTransforms(const FBox& Box, TArray<FTransform>& OutTransforms) const;

	virtual bool ShouldCreatePhysicsState() const override;

	bool BuildTreeIfOutdated(bool Async, bool ForceUpdate);
	static void BuildTreeAnyThread(TArray<FMatrix>& InstanceTransforms, TArray<float>& InstanceCustomDataFloats, int32 NumCustomDataFloats, const FBox& MeshBox, TArray<FClusterNode>& OutClusterTree, TArray<int32>& OutSortedInstances, TArray<int32>& OutInstanceReorderTable, int32& OutOcclusionLayerNum, int32 MaxInstancesPerLeaf, bool InGenerateInstanceScalingRange);
	void AcceptPrebuiltTree(TArray<FClusterNode>& InClusterTree, int32 InOcclusionLayerNumNodes, int32 InNumBuiltRenderInstances);
	bool IsAsyncBuilding() const { return bIsAsyncBuilding; }
	bool IsTreeFullyBuilt() const { return !bIsOutOfDate; }

	/** Heuristic for the number of leaves in the tree **/
	int32 DesiredInstancesPerLeaf();

	virtual void ApplyComponentInstanceData(struct FInstancedStaticMeshComponentInstanceData* InstancedMeshData) override;
	
	// Number of instances in the render-side instance buffer
	virtual int32 GetNumRenderInstances() const { return SortedInstances.Num(); }

	/** Will apply current density scaling, if enabled **/
	void UpdateDensityScaling();

	virtual void PropagateLightingScenarioChange() override;

protected:
	void BuildTree();
	void BuildTreeAsync();
	void ApplyBuildTree(FClusterBuilder& Builder);
	void ApplyEmpty();
	void SetPerInstanceLightMapAndEditorData(FStaticMeshInstanceData& PerInstanceData, const TArray<TRefCountPtr<HHitProxy>>& HitProxies);

	void GetInstanceTransforms(TArray<FMatrix>& InstanceTransforms) const;
	void InitializeInstancingRandomSeed();

	/** Removes specified instances */ 
	void RemoveInstancesInternal(const int32* InstanceIndices, int32 Num);
	
	/** Gets and approximate number of verts for each LOD to generate heuristics **/
	int32 GetVertsForLOD(int32 LODIndex);
	/** Average number of instances per leaf **/
	float ActualInstancesPerLeaf();
	/** For testing, prints some stats after any kind of build **/
	void PostBuildStats();

	virtual void OnPostLoadPerInstanceData() override;

	virtual void GetNavigationPerInstanceTransforms(const FBox& AreaBox, TArray<FTransform>& InstanceData) const override;
	virtual void PartialNavigationUpdate(int32 InstanceIdx) override;
	virtual bool SupportsPartialNavigationUpdate() const override { return true; }
	virtual FBox GetNavigationBounds() const override;
	void FlushAccumulatedNavigationUpdates();
	mutable FBox AccumulatedNavigationDirtyArea;

	FGraphEventArray BuildTreeAsyncTasks;

protected:
	friend FStaticLightingTextureMapping_InstancedStaticMesh;
	friend FInstancedLightMap2D;
	friend FInstancedShadowMap2D;
	friend class FClusterBuilder;
};


struct FFoliageOcclusionResults
{
	TArray<bool> Results; // we keep a copy from the View as the view will get destroyed too often
	int32 ResultsStart;
	int32 NumResults;
	uint32 FrameNumberRenderThread;

	FFoliageOcclusionResults(TArray<bool>* InResults, int32 InResultsStart, int32 InNumResults)
		: Results(*InResults)
		, ResultsStart(InResultsStart)
		, NumResults(InNumResults)
		, FrameNumberRenderThread(GFrameNumberRenderThread)
	{

	}
};

struct FFoliageElementParams;
struct FFoliageRenderInstanceParams;
struct FFoliageCullInstanceParams;

class ENGINE_API FHierarchicalStaticMeshSceneProxy : public FInstancedStaticMeshSceneProxy
{
	TSharedRef<TArray<FClusterNode>, ESPMode::ThreadSafe> ClusterTreePtr;
	const TArray<FClusterNode>& ClusterTree;

	TArray<FBox> UnbuiltBounds;
	int32 FirstUnbuiltIndex;
	int32 InstanceCountToRender;

	int32 FirstOcclusionNode;
	int32 LastOcclusionNode;
	TArray<FBoxSphereBounds> OcclusionBounds;
	TMap<uint32, FFoliageOcclusionResults> OcclusionResults;
	bool bIsGrass;
	bool bDitheredLODTransitions;
	uint32 SceneProxyCreatedFrameNumberRenderThread;

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	mutable TArray<uint32> SingleDebugRuns[MAX_STATIC_MESH_LODS];
	mutable int32 SingleDebugTotalInstances[MAX_STATIC_MESH_LODS];
	mutable TArray<uint32> MultipleDebugRuns[MAX_STATIC_MESH_LODS];
	mutable int32 MultipleDebugTotalInstances[MAX_STATIC_MESH_LODS];
	mutable int32 CaptureTag;
#endif

public:
	SIZE_T GetTypeHash() const override
	{
		static size_t UniquePointer;
		return reinterpret_cast<size_t>(&UniquePointer);
	}

	FHierarchicalStaticMeshSceneProxy(bool bInIsGrass, UHierarchicalInstancedStaticMeshComponent* InComponent, ERHIFeatureLevel::Type InFeatureLevel)
		: FInstancedStaticMeshSceneProxy(InComponent, InFeatureLevel)
		, ClusterTreePtr(InComponent->ClusterTreePtr.ToSharedRef())
		, ClusterTree(*InComponent->ClusterTreePtr)
		, UnbuiltBounds(InComponent->UnbuiltInstanceBoundsList)
		, FirstUnbuiltIndex(InComponent->NumBuiltInstances > 0 ? InComponent->NumBuiltInstances : InComponent->NumBuiltRenderInstances)
		, InstanceCountToRender(InComponent->InstanceCountToRender)
		, bIsGrass(bInIsGrass)
		, bDitheredLODTransitions(InComponent->SupportsDitheredLODTransitions(InFeatureLevel))
		, SceneProxyCreatedFrameNumberRenderThread(UINT32_MAX)
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		, CaptureTag(0)
#endif
	{
		SetupOcclusion(InComponent);
	}

	void SetupOcclusion(UHierarchicalInstancedStaticMeshComponent* InComponent)
	{
		FirstOcclusionNode = 0;
		LastOcclusionNode = 0;
		if (ClusterTree.Num() && InComponent->OcclusionLayerNumNodes)
		{
			while (true)
			{
				int32 NextFirstOcclusionNode = ClusterTree[FirstOcclusionNode].FirstChild;
				int32 NextLastOcclusionNode = ClusterTree[LastOcclusionNode].LastChild;

				if (NextFirstOcclusionNode < 0 || NextLastOcclusionNode < 0)
				{
					break;
				}
				int32 NumNodes = 1 + NextLastOcclusionNode - NextFirstOcclusionNode;
				if (NumNodes > InComponent->OcclusionLayerNumNodes)
				{
					break;
				}
				FirstOcclusionNode = NextFirstOcclusionNode;
				LastOcclusionNode = NextLastOcclusionNode;
			}
		}
		int32 NumNodes = 1 + LastOcclusionNode - FirstOcclusionNode;
		if (NumNodes < 2)
		{
			FirstOcclusionNode = -1;
			LastOcclusionNode = -1;
			NumNodes = 0;
			if (ClusterTree.Num())
			{
				//UE_LOG(LogTemp, Display, TEXT("No SubOcclusion %d inst"), 1 + ClusterTree[0].LastInstance - ClusterTree[0].FirstInstance);
			}
		}
		else
		{
			//int32 NumPerNode = (1 + ClusterTree[0].LastInstance - ClusterTree[0].FirstInstance) / NumNodes;
			//UE_LOG(LogTemp, Display, TEXT("Occlusion level %d   %d inst / node"), NumNodes, NumPerNode);
			OcclusionBounds.Reserve(NumNodes);
			FMatrix XForm = InComponent->GetComponentTransform().ToMatrixWithScale();
			for (int32 Index = FirstOcclusionNode; Index <= LastOcclusionNode; Index++)
			{
				OcclusionBounds.Add(FBoxSphereBounds(FBox(ClusterTree[Index].BoundMin, ClusterTree[Index].BoundMax).TransformBy(XForm)));
			}
		}
	}

	// FPrimitiveSceneProxy interface.
	
	virtual void CreateRenderThreadResources() override
	{
		FInstancedStaticMeshSceneProxy::CreateRenderThreadResources();
		SceneProxyCreatedFrameNumberRenderThread = GFrameNumberRenderThread;
	}
	
	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override
	{
		FPrimitiveViewRelevance Result;
		if (bIsGrass ? View->Family->EngineShowFlags.InstancedGrass : View->Family->EngineShowFlags.InstancedFoliage)
		{
			Result = FStaticMeshSceneProxy::GetViewRelevance(View);
			Result.bDynamicRelevance = true;
			Result.bStaticRelevance = false;

			// Remove relevance for primitives marked for runtime virtual texture only.
			if (RuntimeVirtualTextures.Num() > 0 && !ShouldRenderInMainPass())
			{
				Result.bDynamicRelevance = false;
			}
		}
		return Result;
	}

#if RHI_RAYTRACING
	virtual bool IsRayTracingStaticRelevant() const override
	{
		return false;
	}
#endif

	virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override;

	virtual const TArray<FBoxSphereBounds>* GetOcclusionQueries(const FSceneView* View) const override;
	virtual void AcceptOcclusionResults(const FSceneView* View, TArray<bool>* Results, int32 ResultsStart, int32 NumResults) override;
	virtual bool HasSubprimitiveOcclusionQueries() const override
	{
		return FirstOcclusionNode > 0;
	}


	virtual void DrawStaticElements(FStaticPrimitiveDrawInterface* PDI) override
	{
		if (RuntimeVirtualTextures.Num() > 0)
		{
			// Create non-hierachichal static mesh batches for use by the runtime virtual texture rendering.
			//todo[vt]: Build an acceleration structure better suited for VT rendering maybe with batches aligned to VT pages?
			FInstancedStaticMeshSceneProxy::DrawStaticElements(PDI);
		}
	}

	virtual void ApplyWorldOffset(FVector InOffset) override
	{
		FInstancedStaticMeshSceneProxy::ApplyWorldOffset(InOffset);
		
		for (FBoxSphereBounds& Item : OcclusionBounds)
		{
			Item.Origin+= InOffset;
		}
	}

	void FillDynamicMeshElements(FMeshElementCollector& Collector, const FFoliageElementParams& ElementParams, const FFoliageRenderInstanceParams& Instances) const;

	template<bool TUseVector>
	void Traverse(const FFoliageCullInstanceParams& Params, int32 Index, int32 MinLOD, int32 MaxLOD, bool bFullyContained = false) const;
};
