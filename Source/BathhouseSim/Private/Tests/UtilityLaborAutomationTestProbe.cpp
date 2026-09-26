#include "Tests/UtilityLaborAutomationTestProbe.h"

#include "Utility/UtilityFuelIntakeVolumeComponent.h"
#include "Utility/UtilityFuelTransaction.h"
#include "Utility/UtilityOperationComponent.h"
#include "Utility/UtilityShovelActor.h"
#include "Components/BoxComponent.h"
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
	GetFuelIntakeVolume()->SetBoxExtent(FVector(15.0f));
	GetFuelIntakeVolume()->SetRelativeScale3D(FVector::OneVector);
	TArray<UStaticMeshComponent*> MeshComponents;
	GetComponents(MeshComponents);
	for (UStaticMeshComponent* MeshComponent : MeshComponents)
	{
		if (MeshComponent && MeshComponent->GetFName() == TEXT("GaugeNeedleMesh"))
		{
			MeshComponent->SetStaticMesh(TestMesh.Object);
		}
	}
	GetFuelDoorMesh()->SetStaticMesh(TestMesh.Object);
}

void UUtilityLaborFuelChangedAutomationProbe::Bind(
	AUtilityShovelActor* InSourceShovel,
	AUtilityShovelActor* InNestedShovel,
	UUtilityOperationComponent* InOperation,
	UUtilityFuelIntakeVolumeComponent* InIntake,
	const FPlayerInteractionContext& InContext,
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
