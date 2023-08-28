// Copyright Epic Games, Inc. All Rights Reserved.

#include "PhysXVehicleManager.h"
#include "UObject/UObjectIterator.h"
#include "TireConfig.h"

#include "Misc/CoreMiscDefines.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Physics/PhysicsFiltering.h"
#include "PhysXPublic.h"
#include "Physics/PhysicsInterfaceCore.h"

DEFINE_LOG_CATEGORY(LogVehicles);

PRAGMA_DISABLE_DEPRECATION_WARNINGS

#if WITH_PHYSX_VEHICLES

//Angle thresholds used to categorize contacts as suspension contacts or rigid body contacts.
#define POINT_REJECT_ANGLE PxPi/4.0f
#define NORMAL_REJECT_ANGLE PxPi/4.0f

DECLARE_STATS_GROUP(TEXT("PhysXVehicleManager"), STATGROUP_PhysXVehicleManager, STATGROUP_Advanced);
DECLARE_CYCLE_STAT(TEXT("PxVehicleSuspensionRaycasts"), STAT_PhysXVehicleManager_PxVehicleSuspensionRaycasts, STATGROUP_PhysXVehicleManager);
DECLARE_CYCLE_STAT(TEXT("PxUpdateVehicles"), STAT_PhysXVehicleManager_PxUpdateVehicles, STATGROUP_PhysXVehicleManager);
DECLARE_CYCLE_STAT(TEXT("UpdateTireFrictionTable"), STAT_PhysXVehicleManager_UpdateTireFrictionTable, STATGROUP_PhysXVehicleManager);
DECLARE_CYCLE_STAT(TEXT("TickVehicles"), STAT_PhysXVehicleManager_TickVehicles, STATGROUP_PhysXVehicleManager);
DECLARE_CYCLE_STAT(TEXT("VehicleManager Update"), STAT_PhysXVehicleManager_Update, STATGROUP_PhysXVehicleManager);
DECLARE_CYCLE_STAT(TEXT("Pretick Vehicles"), STAT_PhysXVehicleManager_PretickVehicles, STATGROUP_Physics);

bool FPhysXVehicleManager::bUpdateTireFrictionTable = false;
PxVehicleDrivableSurfaceToTireFrictionPairs* FPhysXVehicleManager::SurfaceTirePairs = NULL;
TMap<FPhysScene*, FPhysXVehicleManager*> FPhysXVehicleManager::SceneToVehicleManagerMap;
uint32 FPhysXVehicleManager::VehicleSetupTag = 0;

/**
 * prefilter shader for suspension raycasts
 */
static PxQueryHitType::Enum WheelRaycastPreFilter(	
	PxFilterData SuspensionData, 
	PxFilterData HitData,
	const void* constantBlock, PxU32 constantBlockSize,
	PxHitFlags& filterFlags)
{
	// SuspensionData is the vehicle suspension raycast.
	// HitData is the shape potentially hit by the raycast.

	// don't collide with owner chassis
	if ( SuspensionData.word0 == HitData.word0 )
	{
		return PxQueryHitType::eNONE;
	}
	
	PxU32 ShapeFlags = SuspensionData.word3 & 0xFFFFFF;
	PxU32 QuerierFlags = HitData.word3 & 0xFFFFFF;
	PxU32 CommonFlags = ShapeFlags & QuerierFlags;

	// Check complexity matches
	if (!(CommonFlags & EPDF_SimpleCollision) && !(CommonFlags & EPDF_ComplexCollision))
	{
		return PxQueryHitType::eNONE;
	}

	// collision channels filter
	ECollisionChannel SuspensionChannel = GetCollisionChannel(SuspensionData.word3);

	if ( ECC_TO_BITFIELD(SuspensionChannel) & HitData.word1)
	{
		// debug what object we hit
		if ( false )
		{
			for ( FObjectIterator It; It; ++It )
			{
				if ( It->GetUniqueID() == HitData.word0 )
				{
					UObject* HitObj = *It;
					FString HitObjName = HitObj->GetName();
					break;
				}
			}
		}

		return PxQueryHitType::eBLOCK;
	}

	return PxQueryHitType::eNONE;
}

