#include "Tests/UtilityLaborAutomationTestProbe.h"

#include "Utility/UtilityFuelIntakeComponent.h"
#include "Utility/UtilityFuelTransaction.h"
#include "Utility/UtilityOperationComponent.h"
#include "Utility/UtilityShovelActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

AUtilityLaborBoilerAutomationActor::AUtilityLaborBoilerAutomationActor()
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> TestMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (!TestMesh.Succeeded())
	{
		return;
	}
	GetFuelIntake()->SetStaticMesh(TestMesh.Object);
	TArray<UStaticMeshComponent*> MeshComponents;
	GetComponents(MeshComponents);
	for (UStaticMeshComponent* MeshComponent : MeshComponents)
	{
		if (MeshComponent && (MeshComponent->GetFName() == TEXT("GaugeFace")
			|| MeshComponent->GetFName() == TEXT("GaugeNeedleMesh")))
		{
			MeshComponent->SetStaticMesh(TestMesh.Object);
		}
	}
}

void UUtilityLaborFuelChangedAutomationProbe::Bind(
	AUtilityShovelActor* InSourceShovel,
	AUtilityShovelActor* InNestedShovel,
	UUtilityOperationComponent* InOperation,
	UUtilityFuelIntakeComponent* InIntake,
	const FHeldEquipmentUseContext& InContext,
	const float InExpectedOperationPoints)
{
	Unbind();
	SourceShovel = InSourceShovel;
	NestedShovel = InNestedShovel;
	Operation = InOperation;
	Intake = InIntake;
	Context = InContext;
	ExpectedOperationPoints = InExpectedOperationPoints;
	if (IsValid(SourceShovel))
	{
		SourceShovel->OnFuelLoadChanged.AddDynamic(this, &UUtilityLaborFuelChangedAutomationProbe::HandleFuelLoadChanged);
	}
}

void UUtilityLaborFuelChangedAutomationProbe::Unbind()
{
	if (IsValid(SourceShovel))
	{
		SourceShovel->OnFuelLoadChanged.RemoveDynamic(this, &UUtilityLaborFuelChangedAutomationProbe::HandleFuelLoadChanged);
	}
}

void UUtilityLaborFuelChangedAutomationProbe::HandleFuelLoadChanged(const FUtilityFuelLoad Load)
{
	++CallbackCount;
	bSawCompleteCommittedState = Load.IsEmpty()
		&& IsValid(SourceShovel) && SourceShovel->IsLoadEmpty()
		&& IsValid(Operation)
		&& FMath::IsNearlyEqual(Operation->GetRemainingPoints(), ExpectedOperationPoints)
		&& IsValid(NestedShovel)
		&& FMath::IsNearlyEqual(NestedShovel->GetFuelLoad().Points, 25.0f);

	if (!IsValid(NestedShovel) || !IsValid(Intake))
	{
		return;
	}
	const FUtilityFuelResult NestedResult = FUtilityFuelTransaction::Insert(*NestedShovel, *Intake, Context);
	bReentrantInsertRejectedByGuard = !NestedResult.bSucceeded
		&& NestedResult.Failure == EUtilityFuelFailure::TransactionBusy;
}
