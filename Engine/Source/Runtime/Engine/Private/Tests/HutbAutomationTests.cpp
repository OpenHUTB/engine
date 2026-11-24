// Copyright 2026 OpenHUTB. All Rights Reserved.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Components/SceneComponent.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "UObject/Package.h"
#include "UObject/UObjectGlobals.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace HutbEngineTests
{
	USceneComponent* NewSceneComponent(const TCHAR* Name)
	{
		return NewObject<USceneComponent>(GetTransientPackage(), FName(Name), RF_Transient);
	}

	UBlueprintGeneratedClass* NewBlueprintGeneratedClass(UPackage* Outer, const TCHAR* BaseName)
	{
		const FName ClassName = MakeUniqueObjectName(Outer, UBlueprintGeneratedClass::StaticClass(), FName(BaseName));
		return NewObject<UBlueprintGeneratedClass>(Outer, ClassName, RF_Public | RF_Transactional);
	}
}

/**
 * Covers the OpenHUTB changes to USceneComponent::SetupAttachment:
 * 1) A call with the same parent and socket as the current values returns early,
 *    so re-initializing an already set up component does not raise an ensure.
 * 2) Self attachment, attaching to a child (cycle), and calling SetupAttachment
 *    after AttachToComponent has already linked the component only raise an
 *    ensure and leave the attachment state untouched.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHutbSetupAttachmentTest, "System.Engine.Hutb.SceneComponent.SetupAttachment", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FHutbSetupAttachmentTest::RunTest(const FString& Parameters)
{
	USceneComponent* Parent = HutbEngineTests::NewSceneComponent(TEXT("HutbParent"));
	USceneComponent* OtherParent = HutbEngineTests::NewSceneComponent(TEXT("HutbOtherParent"));
	USceneComponent* Child = HutbEngineTests::NewSceneComponent(TEXT("HutbChild"));
	const FName SocketName(TEXT("HutbSocket"));

	// Initial state.
	TestNull(TEXT("A new component must not have an AttachParent"), Child->GetAttachParent());
	TestTrue(TEXT("A new component socket must be None"), Child->GetAttachSocketName().IsNone());

	// Regular setup only records the pending attachment and does not touch AttachChildren.
	Child->SetupAttachment(Parent, SocketName);
	TestTrue(TEXT("SetupAttachment must set AttachParent"), Child->GetAttachParent() == Parent);
	TestTrue(TEXT("SetupAttachment must set AttachSocketName"), Child->GetAttachSocketName() == SocketName);
	TestFalse(TEXT("SetupAttachment must not add the component to AttachChildren"), Parent->GetAttachChildren().Contains(Child));

	// Calling again with the same arguments hits the new early out and changes nothing.
	Child->SetupAttachment(Parent, SocketName);
	TestTrue(TEXT("Repeated SetupAttachment must keep AttachParent"), Child->GetAttachParent() == Parent);
	TestTrue(TEXT("Repeated SetupAttachment must keep AttachSocketName"), Child->GetAttachSocketName() == SocketName);

	// Switching parents resets the socket to None.
	Child->SetupAttachment(OtherParent);
	TestTrue(TEXT("Switching parents must update AttachParent"), Child->GetAttachParent() == OtherParent);
	TestTrue(TEXT("Switching parents must reset AttachSocketName"), Child->GetAttachSocketName().IsNone());

	// Passing nullptr clears the pending attachment.
	Child->SetupAttachment(nullptr);
	TestNull(TEXT("SetupAttachment(nullptr) must clear AttachParent"), Child->GetAttachParent());
	TestTrue(TEXT("SetupAttachment(nullptr) must clear AttachSocketName"), Child->GetAttachSocketName().IsNone());

	// Self attachment is rejected by an ensure and does not change the state.
	AddExpectedError(TEXT("Ensure condition failed: InParent != this"), EAutomationExpectedErrorFlags::Contains, 1);
	Child->SetupAttachment(Child);
	TestNull(TEXT("Self attachment must not change AttachParent"), Child->GetAttachParent());

	// Attaching a parent to its own pending child would create a cycle and is rejected.
	Child->SetupAttachment(Parent);
	AddExpectedError(TEXT("Ensure condition failed: InParent == nullptr || !InParent->IsAttachedTo(this)"), EAutomationExpectedErrorFlags::Contains, 1);
	Parent->SetupAttachment(Child);
	TestNull(TEXT("Creating a cycle must not change AttachParent"), Parent->GetAttachParent());

	// Once linked through AttachToComponent, SetupAttachment must not rewire the component.
	USceneComponent* AttachedChild = HutbEngineTests::NewSceneComponent(TEXT("HutbAttachedChild"));
	AttachedChild->AttachToComponent(Parent, FAttachmentTransformRules::KeepRelativeTransform);
	TestTrue(TEXT("AttachToComponent must set AttachParent"), AttachedChild->GetAttachParent() == Parent);
	TestTrue(TEXT("AttachToComponent must register the child in AttachChildren"), Parent->GetAttachChildren().Contains(AttachedChild));

	AddExpectedError(TEXT("Ensure condition failed: AttachParent == nullptr || !AttachParent->AttachChildren.Contains(this)"), EAutomationExpectedErrorFlags::Contains, 1);
	AttachedChild->SetupAttachment(OtherParent);
	TestTrue(TEXT("SetupAttachment must not rewire an attached component"), AttachedChild->GetAttachParent() == Parent);

	return true;
}

/**
 * Covers the OpenHUTB changes to UBlueprintGeneratedClass::IsAsset:
 * the regular asset definition still applies, but a BPGC that was replaced by a
 * newer class version is no longer reported as an asset.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHutbBlueprintGeneratedClassIsAssetTest, "System.Engine.Hutb.BlueprintGeneratedClass.IsAsset", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FHutbBlueprintGeneratedClassIsAssetTest::RunTest(const FString& Parameters)
{
	UPackage* Package = CreatePackage(TEXT("/Temp/HutbBPGCIsAsset"));
	TestNotNull(TEXT("A temporary package must be created"), Package);
	if (Package == nullptr)
	{
		return false;
	}

	UBlueprintGeneratedClass* TestClass = HutbEngineTests::NewBlueprintGeneratedClass(Package, TEXT("HutbTestBPGC"));
	TestNotNull(TEXT("A UBlueprintGeneratedClass must be created"), TestClass);
	if (TestClass == nullptr)
	{
		return false;
	}

	// A regular BPGC (RF_Public, non-transient outer package) is an asset.
	TestTrue(TEXT("A regular BPGC must be an asset"), TestClass->IsAsset());

	// A BPGC replaced by a newer version is not an asset anymore.
	TestClass->ClassFlags = EClassFlags(TestClass->ClassFlags | CLASS_NewerVersionExists);
	TestFalse(TEXT("A BPGC with CLASS_NewerVersionExists must not be an asset"), TestClass->IsAsset());
	TestClass->ClassFlags = EClassFlags(TestClass->ClassFlags & ~CLASS_NewerVersionExists);

	// Transient objects and CDOs follow the UObject::IsAsset rules.
	TestClass->SetFlags(RF_Transient);
	TestFalse(TEXT("A transient BPGC must not be an asset"), TestClass->IsAsset());
	TestClass->ClearFlags(RF_Transient);

	TestClass->SetFlags(RF_ClassDefaultObject);
	TestFalse(TEXT("A CDO must not be an asset"), TestClass->IsAsset());
	TestClass->ClearFlags(RF_ClassDefaultObject);

	// Clearing the flags restores the asset state.
	TestTrue(TEXT("A BPGC must be an asset again after clearing the flags"), TestClass->IsAsset());

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