PxQueryHitType::Enum WheelInitialOverlapPostFilter
(PxFilterData filterData0, PxFilterData filterData1,
    const void* constantBlock, PxU32 constantBlockSize,
    const PxQueryHit& hit)
{
    PX_UNUSED(filterData0);
    PX_UNUSED(filterData1);
    PX_UNUSED(constantBlock);
    PX_UNUSED(constantBlockSize);
    if ((static_cast<const PxSweepHit&>(hit)).hadInitialOverlap())
        return PxQueryHitType::eNONE;
    return PxQueryHitType::eBLOCK;
}

FPhysXVehicleManager::FPhysXVehicleManager(FPhysScene* PhysScene)
	: WheelRaycastBatchQuery(NULL), WheelSweepBatchQuery(NULL)

#if PX_DEBUG_VEHICLE_ON
	, TelemetryData4W(NULL)
	, TelemetryVehicle(NULL)
#endif
{
	// Save pointer to PhysX scene
	Scene = PhysScene->GetPxScene();

	// Set up delegates
	OnPhysScenePreTickHandle = PhysScene->OnPhysScenePreTick.AddRaw(this, &FPhysXVehicleManager::PreTick);
	OnPhysSceneStepHandle = PhysScene->OnPhysSceneStep.AddRaw(this, &FPhysXVehicleManager::Update);

	// Add to map
	FPhysXVehicleManager::SceneToVehicleManagerMap.Add(PhysScene, this);


	// Set the correct basis vectors with Z up, X forward. It's very IMPORTANT to set the Ackermann axle separation and frontWidth, rearWidth accordingly
	PxVehicleSetBasisVectors( PxVec3(0,0,1), PxVec3(1,0,0) );
}

void FPhysXVehicleManager::DetachFromPhysScene(FPhysScene* PhysScene)
{
	PhysScene->OnPhysScenePreTick.Remove(OnPhysScenePreTickHandle);
	PhysScene->OnPhysSceneStep.Remove(OnPhysSceneStepHandle);

	FPhysXVehicleManager::SceneToVehicleManagerMap.Remove(PhysScene);
}

FPhysXVehicleManager::~FPhysXVehicleManager()
{
#if PX_DEBUG_VEHICLE_ON
	if(TelemetryData4W)
	{
		TelemetryData4W->free();
		TelemetryData4W = NULL;
	}

	TelemetryVehicle = NULL;
#endif

	// Remove the N-wheeled vehicles.
	while( Vehicles.Num() > 0 )
	{
		RemoveVehicle( Vehicles.Last() );
	}

	// Release batch query data
	if ( WheelRaycastBatchQuery )
	{
		WheelRaycastBatchQuery->release();
		WheelRaycastBatchQuery = NULL;
	}

	if ( WheelSweepBatchQuery )
	{
		WheelSweepBatchQuery->release();
		WheelRaycastBatchQuery = NULL;
	}

	// Release the  friction values used for combinations of tire type and surface type.
	//if ( SurfaceTirePairs )
	//{
	//	SurfaceTirePairs->release();
	//	SurfaceTirePairs = NULL;
	//}
}

FPhysXVehicleManager* FPhysXVehicleManager::GetVehicleManagerFromScene(FPhysScene* PhysScene)
{
	FPhysXVehicleManager* Manager = nullptr;
	FPhysXVehicleManager** ManagerPtr = SceneToVehicleManagerMap.Find(PhysScene);
	if (ManagerPtr != nullptr)
	{
		Manager = *ManagerPtr;
	}
	return Manager;
}

static UTireConfig* DefaultTireConfig = nullptr;

UTireConfig* FPhysXVehicleManager::GetDefaultTireConfig()
{
	if (DefaultTireConfig == nullptr)
	{
		DefaultTireConfig = NewObject<UTireConfig>();
		DefaultTireConfig->AddToRoot(); // prevent GC
	}

	return DefaultTireConfig;
}

