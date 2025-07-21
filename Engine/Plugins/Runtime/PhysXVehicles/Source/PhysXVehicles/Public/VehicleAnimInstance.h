// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once
/**
 *
 */

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "VehicleAnimInstance.generated.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS

class UWheeledVehicleMovementComponent;

struct FWheelAnimData
{
	FName BoneName;
	FRotator RotOffset;
	FVector LocOffset;
};

 /** Proxy override for this UAnimInstance-derived class */
struct UE_DEPRECATED(4.26, "PhysX is deprecated. Use the FVehicleAnimationInstanceProxy from the ChaosVehiclePhysics Plugin.") FVehicleAnimInstanceProxy;
USTRUCT()
struct PHYSXVEHICLES_API FVehicleAnimInstanceProxy : public FAnimInstanceProxy
{
	GENERATED_BODY()

	FVehicleAnimInstanceProxy()
		: FAnimInstanceProxy()
	{
	}

	FVehicleAnimInstanceProxy(UAnimInstance* Instance)
		: FAnimInstanceProxy(Instance)
	{
	}

public:

	void SetWheeledVehicleMovementComponent(const UWheeledVehicleMovementComponent* InWheeledVehicleMovementComponent);

	/** FAnimInstanceProxy interface begin*/
	virtual void PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds) override;
	/** FAnimInstanceProxy interface end*/

	const TArray<FWheelAnimData>& GetWheelAnimData() const
	{
		return WheelInstances;
	}

	bool bEnableCustomWheelRot = false;

	TArray<float> WheelCustomRotations = { 0.0f, 0.0f, 0.0f, 0.0f };
	TArray<float> WheelCustomPitch = { 0.0f, 0.0f, 0.0f, 0.0f };
	TArray<float> WheelCustomHeight = { 0.0f, 0.0f, 0.0f, 0.0f };
	
private:
	TArray<FWheelAnimData> WheelInstances;
};

class UE_DEPRECATED(4.26, "PhysX is deprecated. Use the UVehicleAnimationInstance from the ChaosVehiclePhysics Plugin.") UVehicleAnimInstance;
UCLASS(transient)
class PHYSXVEHICLES_API UVehicleAnimInstance : public UAnimInstance
{
	GENERATED_UCLASS_BODY()

	/** Makes a montage jump to the end of a named section. */
	UFUNCTION(BlueprintCallable, Category="Animation")
	class AWheeledVehicle * GetVehicle();

public:
	TArray<FWheelAnimData> WheelData;

public:
	
	void SetWheeledVehicleMovementComponent(const UWheeledVehicleMovementComponent* InWheeledVehicleMovementComponent)
	{
		WheeledVehicleMovementComponent = InWheeledVehicleMovementComponent;
		AnimInstanceProxy.SetWheeledVehicleMovementComponent(InWheeledVehicleMovementComponent);
	}

	const UWheeledVehicleMovementComponent* GetWheeledVehicleMovementComponent() const
	{
		return WheeledVehicleMovementComponent;
	}
	
	void SetWheelRotYaw(uint8 WheelLocation, float AngleInDeg) 
	{
		AnimInstanceProxy.bEnableCustomWheelRot = true;
		AnimInstanceProxy.WheelCustomRotations[WheelLocation] = AngleInDeg;
	}

	void SetWheelPitchAngle(uint8 WheelLocation, float AngleInDeg)
	{
		AnimInstanceProxy.WheelCustomPitch[WheelLocation] = AngleInDeg;
	}

	void SetWheelHeight(uint8 WheelLocation, float Height)
	{
		AnimInstanceProxy.WheelCustomHeight[WheelLocation] = Height;
	}

	float GetWheelRotAngle(uint8 WheelLocation) const
	{
		return AnimInstanceProxy.WheelCustomRotations[WheelLocation];
	}

#define PHYSXVEHICLES_HAS_SET_PITCH_HEIGHT 1
	float GetWheelPitchAngle(uint8 WheelLocation) const
	{
		return AnimInstanceProxy.WheelCustomPitch[WheelLocation];
	}

	float GetWheelHeight(uint8 WheelLocation) const
	{
		return AnimInstanceProxy.WheelCustomHeight[WheelLocation];
	}

	void ResetWheelCustomRotations() 
	{
		AnimInstanceProxy.bEnableCustomWheelRot = false;
		AnimInstanceProxy.WheelCustomRotations[0] = 0.0f;
		AnimInstanceProxy.WheelCustomRotations[1] = 0.0f;
		AnimInstanceProxy.WheelCustomRotations[2] = 0.0f;
		AnimInstanceProxy.WheelCustomRotations[3] = 0.0f;
		AnimInstanceProxy.WheelCustomPitch[0] = 0.0f;
		AnimInstanceProxy.WheelCustomPitch[1] = 0.0f;
		AnimInstanceProxy.WheelCustomPitch[2] = 0.0f;
		AnimInstanceProxy.WheelCustomPitch[3] = 0.0f;
		AnimInstanceProxy.WheelCustomHeight[0] = 0.0f;
		AnimInstanceProxy.WheelCustomHeight[1] = 0.0f;
		AnimInstanceProxy.WheelCustomHeight[2] = 0.0f;
		AnimInstanceProxy.WheelCustomHeight[3] = 0.0f;
	}
	
private:
	/** UAnimInstance interface begin*/
	virtual void NativeInitializeAnimation() override;
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
	virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy) override;
	/** UAnimInstance interface end*/

	FVehicleAnimInstanceProxy AnimInstanceProxy;
	
	UPROPERTY(transient)
	const UWheeledVehicleMovementComponent* WheeledVehicleMovementComponent;

};

PRAGMA_ENABLE_DEPRECATION_WARNINGS


