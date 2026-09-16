#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"
#include "Facility/BathWaterStateComponent.h"
#include "Interaction/PlayerInteractable.h"
#include "BathWaterControlComponent.generated.h"

class ABathhouseBathFacilityActor;

UCLASS(ClassGroup = (Bathhouse), meta = (BlueprintSpawnableComponent))
class BATHHOUSESIM_API UBathWaterControlComponent
	: public UStaticMeshComponent
	, public IPlayerInteractable
{
	GENERATED_BODY()

public:
	UBathWaterControlComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

	virtual FPlayerInteractionQuery QueryInteraction(const FPlayerInteractionContext& Context) const override;
	virtual FPlayerInteractionResult ExecuteInteraction(const FPlayerInteractionContext& Context) override;

	UFUNCTION(BlueprintPure, Category = "Bath Water|Control")
	EBathWaterControlType GetControlType() const { return ControlType; }

	UFUNCTION(BlueprintPure, Category = "Bath Water|Control")
	bool IsMoving() const { return bMoving; }

	void InitializeClosedPose();
	void ApplyLogicalState(bool bOpen, EBathWaterControlChangeReason Reason);
	void BeginRecoveryFreeze();
	void CancelRecoveryFreeze();
	void PrepareRecoveryCommit();
	bool HasValidInteractionGeometry() const;
	FQuat CalculateOpenRotation() const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bath Water|Control")
	FVector LocalRotationAxis = FVector::UpVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bath Water|Control")
	float OpenAngleDegrees = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bath Water|Control", meta = (ClampMin = "0.01"))
	float RotationDurationSeconds = 0.5f;

private:
	friend class ABathhouseBathFacilityActor;
	bool ValidateMotionAuthoring(FText& OutFailureReason) const;
	bool HasFiniteMotionState() const;
	void ConfigureControlType(EBathWaterControlType InControlType) { ControlType = InControlType; }
	void StartMotion(bool bOpen, bool bAllowMidMotionReversal);
	void StopAtRotation(const FQuat& Rotation);
	ABathhouseBathFacilityActor* GetBathOwner() const;

	struct FRecoveryMotionSnapshot
	{
		FQuat CurrentRotation = FQuat::Identity;
		FQuat StartRotation = FQuat::Identity;
		FQuat TargetRotation = FQuat::Identity;
		float Elapsed = 0.0f;
		float Duration = 0.0f;
		bool bMoving = false;
		bool bOpening = false;
		bool bTickEnabled = false;
	};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bath Water|Control", meta = (AllowPrivateAccess = "true"))
	EBathWaterControlType ControlType = EBathWaterControlType::FillValve;

	TOptional<FRecoveryMotionSnapshot> RecoveryMotionSnapshot;
	FQuat ClosedRotation = FQuat::Identity;
	FQuat MotionStartRotation = FQuat::Identity;
	FQuat MotionTargetRotation = FQuat::Identity;
	float MotionElapsed = 0.0f;
	float MotionDuration = 0.0f;
	bool bClosedPoseInitialized = false;
	bool bMoving = false;
	bool bOpening = false;
	bool bRecoveryFrozen = false;
};