void FPhysXVehicleManager::UpdateTireFrictionTable()
{
	bUpdateTireFrictionTable = true;
}

void FPhysXVehicleManager::UpdateTireFrictionTableInternal()
{
	const PxU32 MAX_NUM_MATERIALS = 128;

	// There are tire types and then there are drivable surface types.
	// PhysX supports physical materials that share a drivable surface type,
	// but we just create a drivable surface type for every type of physical material
	PxMaterial*							AllPhysicsMaterials[MAX_NUM_MATERIALS];
	PxVehicleDrivableSurfaceType		DrivableSurfaceTypes[MAX_NUM_MATERIALS];

	// Gather all the physical materials
	uint32 NumMaterials = GPhysXSDK->getMaterials(AllPhysicsMaterials, MAX_NUM_MATERIALS);

	uint32 NumTireConfigs = UTireConfig::AllTireConfigs.Num();

	for ( uint32 m = 0; m < NumMaterials; ++m )
	{
		// Set up the drivable surface type that will be used for the new material.
		DrivableSurfaceTypes[m].mType = m;
	}

	// Release the previous SurfaceTirePairs, if any
	if ( SurfaceTirePairs )
	{
		SurfaceTirePairs->release();
		SurfaceTirePairs = NULL;
	}

	// Set up the friction values arising from combinations of tire type and surface type.
	SurfaceTirePairs = PxVehicleDrivableSurfaceToTireFrictionPairs::allocate( NumTireConfigs, NumMaterials );
	SurfaceTirePairs->setup(NumTireConfigs, NumMaterials, (const PxMaterial**)AllPhysicsMaterials, DrivableSurfaceTypes );

	// Iterate over each physical material
	for ( uint32 m = 0; m < NumMaterials; ++m )
	{
		UPhysicalMaterial* PhysMat = FPhysxUserData::Get<UPhysicalMaterial>(AllPhysicsMaterials[m]->userData);
		if (PhysMat != nullptr)
		{
			// Iterate over each tire config
			for (uint32 t = 0; t < NumTireConfigs; ++t)
		{
				UTireConfig* TireConfig = UTireConfig::AllTireConfigs[t].Get();
				if (TireConfig != nullptr)
			{
					float TireFriction = TireConfig->GetTireFriction(PhysMat);
					SurfaceTirePairs->setTypePairFriction(m, t, TireFriction);
			}
			}
		}
	}
}

void FPhysXVehicleManager::SetUpRaycastBatchedSceneQuery()
{
	int32 NumWheelsRC = 0;

	for (int32 v = PRaycastVehicles.Num() - 1; v >= 0; --v )
		NumWheelsRC += PRaycastVehicles[v]->mWheelsSimData.getNbWheels();

	if ( NumWheelsRC > WheelRaycastQueryResults.Num() )
	{
		WheelRaycastQueryResults.AddZeroed( NumWheelsRC - WheelRaycastQueryResults.Num() );
		WheelRaycastHitResults.AddZeroed( NumWheelsRC - WheelRaycastHitResults.Num() );

		check( WheelRaycastHitResults.Num() == WheelRaycastQueryResults.Num() );

		if ( WheelRaycastBatchQuery )
		{
			WheelRaycastBatchQuery->release();
			WheelRaycastBatchQuery = NULL;
		}
		
		PxBatchQueryDesc SqDesc(NumWheelsRC, 0, 0);
		SqDesc.queryMemory.userRaycastResultBuffer = WheelRaycastQueryResults.GetData();
		SqDesc.queryMemory.userRaycastTouchBuffer = WheelRaycastHitResults.GetData();
		SqDesc.queryMemory.raycastTouchBufferSize = WheelRaycastHitResults.Num();
		SqDesc.preFilterShader = WheelRaycastPreFilter;

		WheelRaycastBatchQuery = Scene->createBatchQuery( SqDesc );
	}
}

void FPhysXVehicleManager::SetUpSweepBatchedSceneQuery()
{
	int32 NumWheelsSW = 0;

	for (int32 v = PSweepVehicles.Num() - 1; v >= 0; --v )
	{
		NumWheelsSW += PSweepVehicles[v]->mWheelsSimData.getNbWheels();
	}

	if (NumWheelsSW > WheelSweepQueryResults.Num())
	{
		WheelSweepQueryResults.AddZeroed(NumWheelsSW - WheelSweepQueryResults.Num());
		WheelSweepResults.AddZeroed(NumWheelsSW - WheelSweepResults.Num());

		check(WheelSweepResults.Num() == WheelSweepQueryResults.Num());

		if (WheelSweepBatchQuery)
		{
			WheelSweepBatchQuery->release();
			WheelSweepBatchQuery = NULL;
		}

		PxBatchQueryDesc SqDesc(0, NumWheelsSW, 0);
		SqDesc.queryMemory.userSweepResultBuffer = WheelSweepQueryResults.GetData();
		SqDesc.queryMemory.userSweepTouchBuffer = WheelSweepResults.GetData();
		SqDesc.queryMemory.sweepTouchBufferSize = WheelSweepResults.Num();
		SqDesc.preFilterShader = WheelRaycastPreFilter;
		SqDesc.postFilterShader = WheelInitialOverlapPostFilter;
		PxVehicleSetSweepHitRejectionAngles(POINT_REJECT_ANGLE, NORMAL_REJECT_ANGLE);

		WheelSweepBatchQuery = Scene->createBatchQuery(SqDesc);
	}
}

void FPhysXVehicleManager::SetUpBatchedSceneQuery()
{
	check(Vehicles.Num() == (PRaycastVehicles.Num() + PSweepVehicles.Num()));

	FPhysXVehicleManager::SetUpRaycastBatchedSceneQuery();
	FPhysXVehicleManager::SetUpSweepBatchedSceneQuery();
}

void FPhysXVehicleManager::AddVehicle( TWeakObjectPtr<UWheeledVehicleMovementComponent> Vehicle )
{
	check(Vehicle != NULL);
	check(Vehicle->PVehicle);

	PxU32 NumWheels = Vehicle->PVehicle->mWheelsSimData.getNbWheels();

	Vehicles.Add( Vehicle );
	if(Vehicle->UseSweepWheelCollision) {
		PSweepVehicles.Add( Vehicle->PVehicle );
		int32 NewIndex = PSweepVehiclesWheelsStates.AddZeroed();
		PSweepVehiclesWheelsStates[NewIndex].nbWheelQueryResults = NumWheels;
		PSweepVehiclesWheelsStates[NewIndex].wheelQueryResults = new PxWheelQueryResult[NumWheels];
	}
	else {
		PRaycastVehicles.Add( Vehicle->PVehicle );
		int32 NewIndex = PRaycastVehiclesWheelsStates.AddZeroed();
		PRaycastVehiclesWheelsStates[NewIndex].nbWheelQueryResults = NumWheels;
		PRaycastVehiclesWheelsStates[NewIndex].wheelQueryResults = new PxWheelQueryResult[NumWheels];
	}

	SetUpBatchedSceneQuery();
}

void FPhysXVehicleManager::RemoveVehicle( TWeakObjectPtr<UWheeledVehicleMovementComponent> Vehicle )
{
	check(Vehicle != NULL);
	check(Vehicle->PVehicle);

	PxVehicleWheels* PVehicle = Vehicle->PVehicle;

	int32 RemovedIndex = Vehicles.Find(Vehicle);
	int32 RemovedIndexRC = PRaycastVehicles.Find(PVehicle);
	int32 RemovedIndexSW = PSweepVehicles.Find(PVehicle);

	int32 remV = Vehicles.Remove( Vehicle );
	int32 remRC = PRaycastVehicles.Remove( PVehicle );
	int32 remSW = PSweepVehicles.Remove( PVehicle );

	if (RemovedIndexRC != INDEX_NONE) {
		delete[] PRaycastVehiclesWheelsStates[RemovedIndexRC].wheelQueryResults;
		PRaycastVehiclesWheelsStates.RemoveAt(RemovedIndexRC); // LOC_MOD double check this
	}

	if (RemovedIndexSW != INDEX_NONE) {
		delete[] PSweepVehiclesWheelsStates[RemovedIndexSW].wheelQueryResults;
		PSweepVehiclesWheelsStates.RemoveAt(RemovedIndexSW); // LOC_MOD double check this
	}

	if ( PVehicle == TelemetryVehicle )
	{
		TelemetryVehicle = NULL;
	}

	switch( PVehicle->getVehicleType() )
	{
	case PxVehicleTypes::eDRIVE4W:
		((PxVehicleDrive4W*)PVehicle)->free();
		break;
	case PxVehicleTypes::eDRIVETANK:
		((PxVehicleDriveTank*)PVehicle)->free();
		break;
	case PxVehicleTypes::eDRIVENW:
		((PxVehicleDriveNW*)PVehicle)->free();
		break;
	case PxVehicleTypes::eNODRIVE:
		((PxVehicleNoDrive*)PVehicle)->free();
		break;
	default:
		checkf( 0, TEXT("Unsupported vehicle type"));
		break;
	}
}

void FPhysXVehicleManager::Update(FPhysScene* PhysScene, float DeltaTime)
{
	SCOPE_CYCLE_COUNTER(STAT_PhysXVehicleManager_Update);

	// Only support vehicles in sync scene
	if (Vehicles.Num() == 0 )
	{
		return;
	}

	if ( bUpdateTireFrictionTable )
	{
		SCOPE_CYCLE_COUNTER(STAT_PhysXVehicleManager_UpdateTireFrictionTable);
		bUpdateTireFrictionTable = false;
		UpdateTireFrictionTableInternal();
	}

	// Suspension raycasts
	{
		SCOPE_CYCLE_COUNTER(STAT_PhysXVehicleManager_PxVehicleSuspensionRaycasts);
		SCOPED_SCENE_READ_LOCK(Scene);

		if (WheelRaycastBatchQuery != nullptr && PRaycastVehicles.Num() > 0)
			PxVehicleSuspensionRaycasts( WheelRaycastBatchQuery, PRaycastVehicles.Num(), PRaycastVehicles.GetData(), WheelRaycastQueryResults.Num(), WheelRaycastQueryResults.GetData() );
		if (WheelSweepBatchQuery != nullptr && PSweepVehicles.Num() > 0)
			PxVehicleSuspensionSweeps( WheelSweepBatchQuery, PSweepVehicles.Num(), PSweepVehicles.GetData(), WheelSweepQueryResults.Num(), WheelSweepQueryResults.GetData(), 16 , NULL, 1.0f, 1.1f);
	}
	

	// Tick vehicles
	{
		SCOPE_CYCLE_COUNTER(STAT_PhysXVehicleManager_TickVehicles);
		for (int32 i = Vehicles.Num() - 1; i >= 0; --i)
		{
			Vehicles[i]->TickVehicle(DeltaTime);
		}
	}

#if PX_DEBUG_VEHICLE_ON

	if ( TelemetryVehicle != NULL )
	{
		UpdateVehiclesWithTelemetry( DeltaTime );
	}
	else
	{
		UpdateVehicles( DeltaTime );
	}

#else

	UpdateVehicles( DeltaTime );

#endif //PX_DEBUG_VEHICLE_ON
}

void FPhysXVehicleManager::PreTick(FPhysScene* PhysScene, float DeltaTime)
{
	SCOPE_CYCLE_COUNTER(STAT_PhysXVehicleManager_PretickVehicles);

	for (int32 i = 0; i < Vehicles.Num(); ++i)
	{
		Vehicles[i]->PreTick(DeltaTime);
	}
}


void FPhysXVehicleManager::UpdateVehicles( float DeltaTime )
{
	SCOPE_CYCLE_COUNTER(STAT_PhysXVehicleManager_PxUpdateVehicles);
	SCOPED_SCENE_WRITE_LOCK(Scene);
	if (WheelRaycastBatchQuery != nullptr && PRaycastVehicles.Num() > 0)
		PxVehicleUpdates( DeltaTime, GetSceneGravity_AssumesLocked(), *SurfaceTirePairs, PRaycastVehicles.Num(), PRaycastVehicles.GetData(), PRaycastVehiclesWheelsStates.GetData());
	SCOPED_SCENE_WRITE_LOCK(Scene);
	if (WheelSweepBatchQuery != nullptr && PSweepVehicles.Num() > 0)
		PxVehicleUpdates( DeltaTime, GetSceneGravity_AssumesLocked(), *SurfaceTirePairs, PSweepVehicles.Num(), PSweepVehicles.GetData(), PSweepVehiclesWheelsStates.GetData());
}

PxVec3 FPhysXVehicleManager::GetSceneGravity_AssumesLocked()
{
	return Scene->getGravity();
}

void FPhysXVehicleManager::SetRecordTelemetry( TWeakObjectPtr<UWheeledVehicleMovementComponent> Vehicle, bool bRecord )
{
#if PX_DEBUG_VEHICLE_ON

	if ( Vehicle != NULL && Vehicle->PVehicle != NULL )
	{
		PxVehicleWheels* PVehicle = Vehicle->PVehicle;

		if ( bRecord )
		{
			int32 VehicleIndex = Vehicles.Find( Vehicle );

			if ( VehicleIndex != INDEX_NONE )
			{
				// Make sure telemetry is setup
				SetupTelemetryData();

				TelemetryVehicle = PVehicle;

				if ( VehicleIndex != 0 )
				{

					Vehicles.Swap( 0, VehicleIndex );

					int32 IndexRC = PRaycastVehicles.Find(PVehicle);
					if (IndexRC != INDEX_NONE ) {
						PRaycastVehicles.Swap( 0, IndexRC );
						PRaycastVehiclesWheelsStates.Swap( 0, IndexRC );
					}

					int32 IndexSW = PSweepVehicles.Find(PVehicle);
					if (IndexSW != INDEX_NONE ) {
						PSweepVehicles.Swap( 0, IndexSW );
						PSweepVehiclesWheelsStates.Swap( 0, IndexSW );
					}
				}
			}
		}
		else
		{
			if ( PVehicle == TelemetryVehicle )
			{
				TelemetryVehicle = NULL;
			}
		}
	}

#endif
}

#if PX_DEBUG_VEHICLE_ON

void FPhysXVehicleManager::SetupTelemetryData()
{
	// set up telemetry for 4 wheels
	if(TelemetryData4W == NULL)
	{
		SCOPED_SCENE_WRITE_LOCK(Scene);

		float Empty[] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };

		TelemetryData4W = PxVehicleTelemetryData::allocate(4);
		TelemetryData4W->setup(1.0f, 1.0f, 0.0f, 0.0f, Empty, Empty, PxVec3(0,0,0), PxVec3(0,0,0), PxVec3(0,0,0));
	}
}

void FPhysXVehicleManager::UpdateVehiclesWithTelemetry( float DeltaTime )
{
	check(TelemetryVehicle);
	bool telemetry_sweep = false;

	SCOPED_SCENE_WRITE_LOCK(Scene);
	if ( PxVehicleTelemetryData* TelemetryData = GetTelemetryData_AssumesLocked() )
	{
		PxVehicleWheelQueryResult* TelemetryWheelQueryResult = nullptr;
		if (PRaycastVehicles.Find(TelemetryVehicle) != INDEX_NONE) {
			telemetry_sweep = false;
			TelemetryWheelQueryResult = PRaycastVehiclesWheelsStates.GetData();
		}
		else if (PSweepVehicles.Find(TelemetryVehicle) != INDEX_NONE) {
			telemetry_sweep = true;
			TelemetryWheelQueryResult = PSweepVehiclesWheelsStates.GetData();
		}
		else
			check(false); // It should never arrive here.

		PxVehicleUpdateSingleVehicleAndStoreTelemetryData( DeltaTime, GetSceneGravity_AssumesLocked(), *SurfaceTirePairs, TelemetryVehicle, TelemetryWheelQueryResult, *TelemetryData );

		if (telemetry_sweep == false)
		{
			if (WheelRaycastBatchQuery != nullptr && PRaycastVehicles.Num() > 1)
				PxVehicleUpdates( DeltaTime, GetSceneGravity_AssumesLocked(), *SurfaceTirePairs, PRaycastVehicles.Num() - 1, &PRaycastVehicles[1], &PRaycastVehiclesWheelsStates[1]);
			if (WheelSweepBatchQuery != nullptr && PSweepVehicles.Num() > 0)
				PxVehicleUpdates( DeltaTime, GetSceneGravity_AssumesLocked(), *SurfaceTirePairs, PSweepVehicles.Num(), PSweepVehicles.GetData(), PSweepVehiclesWheelsStates.GetData());
		}
		else
		{
			if (WheelRaycastBatchQuery != nullptr && PRaycastVehicles.Num() > 0)
				PxVehicleUpdates( DeltaTime, GetSceneGravity_AssumesLocked(), *SurfaceTirePairs, PRaycastVehicles.Num(), PRaycastVehicles.GetData(), PRaycastVehiclesWheelsStates.GetData());
			if (WheelSweepBatchQuery != nullptr && PSweepVehicles.Num() > 1)
				PxVehicleUpdates( DeltaTime, GetSceneGravity_AssumesLocked(), *SurfaceTirePairs, PSweepVehicles.Num() - 1, &PSweepVehicles[1], &PSweepVehiclesWheelsStates[1]);
		}
	}
	else
	{
		UE_LOG( LogPhysics, Warning, TEXT("Cannot record telemetry for vehicle, it does not have 4 wheels") );

		if (WheelRaycastBatchQuery != nullptr && PRaycastVehicles.Num() > 0)
			PxVehicleUpdates( DeltaTime, GetSceneGravity_AssumesLocked(), *SurfaceTirePairs, PRaycastVehicles.Num(), PRaycastVehicles.GetData(), PRaycastVehiclesWheelsStates.GetData());
		if (WheelSweepBatchQuery != nullptr && PSweepVehicles.Num() > 0)
			PxVehicleUpdates( DeltaTime, GetSceneGravity_AssumesLocked(), *SurfaceTirePairs, PSweepVehicles.Num(), PSweepVehicles.GetData(), PSweepVehiclesWheelsStates.GetData());
	}
}

PxVehicleTelemetryData* FPhysXVehicleManager::GetTelemetryData_AssumesLocked()
{
	if ( TelemetryVehicle )
	{
		if ( TelemetryVehicle->mWheelsSimData.getNbWheels() == 4 )
		{
			return TelemetryData4W;
		}
	}

	return nullptr;
}

#endif //PX_DEBUG_VEHICLE_ON

PxWheelQueryResult* FPhysXVehicleManager::GetWheelsStates_AssumesLocked(TWeakObjectPtr<const UWheeledVehicleMovementComponent> Vehicle)
{
	PxVehicleWheels* PVehicle = Vehicle->PVehicle;

	int32 RemovedIndexRC = PRaycastVehicles.Find(PVehicle);

	if(RemovedIndexRC != INDEX_NONE)
	{
		return PRaycastVehiclesWheelsStates[RemovedIndexRC].wheelQueryResults;
	}
	else
	{
		int32 RemovedIndexSW = PSweepVehicles.Find(PVehicle);
		if(RemovedIndexSW != INDEX_NONE)
		{
			return PSweepVehiclesWheelsStates[RemovedIndexSW].wheelQueryResults;
		}
		else
		{
			return nullptr;
		}
	}
}

#endif // WITH_PHYSX

PRAGMA_ENABLE_DEPRECATION_WARNINGS
